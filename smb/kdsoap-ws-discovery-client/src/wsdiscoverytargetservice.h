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
#ifndef WSDISCOVERYTARGETSERVICE_H
#define WSDISCOVERYTARGETSERVICE_H

#include "wsdiscoveryclient_export.h"
#include <KDSoapClient/KDQName>
#include <QDateTime>
#include <QUrl>

class WSDISCOVERYCLIENT_EXPORT WSDiscoveryTargetService
{
public:
    explicit WSDiscoveryTargetService(const QString &endpointReference);

    QString endpointReference() const;
    QList<KDQName> typeList() const;
    void setTypeList(const QList<KDQName> &typeList);
    QList<QUrl> scopeList() const;
    void setScopeList(const QList<QUrl> &scopeList);
    QList<QUrl> xAddrList() const;
    void setXAddrList(const QList<QUrl> &xAddrList);
    QDateTime lastSeen() const;
    void setLastSeen(const QDateTime &lastSeen);
    void updateLastSeen();

    bool isMatchingType(const KDQName &matchingType);
    bool isMatchingScope(const QUrl &matchingScope);

private:
    QString m_endpointReference;
    QList<KDQName> m_typeList;
    QList<QUrl> m_scopeList;
    QList<QUrl> m_xAddrList;
    QDateTime m_lastSeen;
};

#endif // WSDISCOVERYTARGETSERVICE_H
