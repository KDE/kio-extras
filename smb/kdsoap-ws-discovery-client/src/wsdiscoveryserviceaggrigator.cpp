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

#include "wsdiscoveryserviceaggrigator.h"
#include "wsdiscoveryserviceaggrigator_p.h"

WSDiscoveryServiceAggrigator::WSDiscoveryServiceAggrigator(QObject* parent)
    : QObject(parent),
    d_ptr(new WSDiscoveryServiceAggrigatorPrivate())
{
}

WSDiscoveryServiceAggrigator::~WSDiscoveryServiceAggrigator()
{
    delete d_ptr;
}

void WSDiscoveryServiceAggrigator::updateService(const WSDiscoveryTargetService& receivedService)
{
    Q_D(WSDiscoveryServiceAggrigator);
    auto endpointReference = receivedService.endpointReference();
    auto aggrigatedService = d->targetServiceMap.value(endpointReference);
    if(aggrigatedService.isNull()) {
        aggrigatedService = QSharedPointer<WSDiscoveryTargetService>::create(endpointReference);
        d->targetServiceMap.insert(endpointReference, aggrigatedService);
    }
    aggrigatedService->setTypeList(receivedService.typeList());
    aggrigatedService->setScopeList(receivedService.scopeList());
    aggrigatedService->setXAddrList(receivedService.xAddrList());
    aggrigatedService->setLastSeen(receivedService.lastSeen());;
    emit serviceUpdated(aggrigatedService);
}

