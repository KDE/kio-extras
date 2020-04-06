/*
    Copyright 2019-2020 Harald Sitter <sitter@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3 of
    the License or any later version accepted by the membership of
    KDE e.V. (or its successor approved by the membership of KDE
    e.V.), which shall act as a proxy defined in Section 14 of
    version 3 of the license.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

#include "kio_smb.h"

// Publication service data resolver!
// Specifically we'll ask the endpoint for PBSData via ws-transfer/Get.
// The implementation is the bare minimum for our purposes!
// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-pbsd
class PBSDResolver : public QObject
{
    Q_OBJECT
signals:
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
        //    interchangibly in implementations.
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
        // need this for the data we want, so it's left out. The actual messagse a windows
        // machine creates would be using "http://schemas.microsoft.com/windows/lms/2007/08"
        // as messageNamespace and set an additional header <LargeMetadataSupport/> on the message.
        // https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-dpwssn/f700463d-cbbf-4545-ab47-b9a6fbf1ac7b

        KDSoapClientInterface client(m_endpointUrl.toString(),
                                     QStringLiteral("http://schemas.xmlsoap.org/ws/2004/09/transfer"));
        client.setSoapVersion(KDSoapClientInterface::SoapVersion::SOAP1_2);
        client.setTimeout(8000);

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
            // (yes, that is either a forward or backward slash!). In practise everyone uses a / for everything though!
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
        emit resolved(m_discovery);
    }

private:
    const QUrl m_endpointUrl;
    const QString m_destination;
    Discovery::Ptr m_discovery;
};

// Utilizes WSDiscoveryClient to probe and resolve WSD services.
WSDiscoverer::WSDiscoverer()
    : m_client(new WSDiscoveryClient(this))
{
    connect(m_client, &WSDiscoveryClient::probeMatchReceived,
            this, &WSDiscoverer::matchReceived);
    connect(m_client, &WSDiscoveryClient::resolveMatchReceived,
            this, &WSDiscoverer::resolveReceived);

    // If we haven't had a probematch in some seconds there's likely no more replies
    // coming and all hosts are known. Naturally resolvers may still be running and
    // get blocked on during stop(). Resolvers themselves have a timeout via
    // kdsoap.
    // NB: only started after first match! If we have no matches the slave will
    // stop us eventually anyway.
    m_probeMatchTimer.setInterval(2000);
    m_probeMatchTimer.setSingleShot(true);
    connect(&m_probeMatchTimer, &QTimer::timeout, this, &WSDiscoverer::stop);
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
    return m_startedTimer && !m_probeMatchTimer.isActive() && m_resolvers.count() == m_resolvedCount;
}

void WSDiscoverer::matchReceived(const WSDiscoveryTargetService &matchedService)
{
    // (re)start match timer to finish-early if at all possible.
    m_probeMatchTimer.start();
    m_startedTimer = true;

    if (matchedService.xAddrList().isEmpty()) {
        // Has no addresses -> needs resolving still
        m_client->sendResolve(matchedService.endpointReference());
        return;
    }

    resolveReceived(matchedService);
}

void WSDiscoverer::resolveReceived(const WSDiscoveryTargetService &service)
{
    // (re)start match timer to finish-early if at all possible.
    m_probeMatchTimer.start();
    m_startedTimer = true;

    if (m_seenEndpoints.contains(service.endpointReference())) {
        return;
    }
    m_seenEndpoints << service.endpointReference();

    QUrl addr;
    for (const auto &xAddr : service.xAddrList()) {
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

    PBSDResolver *resolver = new PBSDResolver(addr, service.endpointReference(), this);
    connect(resolver, &PBSDResolver::resolved, this, [this](Discovery::Ptr discovery) {
        ++m_resolvedCount;
        emit newDiscovery(discovery);
        maybeFinish();
    });
    QTimer::singleShot(0, resolver, &PBSDResolver::run);
    m_resolvers << resolver;
}

void WSDiscoverer::maybeFinish()
{
    if (isFinished()) {
        emit finished();
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
    entry.reserve(6);
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
