# KDSoap WS-Discovery client

This project is trying to create a WS-Discovery client library based on the KDSoap
library. It uses modern C++ 11 and Qt 5. The initial development is done for
[ONVIFViewer](https://gitlab.com/caspermeijn/onvifviewer), a ONVIF camera viewer.
However the library is designed to be useful as a generic WS-Discovery client.

## Current state

The library is not yet ready for production. It needs patches to KDSoap, most are
applied to [this tree](https://github.com/caspermeijn/KDSoap). It also needs some
internal KDSoap headers.

## Contributions

Contributions to the project are appreciated. See the
[issue tracker](https://gitlab.com/caspermeijn/kdsoap-ws-discovery-client/issues)
to open tasks and feel free to open a merge request for the changes you made.

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

To build this project you need a copy of KDSoap and have extra-cmake-modules installed.

```
$ sudo apt install extra-cmake-modules
$ git checkout https://github.com/caspermeijn/KDSoap.git
$ git checkout https://gitlab.com/caspermeijn/kdsoap-ws-discovery-client.git
$ cd kdsoap-ws-discovery-client && mkdir build && cd build
$ cmake .. && make && make install
```
