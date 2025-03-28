#ifndef NETPREF_H
#define NETPREF_H

#include <KCModule>

#include "kio_ftprc.h"
#include "kioslave.h"

class QCheckBox;
class QGroupBox;
class QSpinBox;

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
    QCheckBox *cb_globalMarkPartial;
    QCheckBox *cb_ftpEnablePasv;
    QCheckBox *cb_ftpMarkPartial;
    QGroupBox *gb_Ftp;
    QSpinBox  *sb_globalMinimumKeepSize;
    KioFtp ftpConfig;
    KioSlave kioConfig;
};

#endif // NETPREF_H
