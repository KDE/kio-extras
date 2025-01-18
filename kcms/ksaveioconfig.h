/*
    SPDX-FileCopyrightText: 2001 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSAVEIO_CONFIG_H_
#define KSAVEIO_CONFIG_H_

#include <QString>

class QWidget;

namespace KSaveIOConfig
{

/*
 * Types of proxy configuration
 * @li NoProxy     - No proxy is used
 * @li ManualProxy - Proxies are manually configured
 * @li PACProxy    - A Proxy configuration URL has been given
 * @li WPADProxy   - A proxy should be automatically discovered
 * @li EnvVarProxy - Use the proxy values set through environment variables.
 */
enum ProxyType {
    NoProxy,
    ManualProxy,
    PACProxy,
    WPADProxy,
    EnvVarProxy,
};

int proxyDisplayUrlFlags();
void setProxyDisplayUrlFlags(int);

/** Proxy Settings */
void setUseReverseProxy(bool);

void setProxyType(KSaveIOConfig::ProxyType);

void setProxyConfigScript(const QString &);

void setProxyFor(const QString &, const QString &);

QString noProxyFor();
void setNoProxyFor(const QString &);

/** Miscellaneous Settings */
void setMarkPartial(bool);

void setMinimumKeepSize(int);

/** Update all running KIO workers */
void updateRunningWorkers(QWidget *parent = nullptr);
}

#endif
