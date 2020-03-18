/* Copyright (C) 2019-2020 Casper Meijn <casper@meijn.net>
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <KDSoapClient/KDQName>
#include <QNetworkDatagram>
#include <QSignalSpy>
#include <QTest>
#include <QUdpSocket>
#include <QXmlStreamReader>
#include "wsdiscoveryclient.h"
#include "wsdiscoverytargetservice.h"

Q_DECLARE_METATYPE(WSDiscoveryTargetService)

class testWSDiscoveryClient: public QObject
{
    Q_OBJECT
public:
    explicit testWSDiscoveryClient(QObject* parent = nullptr) :
        QObject(parent) {;}
        
private slots:
    static void testSendProbe();
    static void testSendResolve();
    static void testReceiveProbeMatch();
    static void testReceiveResolveMatch();
    
private:
    static QByteArray zeroOutUuid(const QByteArray& original);
    static QByteArray expectedSendProbeData();
    static QByteArray expectedSendResolveData();
    static QByteArray toBeSendProbeMatchData();
    static QByteArray toBeSendResolveMatchData();
    static QByteArray formatXml(const QByteArray& original);
};

void testWSDiscoveryClient::testSendProbe() 
{
    QUdpSocket testSocket;
    QVERIFY(testSocket.bind(QHostAddress::Any, 3702, QAbstractSocket::ShareAddress));
    QVERIFY(testSocket.joinMulticastGroup(QHostAddress(QStringLiteral("FF02::C"))));

    KDQName type(QStringLiteral("tdn:NetworkVideoTransmitter"));
    type.setNameSpace(QStringLiteral("http://www.onvif.org/ver10/network/wsdl"));
    auto typeList = QList<KDQName>() << type;
    
    auto scopeList = QList<QUrl>() << QUrl(QStringLiteral("onvif://www.onvif.org/Profile/Streaming"));
    
    WSDiscoveryClient discoveryClient;
    discoveryClient.start();
    discoveryClient.sendProbe(typeList, scopeList);

    QVERIFY(testSocket.hasPendingDatagrams());
    auto datagram = testSocket.receiveDatagram();
    auto zeroedDatagram = zeroOutUuid(datagram.data());
    QCOMPARE(formatXml(zeroedDatagram), formatXml(expectedSendProbeData()));
}

QByteArray testWSDiscoveryClient::expectedSendProbeData() {
    return QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<soap:Envelope"
        "  xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\""
        "  xmlns:soap-enc=\"http://www.w3.org/2003/05/soap-encoding\""
        "  xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
        "  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
        "  xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\""
        "  xmlns:n1=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
        "  <soap:Header>"
        "    <wsa:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>"
        "    <wsa:ReplyTo><wsa:Address>http://www.w3.org/2005/08/addressing/anonymous</wsa:Address></wsa:ReplyTo>"
        "    <wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action>"
        "    <wsa:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</wsa:MessageID>"
        "  </soap:Header>"
        "  <soap:Body>"
        "    <n1:Probe>"
        "      <n1:Types xmlns:tdn=\"http://www.onvif.org/ver10/network/wsdl\">tdn:NetworkVideoTransmitter</n1:Types>"
        "      <n1:Scopes>onvif://www.onvif.org/Profile/Streaming</n1:Scopes>"
        "    </n1:Probe>"
        "  </soap:Body>"
        "</soap:Envelope>");
}

void testWSDiscoveryClient::testSendResolve() 
{
    QUdpSocket testSocket;
    QVERIFY(testSocket.bind(QHostAddress::Any, 3702, QAbstractSocket::ShareAddress));
    QVERIFY(testSocket.joinMulticastGroup(QHostAddress(QStringLiteral("FF02::C"))));

    WSDiscoveryClient discoveryClient;
    discoveryClient.start();
    discoveryClient.sendResolve(QStringLiteral("A_Unique_Reference"));

    QVERIFY(testSocket.hasPendingDatagrams());
    auto datagram = testSocket.receiveDatagram();
    auto zeroedDatagram = zeroOutUuid(datagram.data());
    QCOMPARE(formatXml(zeroedDatagram), formatXml(expectedSendResolveData()));
}

QByteArray testWSDiscoveryClient::expectedSendResolveData() {
    return QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<soap:Envelope"
        "  xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\""
        "  xmlns:soap-enc=\"http://www.w3.org/2003/05/soap-encoding\""
        "  xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
        "  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
        "  xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\""
        "  xmlns:n1=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
        "  <soap:Header>"
        "    <wsa:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>"
        "    <wsa:ReplyTo><wsa:Address>http://www.w3.org/2005/08/addressing/anonymous</wsa:Address></wsa:ReplyTo>"
        "    <wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Resolve</wsa:Action>"
        "    <wsa:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</wsa:MessageID>"
        "  </soap:Header>"
        "  <soap:Body>"
        "    <n1:Resolve>"
        "      <wsa:EndpointReference>"
        "        <wsa:Address>A_Unique_Reference</wsa:Address>"
        "      </wsa:EndpointReference>"
        "    </n1:Resolve>"
        "  </soap:Body>"
        "</soap:Envelope>");
}

void testWSDiscoveryClient::testReceiveProbeMatch() 
{
    WSDiscoveryClient discoveryClient;
    discoveryClient.start();
    
    qRegisterMetaType<WSDiscoveryTargetService>();
    QSignalSpy spy(&discoveryClient, &WSDiscoveryClient::probeMatchReceived);
    QVERIFY(spy.isValid());
    
    QUdpSocket testSocket;
    QVERIFY(testSocket.bind(QHostAddress::Any, 3702, QAbstractSocket::ShareAddress));
    QVERIFY(testSocket.joinMulticastGroup(QHostAddress(QStringLiteral("FF02::C"))));
    testSocket.writeDatagram(toBeSendProbeMatchData(), QHostAddress(QStringLiteral("FF02::C")), 3702);
    
    QVERIFY(spy.wait(1000));
    
    QCOMPARE(spy.count(), 1); // make sure the signal was emitted exactly one time
    QList<QVariant> arguments = spy.takeFirst(); // take the first signal
    
    const WSDiscoveryTargetService& probeMatchService = qvariant_cast<WSDiscoveryTargetService>(arguments.at(0));

    QCOMPARE(probeMatchService.endpointReference(), "Incomming_unique_reference");
    QCOMPARE(probeMatchService.scopeList().size(), 1);
    QCOMPARE(probeMatchService.scopeList().at(0), QUrl(QStringLiteral("ldap:///ou=engineering,o=examplecom,c=us")));
    QCOMPARE(probeMatchService.typeList().size(), 1);
    QCOMPARE(probeMatchService.typeList().at(0), KDQName(QStringLiteral("http://printer.example.org/2003/imaging"), QStringLiteral("PrintBasic")));
    QCOMPARE(probeMatchService.xAddrList().size(), 1);
    QCOMPARE(probeMatchService.xAddrList().at(0), QUrl(QStringLiteral("http://prn-example/PRN42/b42-1668-a")));
    QVERIFY(probeMatchService.lastSeen().msecsTo(QDateTime::currentDateTime()) < 500);
}

QByteArray testWSDiscoveryClient::toBeSendProbeMatchData()
{
    return QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<soap:Envelope"
        "  xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\""
        "  xmlns:soap-enc=\"http://www.w3.org/2003/05/soap-encoding\""
        "  xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
        "  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
        "  xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\""
        "  xmlns:n1=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
        "  <soap:Header>"
        "    <wsa:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>"
        "    <wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches</wsa:Action>"
        "    <wsa:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</wsa:MessageID>"
        "    <wsa:RelatesTo>xs:anyURI</wsa:RelatesTo>"
        "    <n1:AppSequence InstanceId=\"12\" MessageNumber=\"12\"/>"
        "  </soap:Header>"
        "  <soap:Body>"
        "    <n1:ProbeMatches>"
        "      <n1:ProbeMatch>"
        "        <wsa:EndpointReference><wsa:Address>Incomming_unique_reference</wsa:Address></wsa:EndpointReference>"
        "        <n1:Types xmlns:i=\"http://printer.example.org/2003/imaging\">i:PrintBasic</n1:Types>"
        "        <n1:Scopes>ldap:///ou=engineering,o=examplecom,c=us</n1:Scopes>"
        "        <n1:XAddrs>http://prn-example/PRN42/b42-1668-a</n1:XAddrs>"
        "        <n1:MetadataVersion>12</n1:MetadataVersion>"
        "      </n1:ProbeMatch>"
        "    </n1:ProbeMatches>"
        "  </soap:Body>"
        "</soap:Envelope>");
}

void testWSDiscoveryClient::testReceiveResolveMatch() 
{
    WSDiscoveryClient discoveryClient;
    discoveryClient.start();
    
    qRegisterMetaType<WSDiscoveryTargetService>();
    QSignalSpy spy(&discoveryClient, &WSDiscoveryClient::resolveMatchReceived);
    QVERIFY(spy.isValid());
    
    QUdpSocket testSocket;
    QVERIFY(testSocket.bind(QHostAddress::Any, 3702, QAbstractSocket::ShareAddress));
    QVERIFY(testSocket.joinMulticastGroup(QHostAddress(QStringLiteral("FF02::C"))));
    testSocket.writeDatagram(toBeSendResolveMatchData(), QHostAddress(QStringLiteral("FF02::C")), 3702);
    
    QVERIFY(spy.wait(1000));
    
    QCOMPARE(spy.count(), 1); // make sure the signal was emitted exactly one time
    QList<QVariant> arguments = spy.takeFirst(); // take the first signal
    
    const WSDiscoveryTargetService& probeMatchService = qvariant_cast<WSDiscoveryTargetService>(arguments.at(0));

    QCOMPARE(probeMatchService.endpointReference(), "Incomming_resolve_reference");
    QCOMPARE(probeMatchService.scopeList().size(), 1);
    QCOMPARE(probeMatchService.scopeList().at(0), QUrl(QStringLiteral("ldap:///ou=floor1,ou=b42,ou=anytown,o=examplecom,c=us")));
    QCOMPARE(probeMatchService.typeList().size(), 1);
    QCOMPARE(probeMatchService.typeList().at(0), KDQName(QStringLiteral("http://printer.example.org/2003/imaging"), QStringLiteral("PrintAdvanced")));
    QCOMPARE(probeMatchService.xAddrList().size(), 1);
    QCOMPARE(probeMatchService.xAddrList().at(0), QUrl(QStringLiteral("http://printer.local:8080")));
    QVERIFY(probeMatchService.lastSeen().msecsTo(QDateTime::currentDateTime()) < 500);
}

QByteArray testWSDiscoveryClient::toBeSendResolveMatchData()
{
    return QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<soap:Envelope"
        "  xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\""
        "  xmlns:soap-enc=\"http://www.w3.org/2003/05/soap-encoding\""
        "  xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
        "  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
        "  xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\""
        "  xmlns:n1=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
        "  <soap:Header>"
        "    <wsa:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>"
        "    <wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/ResolveMatches</wsa:Action>"
        "    <wsa:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</wsa:MessageID>"
        "    <wsa:RelatesTo>xs:anyURI</wsa:RelatesTo>"
        "    <n1:AppSequence InstanceId=\"12\" MessageNumber=\"13\"/>"
        "  </soap:Header>"
        "  <soap:Body>"
        "    <n1:ResolveMatches>"
        "      <n1:ResolveMatch>"
        "        <wsa:EndpointReference><wsa:Address>Incomming_resolve_reference</wsa:Address></wsa:EndpointReference>"
        "        <n1:Types xmlns:i=\"http://printer.example.org/2003/imaging\">i:PrintAdvanced</n1:Types>"
        "        <n1:Scopes>ldap:///ou=floor1,ou=b42,ou=anytown,o=examplecom,c=us</n1:Scopes>"
        "        <n1:XAddrs>http://printer.local:8080</n1:XAddrs>"
        "        <n1:MetadataVersion>12</n1:MetadataVersion>"
        "      </n1:ResolveMatch>"
        "    </n1:ResolveMatches>"
        "  </soap:Body>"
        "</soap:Envelope>");
}


QByteArray testWSDiscoveryClient::formatXml(const QByteArray& original)
{
    QByteArray xmlOut;

    QXmlStreamReader reader(original);
    QXmlStreamWriter writer(&xmlOut);
    writer.setAutoFormatting(true);

    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isWhitespace()) {
            writer.writeCurrentToken(reader);
        }
    }

    return xmlOut;
}

QByteArray testWSDiscoveryClient::zeroOutUuid(const QByteArray& original)
{
    QString originalString = original;
    originalString.replace(QRegExp("[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}"), QStringLiteral("00000000-0000-0000-0000-000000000000"));
    return originalString.toLatin1();
}

QTEST_MAIN(testWSDiscoveryClient)

#include "test_wsdiscoveryclient.moc"
