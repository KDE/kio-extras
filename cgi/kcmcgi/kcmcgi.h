/*
   Copyright (C) 2002 Cornelius Schumacher <schumacher@kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
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
