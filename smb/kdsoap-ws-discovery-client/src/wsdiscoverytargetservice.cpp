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
#include "wsdiscoverytargetservice.h"

class WSDiscoveryTargetServiceData : public QSharedData
{
  public:
    QString endpointReference;
    QList<KDQName> typeList;
    QList<QUrl> scopeList;
    QList<QUrl> xAddrList;
    QDateTime lastSeen;
};

WSDiscoveryTargetService::WSDiscoveryTargetService()
{
    d = new WSDiscoveryTargetServiceData();
}

WSDiscoveryTargetService::WSDiscoveryTargetService(const QString &endpointReference)
{
    d = new WSDiscoveryTargetServiceData();
    d->endpointReference = endpointReference;
}

WSDiscoveryTargetService::WSDiscoveryTargetService(const WSDiscoveryTargetService &other) = default;
WSDiscoveryTargetService::~WSDiscoveryTargetService() = default;

QDateTime WSDiscoveryTargetService::lastSeen() const
{
    return d->lastSeen;
}

void WSDiscoveryTargetService::setLastSeen(const QDateTime &lastSeen)
{
    d->lastSeen = lastSeen;
}

void WSDiscoveryTargetService::updateLastSeen()
{
    setLastSeen(QDateTime::currentDateTime());
}

bool WSDiscoveryTargetService::isMatchingType(const KDQName &matchingType) const
{
    for(const KDQName &type : d->typeList) {
        if(matchingType.nameSpace() == type.nameSpace() &&
                matchingType.localName() == type.localName()) {
            return true;
        }
    }
    return false;
}

bool WSDiscoveryTargetService::isMatchingScope(const QUrl &matchingScope) const
{
    for(const QUrl &scope : d->scopeList) {
        if(matchingScope == scope) {
            return true;
        }
    }
    return false;
}

QList<QUrl> WSDiscoveryTargetService::xAddrList() const
{
    return d->xAddrList;
}

void WSDiscoveryTargetService::setXAddrList(const QList<QUrl> &xAddrList)
{
    d->xAddrList = xAddrList;
}

QList<QUrl> WSDiscoveryTargetService::scopeList() const
{
    return d->scopeList;
}

void WSDiscoveryTargetService::setScopeList(const QList<QUrl> &scopeList)
{
    d->scopeList = scopeList;
}

QList<KDQName> WSDiscoveryTargetService::typeList() const
{
    return d->typeList;
}

void WSDiscoveryTargetService::setTypeList(const QList<KDQName> &typeList)
{
    d->typeList = typeList;
}

QString WSDiscoveryTargetService::endpointReference() const
{
    return d->endpointReference;
}

void WSDiscoveryTargetService::setEndpointReference(const QString& endpointReference)
{
    d->endpointReference = endpointReference;
}

