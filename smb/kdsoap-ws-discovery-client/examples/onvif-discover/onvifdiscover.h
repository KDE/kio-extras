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
#ifndef ONVIFDISCOVER_H
#define ONVIFDISCOVER_H

#include <QObject>

class WSDiscoveryClient;
class WSDiscoveryProbeJob;
class WSDiscoveryTargetService;
class WSDiscoveryServiceAggregator;

class OnvifDiscover : public QObject
{
    Q_OBJECT
public:
    explicit OnvifDiscover(QObject *parent = nullptr);
    ~OnvifDiscover();

public slots:
    void start();

private slots:
    void matchReceived(const QSharedPointer<WSDiscoveryTargetService>& matchedService);

private:
    WSDiscoveryClient * m_client;
    WSDiscoveryProbeJob * m_probeJob;
    WSDiscoveryServiceAggregator * m_aggregator;
};

#endif // ONVIFDISCOVER_H
