#ifndef NETPREF_H
#define NETPREF_H

#include <KCModule>

#include "kio_ftprc.h"
#include "kioslave.h"

class QGroupBox;
class QCheckBox;

class KPluralHandlingSpinBox;

class KIOPreferences : public KCModule
{
    Q_OBJECT

public:
    KIOPreferences(QObject *parent, const KPluginMetaData &data);
    ~KIOPreferences() override;

    void load() override;
    void save() override;
    void defaults() override;

protected Q_SLOTS:
    void configChanged()
    {
        setNeedsSave(true);
    }

private:
    QGroupBox *gb_Ftp;
    QCheckBox *cb_globalMarkPartial;
    KPluralHandlingSpinBox *sb_globalMinimumKeepSize;
    QCheckBox *cb_ftpEnablePasv;
    QCheckBox *cb_ftpMarkPartial;
    KioFtp ftpConfig;
    KioSlave kioConfig;
};

#endif // NETPREF_H
