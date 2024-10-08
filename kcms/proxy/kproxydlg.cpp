/*
    kproxydlg.cpp - Proxy configuration dialog
    SPDX-FileCopyrightText: 2001, 2011 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

// Own
#include "kproxydlg.h"

// Local
#include "../ksaveioconfig.h"

// KDE
#include <KConfigGroup>
#include <KLineEdit> // Needed for KUrlRequester::lineEdit()
#include <KLocalizedString>
#include <KPluginFactory>
#include <kurifilter.h>

// Qt
#include <QSpinBox>
#include <QUrl>

K_PLUGIN_CLASS_WITH_JSON(KProxyDialog, "kcm_proxy.json")

class InputValidator : public QValidator
{
public:
    State validate(QString &input, int &pos) const override
    {
        if (input.isEmpty()) {
            return Acceptable;
        }

        const QChar ch = input.at((pos > 0 ? pos - 1 : pos));
        if (ch.isSpace()) {
            return Invalid;
        }

        return Acceptable;
    }
};

static QString manualProxyToText(const QLineEdit *edit, const QSpinBox *spinBox, const QChar &separator)
{
    const QString value = edit->text() + separator + QString::number(spinBox->value());

    return value;
}

static void setManualProxyFromText(const QString &value, QLineEdit *edit, QSpinBox *spinBox)
{
    if (value.isEmpty()) {
        return;
    }

    const QStringList values = value.split(QLatin1Char(' '));
    edit->setText(values.at(0));
    bool ok = false;
    const int num = values.at(1).toInt(&ok);
    if (ok) {
        spinBox->setValue(num);
    }
}

static void showSystemProxyUrl(QLineEdit *edit, QString *value)
{
    Q_ASSERT(edit);
    Q_ASSERT(value);

    *value = edit->text();
    edit->setEnabled(false);
    const QByteArray envVar(edit->text().toUtf8());
    edit->setText(QString::fromUtf8(qgetenv(envVar.constData())));
}

static QString proxyUrlFromInput(KProxyDialog::DisplayUrlFlags *flags,
                                 const QLineEdit *edit,
                                 const QSpinBox *spinBox,
                                 const QString &defaultScheme = QString(),
                                 KProxyDialog::DisplayUrlFlag flag = KProxyDialog::HideNone)
{
    Q_ASSERT(edit);
    Q_ASSERT(spinBox);

    QString proxyStr;

    if (edit->text().isEmpty()) {
        return proxyStr;
    }

    if (flags && !edit->text().contains(QLatin1String("://"))) {
        *flags |= flag;
    }

    KUriFilterData data;
    data.setData(edit->text());
    data.setCheckForExecutables(false);
    if (!defaultScheme.isEmpty()) {
        data.setDefaultUrlScheme(defaultScheme);
    }

    if (KUriFilter::self()->filterUri(data, QStringList{QStringLiteral("kshorturifilter")})) {
        QUrl url = data.uri();
        const int portNum = (spinBox->value() > 0 ? spinBox->value() : url.port());
        url.setPort(-1);

        proxyStr = url.url();
        if (portNum > -1) {
            proxyStr += QLatin1Char(' ') + QString::number(portNum);
        }
    } else {
        proxyStr = edit->text();
        if (spinBox->value() > 0) {
            proxyStr += QLatin1Char(' ') + QString::number(spinBox->value());
        }
    }

    return proxyStr;
}

static void setProxyInformation(const QString &value,
                                KSaveIOConfig::ProxyType proxyType,
                                QLineEdit *manEdit,
                                QLineEdit *sysEdit,
                                QSpinBox *spinBox,
                                const QString &defaultScheme,
                                KProxyDialog::DisplayUrlFlag flag)
{
    const bool isSysProxy =
        !value.contains(QLatin1Char(' ')) && !value.contains(QLatin1Char('.')) && !value.contains(QLatin1Char(',')) && !value.contains(QLatin1Char(':'));

    if (proxyType == KSaveIOConfig::EnvVarProxy || isSysProxy) {
#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
        sysEdit->setText(value);
#endif
        return;
    }

    if (spinBox) {
        KUriFilterData data;
        data.setData(value);
        data.setCheckForExecutables(false);
        if (!defaultScheme.isEmpty()) {
            data.setDefaultUrlScheme(defaultScheme);
        }

        QUrl url;
        if (KUriFilter::self()->filterUri(data, QStringList{QStringLiteral("kshorturifilter")})) {
            url = QUrl(data.uri());
            url.setUserName(QString());
            url.setPassword(QString());
            url.setPath(QString());
        } else {
            url = QUrl(value);
        }

        if (url.port() > -1) {
            spinBox->setValue(url.port());
        }
        url.setPort(-1);
        manEdit->setText((KSaveIOConfig::proxyDisplayUrlFlags() & flag) ? url.host() : url.url());
        return;
    }

    manEdit->setText(value); // Manual proxy exception...
}

KProxyDialog::KProxyDialog(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
{
    mConfig = KSharedConfig::openConfig(QStringLiteral("kioslaverc"), KConfig::NoGlobals);

    mUi.setupUi(widget());

    connect(mUi.autoDetectButton, &QPushButton::clicked, this, &KProxyDialog::autoDetect);
    connect(mUi.showEnvValueCheckBox, &QAbstractButton::toggled, this, &KProxyDialog::showEnvValue);
    connect(mUi.useSameProxyCheckBox, &QPushButton::clicked, this, &KProxyDialog::setUseSameProxy);
    connect(mUi.manualProxyHttpEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        mUi.useSameProxyCheckBox->setEnabled(!text.isEmpty());
    });
    connect(mUi.manualNoProxyEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        mUi.useReverseProxyCheckBox->setEnabled(!text.isEmpty());
    });
    connect(mUi.manualProxyHttpEdit, &QLineEdit::textEdited, this, &KProxyDialog::syncProxies);
    connect(mUi.manualProxyHttpSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &KProxyDialog::syncProxyPorts);

    mUi.systemProxyGroupBox->setVisible(false);
    mUi.manualProxyGroupBox->setVisible(false);
    mUi.autoDetectButton->setVisible(false);
    mUi.proxyConfigScriptGroupBox->setVisible(false);

    InputValidator *v = new InputValidator;
    mUi.proxyScriptUrlRequester->lineEdit()->setValidator(v);
    mUi.manualProxyHttpEdit->setValidator(v);
    mUi.manualProxyHttpsEdit->setValidator(v);
    mUi.manualProxyFtpEdit->setValidator(v);
    mUi.manualProxySocksEdit->setValidator(v);
    mUi.manualNoProxyEdit->setValidator(v);

    // Signals and slots connections
    connect(mUi.noProxyRadioButton, &QPushButton::clicked, this, &KProxyDialog::slotChanged);
    connect(mUi.autoDiscoverProxyRadioButton, &QPushButton::clicked, this, &KProxyDialog::slotChanged);
    connect(mUi.autoScriptProxyRadioButton, &QPushButton::clicked, this, &KProxyDialog::slotChanged);
    connect(mUi.manualProxyRadioButton, &QPushButton::clicked, this, &KProxyDialog::slotChanged);
    connect(mUi.noProxyRadioButton, &QPushButton::clicked, this, &KProxyDialog::slotChanged);
    connect(mUi.useReverseProxyCheckBox, &QPushButton::clicked, this, &KProxyDialog::slotChanged);
    connect(mUi.useSameProxyCheckBox, &QPushButton::clicked, this, &KProxyDialog::slotChanged);

    connect(mUi.proxyScriptUrlRequester, &KUrlRequester::textChanged, this, &KProxyDialog::slotChanged);

    connect(mUi.manualProxyHttpEdit, &QLineEdit::textChanged, this, &KProxyDialog::slotChanged);
    connect(mUi.manualProxyHttpsEdit, &QLineEdit::textChanged, this, &KProxyDialog::slotChanged);
    connect(mUi.manualProxyFtpEdit, &QLineEdit::textChanged, this, &KProxyDialog::slotChanged);
    connect(mUi.manualProxySocksEdit, &QLineEdit::textChanged, this, &KProxyDialog::slotChanged);
    connect(mUi.manualNoProxyEdit, &QLineEdit::textChanged, this, &KProxyDialog::slotChanged);

    connect(mUi.manualProxyHttpSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &KProxyDialog::slotChanged);
    connect(mUi.manualProxyHttpsSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &KProxyDialog::slotChanged);
    connect(mUi.manualProxyFtpSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &KProxyDialog::slotChanged);
    connect(mUi.manualProxySocksSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &KProxyDialog::slotChanged);

    connect(mUi.systemProxyHttpEdit, &QLineEdit::textEdited, this, &KProxyDialog::slotChanged);
    connect(mUi.systemProxyHttpsEdit, &QLineEdit::textEdited, this, &KProxyDialog::slotChanged);
    connect(mUi.systemProxyFtpEdit, &QLineEdit::textEdited, this, &KProxyDialog::slotChanged);
    connect(mUi.systemProxySocksEdit, &QLineEdit::textEdited, this, &KProxyDialog::slotChanged);
    connect(mUi.systemNoProxyEdit, &QLineEdit::textEdited, this, &KProxyDialog::slotChanged);

#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
    connect(mUi.systemProxyRadioButton, &QAbstractButton::toggled, mUi.systemProxyGroupBox, &QWidget::setVisible);
#else
    mUi.autoDetectButton->setVisible(false);
#endif
    connect(mUi.systemProxyRadioButton, &QPushButton::clicked, this, &KProxyDialog::slotChanged);
}

KProxyDialog::~KProxyDialog()
{
}

QString KProxyDialog::proxyFor(const QString &protocol) const
{
    const QString key = protocol + QLatin1String("Proxy");
    QString proxyStr(mConfig->group(QStringLiteral("Proxy Settings")).readEntry(key));
    const int index = proxyStr.lastIndexOf(QLatin1Char(' '));

    if (index > -1) {
        const QStringView portStr = QStringView(proxyStr).right(proxyStr.length() - index - 1);
        const bool isDigits = std::all_of(portStr.cbegin(), portStr.cend(), [](const QChar c) {
            return c.isDigit();
        });

        if (isDigits) {
            proxyStr = QStringView(proxyStr).left(index).toString() + QLatin1Char(':') + portStr;
        } else {
            proxyStr.clear();
        }
    }

    return proxyStr;
}

QString KProxyDialog::proxyConfigScript() const
{
    return mConfig->group(QStringLiteral("Proxy Settings")).readEntry("Proxy Config Script");
}

bool KProxyDialog::useReverseProxy() const
{
    return mConfig->group(QStringLiteral("Proxy Settings")).readEntry("ReversedException", false);
}

KSaveIOConfig::ProxyType KProxyDialog::proxyType() const
{
    return static_cast<KSaveIOConfig::ProxyType>(mConfig->group(QStringLiteral("Proxy Settings")).readEntry("ProxyType", 0));
}

void KProxyDialog::load()
{
    mProxyMap.insert(QStringLiteral("HttpProxy"), proxyFor(QStringLiteral("http")));
    mProxyMap.insert(QStringLiteral("HttpsProxy"), proxyFor(QStringLiteral("https")));
    mProxyMap.insert(QStringLiteral("FtpProxy"), proxyFor(QStringLiteral("ftp")));
    mProxyMap.insert(QStringLiteral("SocksProxy"), proxyFor(QStringLiteral("socks")));
    mProxyMap.insert(QStringLiteral("ProxyScript"), proxyConfigScript());
    mProxyMap.insert(QStringLiteral("NoProxy"), KSaveIOConfig::noProxyFor());

    const KSaveIOConfig::ProxyType type = proxyType();

    // Make sure showEnvValueCheckBox is unchecked before setting proxy env var names
    mUi.showEnvValueCheckBox->setChecked(false);

    setProxyInformation(mProxyMap.value(QStringLiteral("HttpProxy")),
                        type,
                        mUi.manualProxyHttpEdit,
                        mUi.systemProxyHttpEdit,
                        mUi.manualProxyHttpSpinBox,
                        QStringLiteral("http"),
                        HideHttpUrlScheme);
    setProxyInformation(mProxyMap.value(QStringLiteral("HttpsProxy")),
                        type,
                        mUi.manualProxyHttpsEdit,
                        mUi.systemProxyHttpsEdit,
                        mUi.manualProxyHttpsSpinBox,
                        QStringLiteral("http"),
                        HideHttpsUrlScheme);
    setProxyInformation(mProxyMap.value(QStringLiteral("FtpProxy")),
                        type,
                        mUi.manualProxyFtpEdit,
                        mUi.systemProxyFtpEdit,
                        mUi.manualProxyFtpSpinBox,
                        QStringLiteral("ftp"),
                        HideFtpUrlScheme);
    setProxyInformation(mProxyMap.value(QStringLiteral("SocksProxy")),
                        type,
                        mUi.manualProxySocksEdit,
                        mUi.systemProxySocksEdit,
                        mUi.manualProxySocksSpinBox,
                        QStringLiteral("socks"),
                        HideSocksUrlScheme);
    setProxyInformation(mProxyMap.value(QStringLiteral("NoProxy")), type, mUi.manualNoProxyEdit, mUi.systemNoProxyEdit, nullptr, QString(), HideNone);

    // Check the "Use this proxy server for all protocols" if all the proxy URLs are the same...
    const QString httpProxy(mUi.manualProxyHttpEdit->text());
    if (!httpProxy.isEmpty()) {
        const int httpProxyPort = mUi.manualProxyHttpSpinBox->value();
        mUi.useSameProxyCheckBox->setChecked(httpProxy == mUi.manualProxyHttpsEdit->text() /* clang-format off */
                                             && httpProxy == mUi.manualProxyFtpEdit->text()
                                             && httpProxy == mUi.manualProxySocksEdit->text()
                                             && httpProxyPort == mUi.manualProxyHttpsSpinBox->value()
                                             && httpProxyPort == mUi.manualProxyFtpSpinBox->value()
                                             && httpProxyPort == mUi.manualProxySocksSpinBox->value()); /* clang-format on */
    }

    // Validate and Set the automatic proxy configuration script url.
    QUrl u(mProxyMap.value(QStringLiteral("ProxyScript")));
    if (u.isValid() && !u.isEmpty()) {
        u.setUserName(QString());
        u.setPassword(QString());
        mUi.proxyScriptUrlRequester->setUrl(u);
    }

    // Set use reverse proxy checkbox...
    mUi.useReverseProxyCheckBox->setChecked((!mProxyMap.value(QStringLiteral("NoProxy")).isEmpty() && useReverseProxy()));

    switch (type) {
    case KSaveIOConfig::WPADProxy:
        mUi.autoDiscoverProxyRadioButton->setChecked(true);
        break;
    case KSaveIOConfig::PACProxy:
        mUi.autoScriptProxyRadioButton->setChecked(true);
        break;
    case KSaveIOConfig::ManualProxy:
        mUi.manualProxyRadioButton->setChecked(true);
        break;
    case KSaveIOConfig::EnvVarProxy:
        mUi.systemProxyRadioButton->setChecked(true);
        break;
    case KSaveIOConfig::NoProxy:
    default:
        mUi.noProxyRadioButton->setChecked(true);
        break;
    }
}

void KProxyDialog::save()
{
    KSaveIOConfig::ProxyType proxyType = KSaveIOConfig::NoProxy;
    DisplayUrlFlags displayUrlFlags = static_cast<DisplayUrlFlags>(KSaveIOConfig::proxyDisplayUrlFlags());

    if (mUi.manualProxyRadioButton->isChecked()) {
        DisplayUrlFlags flags = HideNone;
        proxyType = KSaveIOConfig::ManualProxy;
        mProxyMap[QStringLiteral("HttpProxy")] =
            proxyUrlFromInput(&flags, mUi.manualProxyHttpEdit, mUi.manualProxyHttpSpinBox, QStringLiteral("http"), HideHttpUrlScheme);
        mProxyMap[QStringLiteral("HttpsProxy")] =
            proxyUrlFromInput(&flags, mUi.manualProxyHttpsEdit, mUi.manualProxyHttpsSpinBox, QStringLiteral("http"), HideHttpsUrlScheme);
        mProxyMap[QStringLiteral("FtpProxy")] =
            proxyUrlFromInput(&flags, mUi.manualProxyFtpEdit, mUi.manualProxyFtpSpinBox, QStringLiteral("ftp"), HideFtpUrlScheme);
        mProxyMap[QStringLiteral("SocksProxy")] =
            proxyUrlFromInput(&flags, mUi.manualProxySocksEdit, mUi.manualProxySocksSpinBox, QStringLiteral("socks"), HideSocksUrlScheme);
        mProxyMap[QStringLiteral("NoProxy")] = mUi.manualNoProxyEdit->text();
        displayUrlFlags = flags;
    } else if (mUi.systemProxyRadioButton->isChecked()) {
        proxyType = KSaveIOConfig::EnvVarProxy;
        if (!mUi.showEnvValueCheckBox->isChecked()) {
            mProxyMap[QStringLiteral("HttpProxy")] = mUi.systemProxyHttpEdit->text();
            mProxyMap[QStringLiteral("HttpsProxy")] = mUi.systemProxyHttpsEdit->text();
            mProxyMap[QStringLiteral("FtpProxy")] = mUi.systemProxyFtpEdit->text();
            mProxyMap[QStringLiteral("SocksProxy")] = mUi.systemProxySocksEdit->text();
            mProxyMap[QStringLiteral("NoProxy")] = mUi.systemNoProxyEdit->text();
        } else {
            mProxyMap[QStringLiteral("HttpProxy")] = mProxyMap.take(mUi.systemProxyHttpEdit->objectName());
            mProxyMap[QStringLiteral("HttpsProxy")] = mProxyMap.take(mUi.systemProxyHttpsEdit->objectName());
            mProxyMap[QStringLiteral("FtpProxy")] = mProxyMap.take(mUi.systemProxyFtpEdit->objectName());
            mProxyMap[QStringLiteral("SocksProxy")] = mProxyMap.take(mUi.systemProxySocksEdit->objectName());
            mProxyMap[QStringLiteral("NoProxy")] = mProxyMap.take(mUi.systemNoProxyEdit->objectName());
        }
    } else if (mUi.autoScriptProxyRadioButton->isChecked()) {
        proxyType = KSaveIOConfig::PACProxy;
        mProxyMap[QStringLiteral("ProxyScript")] = mUi.proxyScriptUrlRequester->text();
    } else if (mUi.autoDiscoverProxyRadioButton->isChecked()) {
        proxyType = KSaveIOConfig::WPADProxy;
    }

    KSaveIOConfig::setProxyType(proxyType);
    KSaveIOConfig::setProxyDisplayUrlFlags(displayUrlFlags);
    KSaveIOConfig::setUseReverseProxy(mUi.useReverseProxyCheckBox->isChecked());

    // Save the common proxy setting...
    KSaveIOConfig::setProxyFor(QStringLiteral("http"), mProxyMap.value(QStringLiteral("HttpProxy")));
    KSaveIOConfig::setProxyFor(QStringLiteral("https"), mProxyMap.value(QStringLiteral("HttpsProxy")));
    KSaveIOConfig::setProxyFor(QStringLiteral("ftp"), mProxyMap.value(QStringLiteral("FtpProxy")));
    KSaveIOConfig::setProxyFor(QStringLiteral("socks"), mProxyMap.value(QStringLiteral("SocksProxy")));

    KSaveIOConfig::setProxyConfigScript(mProxyMap.value(QStringLiteral("ProxyScript")));
    KSaveIOConfig::setNoProxyFor(mProxyMap.value(QStringLiteral("NoProxy")));

    KSaveIOConfig::updateRunningWorkers(widget());

    setNeedsSave(false);
}

void KProxyDialog::defaults()
{
    mUi.noProxyRadioButton->setChecked(true);
    mUi.proxyScriptUrlRequester->clear();

    mUi.manualProxyHttpEdit->clear();
    mUi.manualProxyHttpsEdit->clear();
    mUi.manualProxyFtpEdit->clear();
    mUi.manualProxySocksEdit->clear();
    mUi.manualNoProxyEdit->clear();

    mUi.manualProxyHttpSpinBox->setValue(0);
    mUi.manualProxyHttpsSpinBox->setValue(0);
    mUi.manualProxyFtpSpinBox->setValue(0);
    mUi.manualProxySocksSpinBox->setValue(0);

    mUi.systemProxyHttpEdit->clear();
    mUi.systemProxyHttpsEdit->clear();
    mUi.systemProxyFtpEdit->clear();
    mUi.systemProxySocksEdit->clear();

    setNeedsSave(true);
}

bool KProxyDialog::autoDetectSystemProxy(QLineEdit *edit, const QString &envVarStr, bool showValue)
{
    const QStringList envVars = envVarStr.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &envVar : envVars) {
        const QByteArray envVarUtf8(envVar.toUtf8());
        const QByteArray envVarValue = qgetenv(envVarUtf8.constData());
        if (!envVarValue.isEmpty()) {
            if (showValue) {
                mProxyMap[edit->objectName()] = envVar;
                edit->setText(QString::fromUtf8(envVarValue));
            } else {
                edit->setText(envVar);
            }
            edit->setEnabled(!showValue);
            return true;
        }
    }
    return false;
}

void KProxyDialog::autoDetect()
{
    const bool showValue = mUi.showEnvValueCheckBox->isChecked();
    bool wasChanged = false;

    wasChanged |= autoDetectSystemProxy(mUi.systemProxyHttpEdit, QStringLiteral("HTTP_PROXY,http_proxy,HTTPPROXY,httpproxy,PROXY,proxy"), showValue);
    wasChanged |= autoDetectSystemProxy(mUi.systemProxyHttpsEdit, QStringLiteral("HTTPS_PROXY,https_proxy,HTTPSPROXY,httpsproxy,PROXY,proxy"), showValue);
    wasChanged |= autoDetectSystemProxy(mUi.systemProxyFtpEdit, QStringLiteral("FTP_PROXY,ftp_proxy,FTPPROXY,ftpproxy,PROXY,proxy"), showValue);
    wasChanged |= autoDetectSystemProxy(mUi.systemProxySocksEdit, QStringLiteral("SOCKS_PROXY,socks_proxy,SOCKSPROXY,socksproxy,PROXY,proxy"), showValue);
    wasChanged |= autoDetectSystemProxy(mUi.systemNoProxyEdit, QStringLiteral("NO_PROXY,no_proxy"), showValue);

    if (wasChanged) {
        setNeedsSave(true);
    }
}

void KProxyDialog::syncProxies(const QString &text)
{
    if (!mUi.useSameProxyCheckBox->isChecked()) {
        return;
    }

    mUi.manualProxyHttpsEdit->setText(text);
    mUi.manualProxyFtpEdit->setText(text);
    mUi.manualProxySocksEdit->setText(text);
}

void KProxyDialog::syncProxyPorts(int value)
{
    if (!mUi.useSameProxyCheckBox->isChecked()) {
        return;
    }

    mUi.manualProxyHttpsSpinBox->setValue(value);
    mUi.manualProxyFtpSpinBox->setValue(value);
    mUi.manualProxySocksSpinBox->setValue(value);
}

void KProxyDialog::showEnvValue(bool on)
{
    if (on) {
        showSystemProxyUrl(mUi.systemProxyHttpEdit, &mProxyMap[mUi.systemProxyHttpEdit->objectName()]);
        showSystemProxyUrl(mUi.systemProxyHttpsEdit, &mProxyMap[mUi.systemProxyHttpsEdit->objectName()]);
        showSystemProxyUrl(mUi.systemProxyFtpEdit, &mProxyMap[mUi.systemProxyFtpEdit->objectName()]);
        showSystemProxyUrl(mUi.systemProxySocksEdit, &mProxyMap[mUi.systemProxySocksEdit->objectName()]);
        showSystemProxyUrl(mUi.systemNoProxyEdit, &mProxyMap[mUi.systemNoProxyEdit->objectName()]);
        return;
    }

    mUi.systemProxyHttpEdit->setText(mProxyMap.take(mUi.systemProxyHttpEdit->objectName()));
    mUi.systemProxyHttpEdit->setEnabled(true);
    mUi.systemProxyHttpsEdit->setText(mProxyMap.take(mUi.systemProxyHttpsEdit->objectName()));
    mUi.systemProxyHttpsEdit->setEnabled(true);
    mUi.systemProxyFtpEdit->setText(mProxyMap.take(mUi.systemProxyFtpEdit->objectName()));
    mUi.systemProxyFtpEdit->setEnabled(true);
    mUi.systemProxySocksEdit->setText(mProxyMap.take(mUi.systemProxySocksEdit->objectName()));
    mUi.systemProxySocksEdit->setEnabled(true);
    mUi.systemNoProxyEdit->setText(mProxyMap.take(mUi.systemNoProxyEdit->objectName()));
    mUi.systemNoProxyEdit->setEnabled(true);
}

void KProxyDialog::setUseSameProxy(bool on)
{
    if (on) {
        mProxyMap[QStringLiteral("ManProxyHttps")] = manualProxyToText(mUi.manualProxyHttpsEdit, mUi.manualProxyHttpsSpinBox, QLatin1Char(' '));
        mProxyMap[QStringLiteral("ManProxyFtp")] = manualProxyToText(mUi.manualProxyFtpEdit, mUi.manualProxyFtpSpinBox, QLatin1Char(' '));
        mProxyMap[QStringLiteral("ManProxySocks")] = manualProxyToText(mUi.manualProxySocksEdit, mUi.manualProxySocksSpinBox, QLatin1Char(' '));

        const QString &httpProxy = mUi.manualProxyHttpEdit->text();
        if (!httpProxy.isEmpty()) {
            mUi.manualProxyHttpsEdit->setText(httpProxy);
            mUi.manualProxyFtpEdit->setText(httpProxy);
            mUi.manualProxySocksEdit->setText(httpProxy);
        }
        const int httpProxyPort = mUi.manualProxyHttpSpinBox->value();
        if (httpProxyPort > 0) {
            mUi.manualProxyHttpsSpinBox->setValue(httpProxyPort);
            mUi.manualProxyFtpSpinBox->setValue(httpProxyPort);
            mUi.manualProxySocksSpinBox->setValue(httpProxyPort);
        }
        return;
    }

    setManualProxyFromText(mProxyMap.take(QStringLiteral("ManProxyHttps")), mUi.manualProxyHttpsEdit, mUi.manualProxyHttpsSpinBox);
    setManualProxyFromText(mProxyMap.take(QStringLiteral("ManProxyFtp")), mUi.manualProxyFtpEdit, mUi.manualProxyFtpSpinBox);
    setManualProxyFromText(mProxyMap.take(QStringLiteral("ManProxySocks")), mUi.manualProxySocksEdit, mUi.manualProxySocksSpinBox);
}

void KProxyDialog::slotChanged()
{
    setNeedsSave(true);
}

#include "kproxydlg.moc"

#include "moc_kproxydlg.cpp"
