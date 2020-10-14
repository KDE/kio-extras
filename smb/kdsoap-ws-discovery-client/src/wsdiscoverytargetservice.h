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
#ifndef WSDISCOVERYTARGETSERVICE_H
#define WSDISCOVERYTARGETSERVICE_H

#include "wsdiscoveryclient_export.h"
#include <KDSoapClient/KDQName>
#include <QDateTime>
#include <QUrl>

class WSDiscoveryTargetServiceData;

class WSDISCOVERYCLIENT_EXPORT WSDiscoveryTargetService
{
public:
    explicit WSDiscoveryTargetService();
    explicit WSDiscoveryTargetService(const QString &endpointReference);
    WSDiscoveryTargetService(const WSDiscoveryTargetService &other);
    ~WSDiscoveryTargetService();

    void setEndpointReference(const QString &endpointReference);
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

    bool isMatchingType(const KDQName &matchingType) const;
    bool isMatchingScope(const QUrl &matchingScope) const;

private:
    QSharedDataPointer<WSDiscoveryTargetServiceData> d;
};

#endif // WSDISCOVERYTARGETSERVICE_H
