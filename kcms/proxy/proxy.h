/*
    kproxydlg.h - Proxy configuration dialog
    SPDX-FileCopyrightText: 2001, 2011 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KQuickManagedConfigModule>

#include "kioslave.h"

class Proxy : public KQuickManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(ProxySettings *proxySettings READ proxySettings CONSTANT)
    Q_PROPERTY(QByteArrayList httpEnvs MEMBER m_httpEnvs CONSTANT)
    Q_PROPERTY(QByteArrayList httpsEnvs MEMBER m_httpsEnvs CONSTANT)
    Q_PROPERTY(QByteArrayList ftpEnvs MEMBER m_ftpEnvs CONSTANT)
    Q_PROPERTY(QByteArrayList socksEnvs MEMBER m_socksEnvs CONSTANT)
    Q_PROPERTY(QByteArrayList noProxyEnvs MEMBER m_noProxyEnvs CONSTANT)

public:
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
    Q_ENUM(ProxyType);

    Proxy(QObject *parent, const KPluginMetaData &data);

    void save() override;
    [[nodiscard]] ProxySettings *proxySettings();

    Q_INVOKABLE QString autoDetect(const QByteArrayList &envVars) const;
    Q_INVOKABLE QString autoDetectResolve(const QByteArrayList &envVars) const;

private:
    ProxySettings *m_settings;
    const QByteArrayList m_httpEnvs{QByteArrayLiteral("HTTP_PROXY"),
                                    QByteArrayLiteral("http_proxy"),
                                    QByteArrayLiteral("HTTPPROXY"),
                                    QByteArrayLiteral("httpproxy"),
                                    QByteArrayLiteral("PROXY"),
                                    QByteArrayLiteral("proxy")};
    const QByteArrayList m_httpsEnvs{QByteArrayLiteral("HTTPS_PROXY"),
                                     QByteArrayLiteral("https_proxy"),
                                     QByteArrayLiteral("HTTPSPROXY"),
                                     QByteArrayLiteral("httpsproxy"),
                                     QByteArrayLiteral("PROXY"),
                                     QByteArrayLiteral("proxy")};
    const QByteArrayList m_ftpEnvs{QByteArrayLiteral("FTP_PROXY"),
                                   QByteArrayLiteral("ftp_proxy"),
                                   QByteArrayLiteral("FTPPROXY"),
                                   QByteArrayLiteral("ftpproxy"),
                                   QByteArrayLiteral("PROXY"),
                                   QByteArrayLiteral("proxy")};
    const QByteArrayList m_socksEnvs{QByteArrayLiteral("SOCKS_PROXY"),
                                     QByteArrayLiteral("socks_proxy"),
                                     QByteArrayLiteral("SOCKSPROXY"),
                                     QByteArrayLiteral("socksproxy"),
                                     QByteArrayLiteral("PROXY"),
                                     QByteArrayLiteral("proxy")};
    const QByteArrayList m_noProxyEnvs{QByteArrayLiteral("NO_PROXY"), QByteArrayLiteral("no_proxy")};
};
