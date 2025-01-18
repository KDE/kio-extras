
// Own
#include "netpref.h"

// Qt
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>

// KDE
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KPluralHandlingSpinBox>
#include <KProtocolManager>
#include <kio/ioworker_defaults.h>

// Local
#include "../ksaveioconfig.h"

K_PLUGIN_CLASS_WITH_JSON(KIOPreferences, "kcm_netpref.json")

KIOPreferences::KIOPreferences(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(widget());

    QGroupBox *gb_Global = new QGroupBox(i18n("Global Options"), widget());
    gb_Global->setFlat(true);
    mainLayout->addWidget(gb_Global);
    QVBoxLayout *globalLayout = new QVBoxLayout(gb_Global);

    cb_globalMarkPartial = new QCheckBox(i18n("Mark &partially uploaded files"), widget());
    cb_globalMarkPartial->setWhatsThis(
        i18n("<p>Marks partially uploaded files "
             "through SMB, SFTP and other protocols."
             "</p><p>When this option is "
             "enabled, partially uploaded files "
             "will have a \".part\" extension. "
             "This extension will be removed "
             "once the transfer is complete.</p>"));
    connect(cb_globalMarkPartial, &QAbstractButton::toggled, this, &KIOPreferences::configChanged);
    globalLayout->addWidget(cb_globalMarkPartial);
    globalLayout->setAlignment(cb_globalMarkPartial, Qt::AlignHCenter);

    auto partialWidget = new QWidget(widget());
    connect(cb_globalMarkPartial, &QAbstractButton::toggled, partialWidget, &QWidget::setEnabled);
    globalLayout->addWidget(partialWidget);
    auto partialLayout = new QFormLayout(partialWidget);
    partialLayout->setFormAlignment(Qt::AlignHCenter);
    partialLayout->setContentsMargins(0, 0, 0, 0);

    sb_globalMinimumKeepSize = new KPluralHandlingSpinBox(widget());
    sb_globalMinimumKeepSize->setSuffix(ki18np(" byte", " bytes"));
    connect(sb_globalMinimumKeepSize, qOverload<int>(&QSpinBox::valueChanged), this, &KIOPreferences::configChanged);
    partialLayout->addRow(i18nc("@label:spinbox", "If cancelled, automatically delete partially uploaded files smaller than:"), sb_globalMinimumKeepSize);

    gb_Ftp = new QGroupBox(i18n("FTP Options"), widget());
    gb_Ftp->setFlat(true);
    mainLayout->addWidget(gb_Ftp);
    QVBoxLayout *ftpLayout = new QVBoxLayout(gb_Ftp);
    ftpLayout->setAlignment(Qt::AlignHCenter);

    cb_ftpEnablePasv = new QCheckBox(i18n("Enable passive &mode (PASV)"), widget());
    cb_ftpEnablePasv->setWhatsThis(
        i18n("Enables FTP's \"passive\" mode. "
             "This is required to allow FTP to "
             "work from behind firewalls."));
    connect(cb_ftpEnablePasv, &QAbstractButton::toggled, this, &KIOPreferences::configChanged);
    ftpLayout->addWidget(cb_ftpEnablePasv);

    cb_ftpMarkPartial = new QCheckBox(i18n("Mark &partially uploaded files"), widget());
    cb_ftpMarkPartial->setWhatsThis(
        i18n("<p>Marks partially uploaded FTP "
             "files.</p><p>When this option is "
             "enabled, partially uploaded files "
             "will have a \".part\" extension. "
             "This extension will be removed "
             "once the transfer is complete.</p>"));
    connect(cb_ftpMarkPartial, &QAbstractButton::toggled, this, &KIOPreferences::configChanged);
    ftpLayout->addWidget(cb_ftpMarkPartial);

    mainLayout->addStretch(1);
}

KIOPreferences::~KIOPreferences()
{
}

void KIOPreferences::load()
{
    KProtocolManager proto;

    cb_globalMarkPartial->setChecked(proto.markPartial());
    sb_globalMinimumKeepSize->setRange(0, 1024 * 1024 * 1024 /* 1 GiB */);
    sb_globalMinimumKeepSize->setValue(proto.minimumKeepSize());

    KConfig config(QStringLiteral("kio_ftprc"), KConfig::NoGlobals);
    cb_ftpEnablePasv->setChecked(!config.group(QString()).readEntry("DisablePassiveMode", false));
    cb_ftpMarkPartial->setChecked(config.group(QString()).readEntry("MarkPartial", true));
    setNeedsSave(false);
}

void KIOPreferences::save()
{
    KSaveIOConfig::setMarkPartial(cb_globalMarkPartial->isChecked());
    KSaveIOConfig::setMinimumKeepSize(sb_globalMinimumKeepSize->value());

    KConfig config(QStringLiteral("kio_ftprc"), KConfig::NoGlobals);
    config.group(QString()).writeEntry("DisablePassiveMode", !cb_ftpEnablePasv->isChecked());
    config.group(QString()).writeEntry("MarkPartial", cb_ftpMarkPartial->isChecked());
    config.sync();

    KSaveIOConfig::updateRunningWorkers(widget());

    setNeedsSave(false);
}

void KIOPreferences::defaults()
{
    cb_globalMarkPartial->setChecked(true);

    cb_ftpEnablePasv->setChecked(true);
    cb_ftpMarkPartial->setChecked(true);

    setNeedsSave(true);
}

#include "netpref.moc"

#include "moc_netpref.cpp"
