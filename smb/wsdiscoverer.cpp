/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>
*/

#include "wsdiscoverer.h"

#include <QCoreApplication>
#include <QDebug>
#include <QHostInfo>
#include <QRegularExpression>
#include <QUuid>

#include <WSDiscoveryClient>
#include <WSDiscoveryTargetService>

#include <KDSoapClient/KDSoapClientInterface>
#include <KDSoapClient/KDSoapMessage>
#include <KDSoapClient/KDSoapMessageAddressingProperties>
#include <KDSoapClient/KDSoapNamespaceManager>

#include <KIO/UDSEntry>
#include <KLocalizedString>

#include <chrono>

#include "kio_smb.h"

using namespace std::chrono_literals;

// http://docs.oasis-open.org/ws-dd/dpws/wsdd-dpws-1.1-spec.html
// WSD itself defines shorter timeouts. We follow DPWS instead because Windows 10 actually speaks DPWS, so it seems
// prudent to follow its presumed internal limits.
// - discard Probe & ResolveMatch N seconds after corresponding Probe:
constexpr auto MATCH_TIMEOUT = 10s;

// Not specified default value for HTTP timeouts. We could go with a default 120s or 600s timeout but that seems
// a bit excessive. We only use SOAP over HTTP for device PBSD resolution.
constexpr auto HTTP_TIMEOUT = 20s;

// Publication service data resolver!
// Specifically we'll ask the endpoint for PBSData via ws-transfer/Get.
// The implementation is the bare minimum for our purposes!
// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-pbsd
class PBSDResolver : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void resolved(Discovery::Ptr discovery);

public:
    /**
     * @param endpointUrl valid xaddr as advertised over ws-discovery (http://$ip/$referenceUuid)
     * @param destination endpoint reference urn as sent over ws-discovery ($referenceUuid)
     */
    PBSDResolver(const QUrl &endpointUrl, const QString &destination, QObject *parent = nullptr)
        : QObject(parent)
        , m_endpointUrl(endpointUrl)
        , m_destination(destination)
    {
    }

    static QString nameFromComputerInfo(const QString &info)
    {
        // NB: spec says to use \ or / based on context, but in reality they are used
        //    interchangeably in implementations.
        static QRegularExpression domainExpression("(?<name>.+)[\\/]Domain:(?<domain>.+)");
        static QRegularExpression workgroupExpression("(?<name>.+)[\\/]Workgroup:(?<workgroup>.+)");
        static QRegularExpression notJoinedExpression("(?<name>.+)[\\/]NotJoined");

        // We don't do anything with WG or domain info because windows10 doesn't seem to either.
        const auto joinedMatch = notJoinedExpression.match(info);
        if (joinedMatch.hasMatch()) {
            return joinedMatch.captured("name");
        }

        const auto domainMatch = domainExpression.match(info);
        if (domainMatch.hasMatch()) {
            return domainMatch.captured("name");
        }

        const auto workgroupMatch = workgroupExpression.match(info);
        if (workgroupMatch.hasMatch()) {
            return workgroupMatch.captured("name");
        }

        return info;
    }

    // This must always set m_discovery and it must also time out on its own!
    void run()
    {
        // NB: when windows talks to windows they use lms:LargeMetadataSupport we probably don't
        // need this for the data we want, so it's left out. The actual messages a windows
        // machine creates would be using "http://schemas.microsoft.com/windows/lms/2007/08"
        // as messageNamespace and set an additional header <LargeMetadataSupport/> on the message.
        // https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-dpwssn/f700463d-cbbf-4545-ab47-b9a6fbf1ac7b

        KDSoapClientInterface client(m_endpointUrl.toString(),
                                     QStringLiteral("http://schemas.xmlsoap.org/ws/2004/09/transfer"));
        client.setSoapVersion(KDSoapClientInterface::SoapVersion::SOAP1_2);
        client.setTimeout(std::chrono::milliseconds(HTTP_TIMEOUT).count());

        KDSoapMessage message;
        KDSoapMessageAddressingProperties addressing;
        addressing.setAddressingNamespace(KDSoapMessageAddressingProperties::Addressing200408);
        addressing.setAction(QStringLiteral("http://schemas.xmlsoap.org/ws/2004/09/transfer/Get"));
        addressing.setMessageID(QStringLiteral("urn:uuid:") + QUuid::createUuid().toString(QUuid::WithoutBraces));
        addressing.setDestination(m_destination);
        addressing.setReplyEndpointAddress(KDSoapMessageAddressingProperties::predefinedAddressToString(
                                               KDSoapMessageAddressingProperties::Anonymous,
                                               KDSoapMessageAddressingProperties::Addressing200408));
        addressing.setSourceEndpointAddress(QStringLiteral("urn:uuid:")
                                            + QUuid::createUuid().toString(QUuid::WithoutBraces));
        message.setMessageAddressingProperties(addressing);

        QString computer;

        KDSoapMessage response = client.call(QString(), message);
        if (response.isFault()) {
            qCDebug(KIO_SMB_LOG) << "Failed to obtain PBSD response"
                                 << m_endpointUrl.host()
                                 << m_destination
                                 << response.arguments()
                                 << response.faultAsString();
            // No return! We'd disqualify systems that do not implement pbsd.
        } else {
            // The response xml would be nesting Metdata<MetadataSection<Relationship<Host<Computer
            // https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-pbsd/ec0810ba-2427-46f5-8d47-cc94919ee4c1
            // The value therein is $netbiosname/Domain:$domain or $netbiosname\Workgroup:$workgroup or $netbiosname\NotJoined
            // (yes, that is either a forward or backward slash!). In practice everyone uses a / for everything though!
            // For simplicity's sake we'll manually pop the value (or empty) out, if we get a name it's grand
            // otherwise we'll attempt reverse resolution from the IP (which ideally would yield results
            // over systemd-resolved's llmnr).

            const auto childValues = response.childValues();
            for (const auto &section : qAsConst(childValues)) {
                computer = section
                           .childValues().child("Relationship")
                           .childValues().child("Host")
                           .childValues().child("Computer").value().toString();
                if (!computer.isEmpty()) {
                    break;
                }
            }

            computer = nameFromComputerInfo(computer);
        }

        if (computer.isEmpty()) {
            // Chances are if we get here the remote doesn't implement PBSD.
            // Shouldn't really happen. Seeing as we got a return message
            // for our PBSD request however we'll assume the implementation is
            // just misbehaving and will assume it supports SMB all the same.

            // As a shot in the dark try to resolver via QHostInfo (which ideally
            // does a LMNR lookup via libc/systemd)
            auto hostInfo = QHostInfo::fromName(m_endpointUrl.host());
            if (hostInfo.error() == QHostInfo::NoError) {
                computer = hostInfo.hostName();
            } else {
                qCWarning(KIO_SMB_LOG) << "failed to resolve host for endpoint url:" << m_endpointUrl;
            }
        }

        // Default the host to the IP unless we have a prettyName in which case we use
        // the presumed DNSSD name prettyName.local. listDir will redirect this to the
        // LLMNR variant (without .local) should it turn out not resolvable.
        QString host = m_endpointUrl.host();
        if (computer.isEmpty()) {
            computer = xi18nc("host entry when no pretty name is available. %1 likely is an IP address",
                              "Unknown Device @ <resource>%1</resource>",
                              host);
        } else {
            // If we got a DNSSD name, use that, otherwise redirect to on-demand resolution.
            host = computer.endsWith(".local") ? computer : computer + ".kio-discovery-wsd";
        }

        m_discovery.reset(new WSDiscovery(computer, host));
        Q_EMIT resolved(m_discovery);
    }

private:
    const QUrl m_endpointUrl;
    const QString m_destination;
    Discovery::Ptr m_discovery;
};

// Wraps a /Resolve request. Resolves are subject to timeouts which is implemented by only waiting for a reply until
// the internal timeout is hit and then deleting the resolver.
class WSDResolver : public QObject
{
    Q_OBJECT
public:
    explicit WSDResolver(const QString &endpoint, QObject *parent = nullptr)
        : QObject(parent)
        , m_endpoint(endpoint)
    {
        connect(&m_client, &WSDiscoveryClient::resolveMatchReceived, this, [this](const WSDiscoveryTargetService &service) {
            Q_ASSERT(service.endpointReference() == m_endpoint);
            Q_EMIT resolved(service);
            stop();
        });

        m_stopTimer.setInterval(MATCH_TIMEOUT); // R4066 of DPWS spec
        m_stopTimer.setSingleShot(true);
        connect(&m_stopTimer, &QTimer::timeout, this, &WSDResolver::stop);
    }

public Q_SLOTS:
    void start()
    {
        m_client.sendResolve(m_endpoint);
        m_stopTimer.start();
    }

    void stop()
    {
        m_stopTimer.stop();
        disconnect(&m_stopTimer);
        Q_EMIT stopped();
    }

Q_SIGNALS:
    void resolved(const WSDiscoveryTargetService &service);
    void stopped();

private:
    const QString m_endpoint;
    WSDiscoveryClient m_client;
    QTimer m_stopTimer;
};

// Utilizes WSDiscoveryClient to probe and resolve WSD services.
WSDiscoverer::WSDiscoverer()
    : m_client(new WSDiscoveryClient(this))
{
    connect(m_client, &WSDiscoveryClient::probeMatchReceived,
            this, &WSDiscoverer::matchReceived);

    // Matches may only arrive within a given time period afterwards we no
    // longer care as per the spec. stopping is further contigent on all
    // resolvers having finished though (they each have timeouts as well).
    m_probeMatchTimer.setInterval(MATCH_TIMEOUT);
    m_probeMatchTimer.setSingleShot(true);
    connect(&m_probeMatchTimer, &QTimer::timeout, this, &WSDiscoverer::stop);
}

WSDiscoverer::~WSDiscoverer()
{
    qDeleteAll(m_resolvers);
    qDeleteAll(m_endpointResolvers);
}

void WSDiscoverer::start()
{
    m_client->start();

    // We only want devices.
    // We technically would probably also want to filter pub:Computer.
    // But! I am not sure if e.g. a NAS would publish itself as computer.
    // https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-pbsd
    KDQName type("wsdp:Device");
    type.setNameSpace("http://schemas.xmlsoap.org/ws/2006/02/devprof");
    m_client->sendProbe({type}, {});

    // (re)start match timer to finish-early if at all possible.
    m_probeMatchTimer.start();
    m_startedTimer = true;
}

void WSDiscoverer::stop()
{
    m_startedTimer = true;
    disconnect(&m_probeMatchTimer);
    m_probeMatchTimer.stop();
    maybeFinish();
}

bool WSDiscoverer::isFinished() const
{
    const bool notProbing = !m_probeMatchTimer.isActive();
    const bool notWaitingForWSDResolve = m_endpointResolvers.isEmpty();
    const bool notWaitingForPBSD = m_resolvers.count() == m_resolvedCount;
    return m_startedTimer && (notProbing && notWaitingForWSDResolve && notWaitingForPBSD);
}

void WSDiscoverer::matchReceived(const WSDiscoveryTargetService &matchedService)
{
    if (!m_probeMatchTimer.isActive()) { // R4065 of DPWS spec
        qCWarning(KIO_SMB_LOG) << "match received too late" << matchedService.endpointReference();
        return;
    }

    if (matchedService.xAddrList().isEmpty()) { // Has no addresses -> needs resolving still
        const QString endpoint = matchedService.endpointReference();
        if (m_seenEndpoints.contains(endpoint) || m_endpointResolvers.contains(endpoint)) {
            return;
        }

        auto resolver = new WSDResolver(endpoint, this);
        connect(resolver, &WSDResolver::resolved, this, &WSDiscoverer::resolveReceived);
        connect(resolver, &WSDResolver::stopped, this, [this, endpoint] {
            if (m_endpointResolvers.contains(endpoint)) {
                m_endpointResolvers.take(endpoint)->deleteLater();
            }
            maybeFinish();
        });
        m_endpointResolvers.insert(endpoint, resolver);
        resolver->start();

        return;
    }

    resolveReceived(matchedService);
}

void WSDiscoverer::resolveReceived(const WSDiscoveryTargetService &service)
{
    if (m_seenEndpoints.contains(service.endpointReference())) {
        return;
    }
    m_seenEndpoints << service.endpointReference();

    QUrl addr;
    const QList<QUrl> xAddrList = service.xAddrList();
    for (const auto &xAddr : xAddrList) {
        // https://docs.microsoft.com/en-us/windows/win32/wsdapi/xaddr-validation-rules
        // "At least one IP address included in the XAddrs (or IP address resolved from
        // a hostname included in the XAddrs) must be on the same subnet as the adapter
        // over which the ProbeMatches or ResolveMatches message was received."
        const auto hostInfo = QHostInfo::fromName(xAddr.host());
        if (hostInfo.error() == QHostInfo::NoError) {
            addr = xAddr;
            break;
        }
    }

    if (addr.isEmpty()) {
        qCWarning(KIO_SMB_LOG) << "Failed to resolve any WS transport address."
                               << "This suggests that DNS resolution may be broken."
                               << service.xAddrList();
        return;
    }

    auto *resolver = new PBSDResolver(addr, service.endpointReference(), this);
    connect(resolver, &PBSDResolver::resolved, this, [this](Discovery::Ptr discovery) {
        ++m_resolvedCount;
        Q_EMIT newDiscovery(discovery);
        maybeFinish();
    });
    QTimer::singleShot(0, resolver, &PBSDResolver::run);
    m_resolvers << resolver;
}

void WSDiscoverer::maybeFinish()
{
    if (isFinished()) {
        Q_EMIT finished();
    }
}

WSDiscovery::WSDiscovery(const QString &computer, const QString &remote)
    : m_computer(computer)
    , m_remote(remote)
{
}

QString WSDiscovery::udsName() const
{
    return m_computer;
}

KIO::UDSEntry WSDiscovery::toEntry() const
{
    KIO::UDSEntry entry;
    const int fastInsertCount = 6;
    entry.reserve(fastInsertCount);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, udsName());

    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, (S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH));
    entry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, "network-server");

    QUrl u;
    u.setScheme(QStringLiteral("smb"));
    u.setHost(m_remote);
    u.setPath("/"); // https://bugs.kde.org/show_bug.cgi?id=388922

    entry.fastInsert(KIO::UDSEntry::UDS_URL, u.url());
    entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE,
                     QStringLiteral("application/x-smb-server"));
    return entry;
}

#include "wsdiscoverer.moc"
