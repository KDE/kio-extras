<!--
Copyright (C) 2019-2020 Casper Meijn <casper@meijn.net>

SPDX-License-Identifier: GPL-3.0-or-later
-->

# KDSoap WS-Discovery client                {#mainpage}

This project is trying to create a WS-Discovery client library based on the KDSoap
library. It uses modern C++ 11 and Qt 5. The initial development is done for
[ONVIFViewer](https://gitlab.com/caspermeijn/onvifviewer), a ONVIF camera viewer.
However the library is designed to be useful as a generic WS-Discovery client.

## Current state

The library is not yet ready for production. There are some API changes planned and it needs [KDSoap](https://github.com/KDAB/KDSoap) 1.9.0. The [WS-Discovery 2005-04](http://schemas.xmlsoap.org/ws/2005/04/discovery/) standard is specification, but only the Probe and Resolve messages are available.

## Contributions

Contributions to the project are appreciated. See the
[issue tracker](https://gitlab.com/caspermeijn/kdsoap-ws-discovery-client/issues)
for open tasks and feel free to open a merge request for the changes you made.

Compatibility testing with devices you own is also useful. These could be ONVIF
cameras, printers or other WS-Discovery devices. Open an issue in the
[issue tracker](https://gitlab.com/caspermeijn/kdsoap-ws-discovery-client/issues)
to report your test result (good and bad result are both welcome).

## Example

The onif-discover example will send out a Probe message for ONVIF devices and will list the properties of the responding devices.

``` 
$ onvif-discover
Starting ONVIF discovery for 5 seconds
ProbeMatch received:
  Endpoint reference: "urn:uuid:5f5a69c2-e0ae-504f-829b-00AFA538AC17"
  Type: "NetworkVideoTransmitter" in namespace "http://www.onvif.org/ver10/network/wsdl"
  Scope: "onvif://www.onvif.org/Profile/Streaming"
  Scope: "onvif://www.onvif.org/model/C6F0SiZ0N0P0L0"
  Scope: "onvif://www.onvif.org/name/IPCAM"
  Scope: "onvif://www.onvif.org/location/country/china"
  XAddr: "http://192.168.128.248:8080/onvif/devices"
```

## Building

To build this project you need to have extra-cmake-modules and the master branch of KDSoap installed.

```
$ sudo apt install extra-cmake-modules
$ git checkout https://github.com/KDAB/KDSoap.git
$ cd KDSoap && mkdir build && cd build
$ cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local && make && make install
$ git checkout https://gitlab.com/caspermeijn/kdsoap-ws-discovery-client.git
$ cd kdsoap-ws-discovery-client && mkdir build && cd build
$ cmake .. && make && make install
```
