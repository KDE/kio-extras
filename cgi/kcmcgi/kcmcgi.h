#ifndef KCMCGI_H
#define KCMCGI_H

#include <kcmodule.h>

class QListBox;
class QPushButton;

class KConfig;

class KCMCgi : public KCModule
{
    Q_OBJECT
  public:
    KCMCgi( QWidget *parent = 0, const char *name = 0 );
    ~KCMCgi();
    
    virtual const KAboutData * aboutData () const;

    void load();
    void save();
    void defaults();
    QString quickHelp() const;

  public slots:

  protected slots:
    void addPath();
    void removePath();

  private:
    QListBox *mListBox;
    QPushButton *mAddButton;
    QPushButton *mRemoveButton;
    
    KConfig *mConfig;
    
    KAboutData *mAboutData;
};

#endif
