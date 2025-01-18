#ifndef NETPREF_H
#define NETPREF_H

#include <KCModule>

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
};

#endif // NETPREF_H
