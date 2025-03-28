
// Own
#include "netpref.h"

// Qt
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QSpinBox>

// KDE
#include <KLocalization>
#include <KLocalizedString>
#include <KPluginFactory>

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

    sb_globalMinimumKeepSize = new QSpinBox(widget());
    KLocalization::setupSpinBoxFormatString(sb_globalMinimumKeepSize, ki18ncp("@label:spinbox", "%v byte", "%v bytes"));
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
    kioConfig.load();

    cb_globalMarkPartial->setChecked(kioConfig.markPartial());
    sb_globalMinimumKeepSize->setRange(0, 1024 * 1024 * 1024 /* 1 GiB */);
    sb_globalMinimumKeepSize->setValue(kioConfig.minimumKeepSize());

    ftpConfig.load();

    cb_ftpEnablePasv->setChecked(!ftpConfig.disablePassiveMode());
    cb_ftpMarkPartial->setChecked(ftpConfig.markPartial());
    setNeedsSave(false);
}

void KIOPreferences::save()
{
    kioConfig.setMarkPartial(cb_globalMarkPartial->isChecked());
    kioConfig.setMinimumKeepSize(sb_globalMinimumKeepSize->value());
    kioConfig.save();

    ftpConfig.setDisablePassiveMode(!cb_ftpEnablePasv->isChecked());
    ftpConfig.setMarkPartial(cb_ftpMarkPartial->isChecked());
    ftpConfig.save();

    KSaveIOConfig::updateRunningWorkers(widget());

    setNeedsSave(false);
}

void KIOPreferences::defaults()
{
    kioConfig.setDefaults();
    ftpConfig.setDefaults();

    cb_globalMarkPartial->setChecked(kioConfig.markPartial());
    sb_globalMinimumKeepSize->setValue(kioConfig.minimumKeepSize());

    cb_ftpEnablePasv->setChecked(!ftpConfig.disablePassiveMode());
    cb_ftpMarkPartial->setChecked(ftpConfig.markPartial());

    setNeedsSave(true);
}

#include "netpref.moc"

#include "moc_netpref.cpp"
