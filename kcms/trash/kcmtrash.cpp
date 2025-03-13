/*
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcmtrash.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSpinBox>
#include <QStorageInfo>

#include <KConfig>
#include <KConfigGroup>
#include <KFormat>
#include <KIO/SpecialJob>
#include <KLocalization>
#include <KLocalizedString>
#include <KPluginFactory>
#include <kio/simplejob.h>

K_PLUGIN_CLASS_WITH_JSON(TrashConfigModule, "kcm_trash.json")

static constexpr int SPECIAL_TRASH_DIRECTORIES = 4;

TrashConfigModule::TrashConfigModule(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
    , trashInitialize(false)
{
    QByteArray specialData;
    QDataStream stream(&specialData, QIODevice::WriteOnly);
    stream << SPECIAL_TRASH_DIRECTORIES;
    auto job = KIO::special(QUrl(QStringLiteral("trash:")), specialData);

    readConfig();

    connect(job, &KJob::finished, [job, this]() {
        auto doc = QJsonDocument::fromJson(job->metaData().value(QStringLiteral("TRASH_DIRECTORIES")).toLocal8Bit());
        const auto map = doc.object().toVariantMap();
        for (auto it = map.begin(); it != map.end(); it++) {
            m_trashMap.insert(it.key().toInt(), it.value().toString());
        }
        setupGui();
        trashChanged(0);

        connect(mUseTimeLimit, &QAbstractButton::toggled, this, &TrashConfigModule::markAsChanged);
        connect(mUseTimeLimit, &QAbstractButton::toggled, this, &TrashConfigModule::useTypeChanged);
        connect(mDays, qOverload<int>(&QSpinBox::valueChanged), this, &TrashConfigModule::markAsChanged);
        connect(mUseSizeLimit, &QAbstractButton::toggled, this, &TrashConfigModule::markAsChanged);
        connect(mUseSizeLimit, &QAbstractButton::toggled, this, &TrashConfigModule::useTypeChanged);
        connect(mPercent, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &TrashConfigModule::percentChanged);
        connect(mPercent, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &TrashConfigModule::markAsChanged);
        connect(mLimitReachedAction, qOverload<int>(&QComboBox::currentIndexChanged), this, &TrashConfigModule::markAsChanged);

        useTypeChanged();

        trashInitialize = true;
    });
}

TrashConfigModule::~TrashConfigModule()
{
}

void TrashConfigModule::save()
{
    if (!mCurrentTrash.isEmpty()) {
        ConfigEntry entry;
        entry.useTimeLimit = mUseTimeLimit->isChecked();
        entry.days = mDays->value();
        entry.useSizeLimit = mUseSizeLimit->isChecked();
        entry.percent = mPercent->value(), entry.actionType = mLimitReachedAction->currentIndex();
        mConfigMap.insert(mCurrentTrash, entry);
    }

    writeConfig();
}

void TrashConfigModule::defaults()
{
    ConfigEntry entry;
    entry.useTimeLimit = false;
    entry.days = 7;
    entry.useSizeLimit = true;
    entry.percent = 10.0;
    entry.actionType = 0;
    mConfigMap.insert(mCurrentTrash, entry);
    trashInitialize = false;
    trashChanged(0);
}

void TrashConfigModule::percentChanged(double percent)
{
    qint64 fullSize = 0;
    QStorageInfo storageInfo(mCurrentTrash);
    if (storageInfo.isValid() && storageInfo.isReady()) {
        fullSize = storageInfo.bytesTotal();
    }

    double size = static_cast<double>(fullSize / 100) * percent;

    KFormat format;
    mSizeLabel->setText(QLatin1Char('(') + format.formatByteSize(size, 2) + QLatin1Char(')'));
}

void TrashConfigModule::trashChanged(int value)
{
    if (!mCurrentTrash.isEmpty() && trashInitialize) {
        ConfigEntry entry;
        entry.useTimeLimit = mUseTimeLimit->isChecked();
        entry.days = mDays->value();
        entry.useSizeLimit = mUseSizeLimit->isChecked();
        entry.percent = mPercent->value(), entry.actionType = mLimitReachedAction->currentIndex();
        mConfigMap.insert(mCurrentTrash, entry);
    }

    mCurrentTrash = m_trashMap[value];
    const auto currentTrashIt = mConfigMap.constFind(mCurrentTrash);
    if (currentTrashIt != mConfigMap.constEnd()) {
        const ConfigEntry &entry = *currentTrashIt;
        mUseTimeLimit->setChecked(entry.useTimeLimit);
        mDays->setValue(entry.days);
        mUseSizeLimit->setChecked(entry.useSizeLimit);
        mPercent->setValue(entry.percent);
        mLimitReachedAction->setCurrentIndex(entry.actionType);
    } else {
        mUseTimeLimit->setChecked(false);
        mDays->setValue(7);
        mUseSizeLimit->setChecked(true);
        mPercent->setValue(10.0);
        mLimitReachedAction->setCurrentIndex(0);
    }

    percentChanged(mPercent->value());
}

void TrashConfigModule::useTypeChanged()
{
    mDays->setEnabled(mUseTimeLimit->isChecked());
    mPercent->setEnabled(mUseSizeLimit->isChecked());
    mSizeLabel->setEnabled(mUseSizeLimit->isChecked());
}

void TrashConfigModule::readConfig()
{
    KConfig config(QStringLiteral("ktrashrc"));
    mConfigMap.clear();

    const QStringList groups = config.groupList();
    for (const auto &name : groups) {
        if (name.startsWith(QLatin1Char('/'))) {
            const KConfigGroup group = config.group(name);

            ConfigEntry entry;
            entry.useTimeLimit = group.readEntry("UseTimeLimit", false);
            entry.days = group.readEntry("Days", 7);
            entry.useSizeLimit = group.readEntry("UseSizeLimit", true);
            entry.percent = group.readEntry("Percent", 10.0);
            entry.actionType = group.readEntry("LimitReachedAction", 0);
            mConfigMap.insert(name, entry);
        }
    }
}

void TrashConfigModule::writeConfig()
{
    KConfig config(QStringLiteral("ktrashrc"));

    // first delete all existing groups
    const QStringList groups = config.groupList();
    for (const auto &name : groups) {
        if (name.startsWith(QLatin1Char('/'))) {
            config.deleteGroup(name);
        }
    }

    QMapIterator<QString, ConfigEntry> it(mConfigMap);
    while (it.hasNext()) {
        it.next();
        KConfigGroup group = config.group(it.key());

        const ConfigEntry entry = it.value();
        group.writeEntry("UseTimeLimit", entry.useTimeLimit);
        group.writeEntry("Days", entry.days);
        group.writeEntry("UseSizeLimit", entry.useSizeLimit);
        group.writeEntry("Percent", entry.percent);
        group.writeEntry("LimitReachedAction", entry.actionType);
    }
    config.sync();
}

void TrashConfigModule::setupGui()
{
    QVBoxLayout *layout = new QVBoxLayout(widget());

#ifdef Q_OS_OSX
    QLabel *infoText = new QLabel(i18n("<para>KDE's wastebin is configured to use the <b>Finder</b>'s Trash.<br></para>"));
    infoText->setWhatsThis(i18nc("@info:whatsthis",
                                 "<para>Emptying KDE's wastebin will remove only KDE's trash items, while<br>"
                                 "emptying the Trash through the Finder will delete everything.</para>"
                                 "<para>KDE's trash items will show up in a folder called KDE.trash, in the Trash can.</para>"));
    layout->addWidget(infoText);
#endif

    if (m_trashMap.count() != 1) {
        // If we have multiple trashes, we setup a widget to choose
        // which trash to configure
        QListWidget *mountPoints = new QListWidget(widget());
        layout->addWidget(mountPoints);

        QMapIterator<int, QString> it(m_trashMap);
        while (it.hasNext()) {
            it.next();

            QString mountPoint;
            QStorageInfo storageInfo(it.value());
            if (storageInfo.isValid() && storageInfo.isReady()) {
                mountPoint = storageInfo.rootPath();
            }
            auto item = new QListWidgetItem(QIcon(QStringLiteral("folder")), mountPoint);
            item->setData(Qt::UserRole, it.key());

            mountPoints->addItem(item);
        }

        mountPoints->setCurrentRow(0);

        connect(mountPoints, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *item) {
            trashChanged(item->data(Qt::UserRole).toInt());
        });
    } else {
        mCurrentTrash = m_trashMap.value(0);
    }

    QFormLayout *formLayout = new QFormLayout();
    layout->addLayout(formLayout);

    QHBoxLayout *daysLayout = new QHBoxLayout();

    mUseTimeLimit = new QCheckBox(i18n("Delete files older than"), widget());
    mUseTimeLimit->setWhatsThis(
        xi18nc("@info:whatsthis",
               "<para>Check this box to allow <emphasis strong='true'>automatic deletion</emphasis> of files that are older than the value specified. "
               "Leave this disabled to <emphasis strong='true'>not</emphasis> automatically delete any items after a certain timespan</para>"));
    daysLayout->addWidget(mUseTimeLimit);
    mDays = new QSpinBox(widget());

    mDays->setRange(1, 365);
    mDays->setSingleStep(1);
    KLocalization::setupSpinBoxFormatString(mDays, ki18ncp("@label:spinbox", "%v day", "%v days"));
    mDays->setWhatsThis(xi18nc("@info:whatsthis",
                               "<para>Set the number of days that files can remain in the trash. "
                               "Any files older than this will be automatically deleted.</para>"));
    daysLayout->addWidget(mDays);
    daysLayout->addStretch();
    formLayout->addRow(i18n("Cleanup:"), daysLayout);

    QHBoxLayout *maximumSizeLayout = new QHBoxLayout();
    mUseSizeLimit = new QCheckBox(i18n("Limit to"), widget());
    mUseSizeLimit->setWhatsThis(xi18nc("@info:whatsthis",
                                       "<para>Check this box to limit the trash to the maximum amount of disk space that you specify below. "
                                       "Otherwise, it will be unlimited.</para>"));
    maximumSizeLayout->addWidget(mUseSizeLimit);
    formLayout->addRow(i18n("Size:"), maximumSizeLayout);

    mPercent = new QDoubleSpinBox(widget());
    mPercent->setRange(0.01, 100);
    mPercent->setDecimals(2);
    mPercent->setSingleStep(1);
    KLocalization::setupSpinBoxFormatString(mPercent, ki18nc("@label:spinbox Percent value", "%v%"));
    mPercent->setWhatsThis(xi18nc("@info:whatsthis", "<para>This is the maximum percent of disk space that will be used for the trash.</para>"));
    maximumSizeLayout->addWidget(mPercent);

    mSizeLabel = new QLabel(widget());
    mSizeLabel->setWhatsThis(
        xi18nc("@info:whatsthis", "<para>This is the calculated amount of disk space that will be allowed for the trash, the maximum.</para>"));
    maximumSizeLayout->addWidget(mSizeLabel);

    mLimitReachedAction = new QComboBox();
    mLimitReachedAction->addItem(i18n("Show a warning"));
    mLimitReachedAction->addItem(i18n("Delete oldest files from trash"));
    mLimitReachedAction->addItem(i18n("Delete biggest files from trash"));
    mLimitReachedAction->setWhatsThis(xi18nc("@info:whatsthis",
                                             "<para>When the size limit is reached, it will prefer to delete the type of files that you specify, first. "
                                             "If this is set to warn you, it will do so instead of automatically deleting files.</para>"));
    formLayout->addRow(i18n("Full trash:"), mLimitReachedAction);

    layout->addStretch();
}

#include "kcmtrash.moc"

#include "moc_kcmtrash.cpp"
