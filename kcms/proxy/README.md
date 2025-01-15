This file explains how the proxy settings architecture works.

# Overview

The Proxy KCM writes the settings into .config/kioslaverc

Qt/KDE applications use the [QNetworkProxy](https://doc.qt.io/qt-6/qnetworkproxy.html) API to obtain the proxy settings, preferrably via [QNetworkProxyFactory::proxyForQuery()](https://doc.qt.io/qt-6/qnetworkproxyfactory.html#proxyForQuery). This works automatically when using [QNetworkAccessManager](https://doc.qt.io/qt-6/qnetworkaccessmanager.html).

On Linux this will use [libproxy](https://github.com/libproxy/libproxy) under the hood, which on Plasma will read the settings from .config/kioslaverc. This requires Qt to be built with libproxy support. For this reason the settings in kioslaverc must be kept stable.

Third-party applications often rely on libproxy too, making them respect our proxy settings.

# Limitations

The HTTP and FTP workers also use QNetworkProxy to read the settings. Since the workers are running in a separate process from the application this means that any application-specific configuration set via Qt API is not followed and instead the system-wide settings are used.

# History
Prior to KF6 KIO had its own proxy infrastructure that was used by KIO workers and some applications. In KF6 this was dropped in favor of Qt/libproxy, which gives better cross-platform support.

