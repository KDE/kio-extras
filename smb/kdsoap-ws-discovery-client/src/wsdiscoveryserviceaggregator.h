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

#ifndef WSDISCOVERYSERVICEAGGREGATOR_H
#define WSDISCOVERYSERVICEAGGREGATOR_H

#include "wsdiscoveryclient_export.h"
#include <QObject>

class WSDiscoveryTargetService;
class WSDiscoveryServiceAggregatorPrivate;

/*!
 * \brief Aggregates multiple updates from the same WSDiscoveryTargetService
 *
 * When receiving multiple probe and resolve matches of the same service, this
 * class will aggregate those into a single shared instance. This eases the
 * administration needed by the application.
 */
class WSDISCOVERYCLIENT_EXPORT WSDiscoveryServiceAggregator : public QObject
{
    Q_OBJECT

public:
    /*!
     * Create a WSDiscoveryClient
     * \param parent The QObject parent
     */
    WSDiscoveryServiceAggregator(QObject* parent = nullptr);

    /*!
     * Destructor
     */
    ~WSDiscoveryServiceAggregator();

signals:
    /*!
     * Emitted when a service has new information. The service instance is 
     * reused, when a new update is received. Therefore you can compare the 
     * pointers when looking for updates of a previous received service.
     * \param updatedService A pointer to the updated service
     */
    void serviceUpdated(const QSharedPointer<WSDiscoveryTargetService>& updatedService);

public slots:
    /*!
     * Provides a new service update. 
     * \param receivedService The service with updated information
     */
    void updateService(const WSDiscoveryTargetService& receivedService);

private:
    WSDiscoveryServiceAggregatorPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(WSDiscoveryServiceAggregator)
};

#endif // WSDISCOVERYSERVICEAGGREGATOR_H
