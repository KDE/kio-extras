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
#include "wsdiscoveryprobejob.h"

#include "loggingcategory.h"
#include "wsdiscoveryclient.h"
#include "wsdiscoverytargetservice.h"
#include <QDebug>
#include <QSharedPointer>

WSDiscoveryProbeJob::WSDiscoveryProbeJob(WSDiscoveryClient *parent) :
    QObject(parent),
    m_client(parent)
{
    connect(m_client, &WSDiscoveryClient::probeMatchReceived, this, &WSDiscoveryProbeJob::probeMatchReceived);

    m_timer.setInterval(30000);
    connect(&m_timer, &QTimer::timeout, this, &WSDiscoveryProbeJob::timeout);
}

QList<KDQName> WSDiscoveryProbeJob::typeList() const
{
    return m_typeList;
}

void WSDiscoveryProbeJob::setTypeList(const QList<KDQName> &typeList)
{
    m_typeList = typeList;
}

void WSDiscoveryProbeJob::addType(const KDQName &type)
{
    m_typeList.append(type);
}

QList<QUrl> WSDiscoveryProbeJob::scopeList() const
{
    return m_scopeList;
}

void WSDiscoveryProbeJob::setScopeList(const QList<QUrl> &scopeList)
{
    m_scopeList = scopeList;
}

void WSDiscoveryProbeJob::addScope(const QUrl &scope)
{
    m_scopeList.append(scope);
}

int WSDiscoveryProbeJob::interval() const
{
    return m_timer.interval();
}

void WSDiscoveryProbeJob::setInterval(int interval)
{
    m_timer.setInterval(interval);
}

void WSDiscoveryProbeJob::start()
{
    timeout();
    m_timer.start();
}

void WSDiscoveryProbeJob::stop()
{
    m_timer.stop();
}

void WSDiscoveryProbeJob::timeout()
{
    m_client->sendProbe(m_typeList, m_scopeList);
}

void WSDiscoveryProbeJob::probeMatchReceived(const WSDiscoveryTargetService &probeMatchService)
{
    bool isMatch = true;
    for(const KDQName& type : qAsConst(m_typeList)) {
        isMatch = probeMatchService.isMatchingType(type) && isMatch;
    }
    for(const QUrl& scope : qAsConst(m_scopeList)) {
        isMatch = probeMatchService.isMatchingScope(scope) && isMatch;
    }
    if(isMatch) {
        emit matchReceived(probeMatchService);
    } else {
        qCDebug(KDSoapWSDiscoveryClient) << "Received probe match that didn't match the probe job";
    }
}
