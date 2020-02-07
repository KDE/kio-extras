/* Copyright (C) 2019 Casper Meijn <casper@meijn.net>
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

WSDiscoveryTargetService::WSDiscoveryTargetService(const QString &endpointReference) :
    m_endpointReference(endpointReference)
{
}

QDateTime WSDiscoveryTargetService::lastSeen() const
{
    return m_lastSeen;
}

void WSDiscoveryTargetService::setLastSeen(const QDateTime &lastSeen)
{
    m_lastSeen = lastSeen;
}

void WSDiscoveryTargetService::updateLastSeen()
{
    setLastSeen(QDateTime::currentDateTime());
}

bool WSDiscoveryTargetService::isMatchingType(const KDQName &matchingType)
{
    for(const KDQName &type : m_typeList) {
        if(matchingType.nameSpace() == type.nameSpace() &&
                matchingType.localName() == type.localName())
            return true;
    }
    return false;
}

bool WSDiscoveryTargetService::isMatchingScope(const QUrl &matchingScope)
{
    for(const QUrl &scope : m_scopeList) {
        if(matchingScope == scope)
            return true;
    }
    return false;
}

QList<QUrl> WSDiscoveryTargetService::xAddrList() const
{
    return m_xAddrList;
}

void WSDiscoveryTargetService::setXAddrList(const QList<QUrl> &xAddrList)
{
    m_xAddrList = xAddrList;
}

QList<QUrl> WSDiscoveryTargetService::scopeList() const
{
    return m_scopeList;
}

void WSDiscoveryTargetService::setScopeList(const QList<QUrl> &scopeList)
{
    m_scopeList = scopeList;
}

QList<KDQName> WSDiscoveryTargetService::typeList() const
{
    return m_typeList;
}

void WSDiscoveryTargetService::setTypeList(const QList<KDQName> &typeList)
{
    m_typeList = typeList;
}

QString WSDiscoveryTargetService::endpointReference() const
{
    return m_endpointReference;
}
