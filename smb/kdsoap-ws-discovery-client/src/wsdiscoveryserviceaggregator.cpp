/* Copyright (C) 2020  Casper Meijn <casper@meijn.net>
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

#include "wsdiscoveryserviceaggregator.h"
#include "wsdiscoveryserviceaggregator_p.h"

WSDiscoveryServiceAggregator::WSDiscoveryServiceAggregator(QObject* parent)
    : QObject(parent),
    d_ptr(new WSDiscoveryServiceAggregatorPrivate())
{
}

WSDiscoveryServiceAggregator::~WSDiscoveryServiceAggregator()
{
    delete d_ptr;
}

void WSDiscoveryServiceAggregator::updateService(const WSDiscoveryTargetService& receivedService)
{
    Q_D(WSDiscoveryServiceAggregator);
    auto endpointReference = receivedService.endpointReference();
    auto aggregatedService = d->targetServiceMap.value(endpointReference);
    if(aggregatedService.isNull()) {
        aggregatedService = QSharedPointer<WSDiscoveryTargetService>::create(endpointReference);
        d->targetServiceMap.insert(endpointReference, aggregatedService);
    }
    aggregatedService->setTypeList(receivedService.typeList());
    aggregatedService->setScopeList(receivedService.scopeList());
    aggregatedService->setXAddrList(receivedService.xAddrList());
    aggregatedService->setLastSeen(receivedService.lastSeen());;
    emit serviceUpdated(aggregatedService);
}

