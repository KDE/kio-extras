/* Copyright (C) 2019 Casper Meijn <casper@meijn.net>
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
#include "onvifdiscover.h"

#include <QDebug>
#include <QSharedPointer>
#include <WSDiscoveryClient>
#include <WSDiscoveryProbeJob>
#include <WSDiscoveryTargetService>
#include <WSDiscoveryServiceAggregator>

OnvifDiscover::OnvifDiscover(QObject *parent) : QObject(parent)
{
    m_client = new WSDiscoveryClient(this);

    m_probeJob = new WSDiscoveryProbeJob(m_client);
    
    m_aggregator = new WSDiscoveryServiceAggregator(this);
    
    connect(m_probeJob, &WSDiscoveryProbeJob::matchReceived, m_aggregator, &WSDiscoveryServiceAggregator::updateService);
    connect(m_aggregator, &WSDiscoveryServiceAggregator::serviceUpdated, this, &OnvifDiscover::matchReceived);
    
    KDQName type("tdn:NetworkVideoTransmitter");
    type.setNameSpace("http://www.onvif.org/ver10/network/wsdl");
    m_probeJob->addType(type);
}

OnvifDiscover::~OnvifDiscover() = default;

void OnvifDiscover::start()
{
    m_client->start();
    m_probeJob->start();
}

void OnvifDiscover::matchReceived(const QSharedPointer<WSDiscoveryTargetService> &matchedService)
{
    qDebug() << "ProbeMatch received:";
    qDebug() << "  Endpoint reference:" << matchedService->endpointReference();
    const auto& typeList = matchedService->typeList();
    for(const auto& type : typeList) {
        qDebug() << "  Type:"  << type.localName() << "in namespace" << type.nameSpace();
    }
    const auto& scopeList = matchedService->scopeList();
    for(const auto& scope : scopeList) {
        qDebug() << "  Scope:"  << scope.toString();
    }
    const auto& xAddrList = matchedService->xAddrList();
    for(const auto& xAddr : xAddrList) {
        qDebug() << "  XAddr:" << xAddr.toString();
    }
}
