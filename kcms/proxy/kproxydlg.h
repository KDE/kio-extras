/*
    kproxydlg.h - Proxy configuration dialog
    SPDX-FileCopyrightText: 2001, 2011 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KPROXYDLG_H
#define KPROXYDLG_H

#include "ui_kproxydlg.h"
#include <KCModule>
#include <KSharedConfig>

#include "../ksaveioconfig.h"

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
    QString proxyFor(const QString &protocol) const;
    QString proxyConfigScript() const;
    bool useReverseProxy() const;
    KSaveIOConfig::ProxyType proxyType() const;

    Ui::ProxyDialogUI mUi;
    QStringList mNoProxyForList;
    QMap<QString, QString> mProxyMap;
    KSharedConfig::Ptr mConfig;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KProxyDialog::DisplayUrlFlags)

#endif // KPROXYDLG_H
