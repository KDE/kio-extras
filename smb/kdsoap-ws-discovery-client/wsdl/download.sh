#!/bin/bash
set -eu

# Copyright (C) 2019 Casper Meijn <casper@meijn.net>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# First download WS-Discovery 2005-04
wget --no-verbose -r http://schemas.xmlsoap.org/ws/2005/04/discovery/ws-discovery.xsd
wget --no-verbose -r http://schemas.xmlsoap.org/ws/2004/08/addressing
# Apperently the original file is not available anymore: http://schemas.xmlsoap.org/ws/2005/04/discovery/ws-discovery.wsdl
wget --no-verbose https://web.archive.org/web/20070225062601/http://schemas.xmlsoap.org/ws/2005/04/discovery/ws-discovery.wsdl
mv -v ws-discovery.wsdl schemas.xmlsoap.org/ws/2005/04/discovery/ws-discovery.wsdl

# Then download OASIS WS-Discovery 2009-01
wget --no-verbose -r http://docs.oasis-open.org/ws-dd/discovery/1.1/os/wsdd-discovery-1.1-wsdl-os.wsdl
wget --no-verbose -r http://docs.oasis-open.org/ws-dd/discovery/1.1/os/wsdd-discovery-1.1-schema-os.xsd
wget --no-verbose -r http://www.w3.org/2006/03/addressing/ws-addr.xsd

