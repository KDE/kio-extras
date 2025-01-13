/*
    kproxydlg.h - Proxy configuration dialog
    SPDX-FileCopyrightText: 2001, 2011 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KPROXYDLG_H
#define KPROXYDLG_H

#include "ui_kproxydlg.h"

#include <KCModule>

#include "kioslave.h"

class KProxyDialog : public KCModule
{
    Q_OBJECT

public:
    enum DisplayUrlFlag {
        HideNone = 0x00,
        HideHttpUrlScheme = 0x01,
        HideHttpsUrlScheme = 0x02,
        HideFtpUrlScheme = 0x04,
        HideSocksUrlScheme = 0x08,
    };
    Q_DECLARE_FLAGS(DisplayUrlFlags, DisplayUrlFlag)

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

    KProxyDialog(QObject *parent, const KPluginMetaData &data);
    ~KProxyDialog() override;

    void load() override;
    void save() override;
    void defaults() override;

private Q_SLOTS:
    void autoDetect();
    void showEnvValue(bool);
    void setUseSameProxy(bool);
    void syncProxies(const QString &);
    void syncProxyPorts(int);

    void slotChanged();

private:
    bool autoDetectSystemProxy(QLineEdit *edit, const QString &envVarStr, bool showValue);
    QString parseProxyString(const QString &protocol) const;
    ProxyType proxyType() const;
    void setProxyInformation(const QString &value,
                             KProxyDialog::ProxyType proxyType,
                             QLineEdit *manEdit,
                             QLineEdit *sysEdit,
                             QSpinBox *spinBox,
                             const QString &defaultScheme,
                             KProxyDialog::DisplayUrlFlag flag);

    Ui::ProxyDialogUI mUi;
    QStringList mNoProxyForList;
    QMap<QString, QString> mProxyMap;

    ProxySettings mSettings;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KProxyDialog::DisplayUrlFlags)

#endif // KPROXYDLG_H
