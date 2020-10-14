/*  This file is part of the KDE libraries
    Copyright (c) 2000 Matthias Hoelzer-Kluepfel <mhk@caldera.de>


    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef __kio_man_h__
#define __kio_man_h__



#include <QBuffer>
#include <QTextStream>

#include <kio/global.h>
#include <kio/slavebase.h>

class MANProtocol : public QObject, public KIO::SlaveBase
{
    Q_OBJECT

public:

    MANProtocol(const QByteArray &pool_socket, const QByteArray &app_socket);
    ~MANProtocol() override;

    void get(const QUrl& url) override;
    void stat(const QUrl& url) override;

    void mimetype(const QUrl &url) override;
    void listDir(const QUrl &url) override;

    void outputError(const QString& errmsg);
    void outputMatchingPages(const QStringList &matchingPages);

    void showMainIndex();
    void showIndex(const QString& section);

    // the following two functions are the interface to man2html
    void output(const char *insert);
    char *readManPage(const char *filename);

    static MANProtocol *self();
   
private:
    void checkManPaths();
    QStringList manDirectories();
    QMap<QString, QString> buildIndexMap(const QString& section);
    bool addWhatIs(QMap<QString, QString>& i, const QString& f, const QString& mark);
    void parseWhatIs( QMap<QString, QString> &i, QTextStream &t, const QString &mark );
    QStringList findPages(const QString& section, 
                          const QString &title, 
                          bool full_path = true);

    QStringList buildSectionList(const QStringList& dirs) const;
    void constructPath(QStringList& constr_path, QStringList constr_catmanpath);

    void outputHeader(QTextStream &os, const QString &header, const QString &title = QString());

private:
    static MANProtocol *_self;
    QByteArray lastdir;

    void findManPagesInSection(const QString &dir, const QString &title, bool full_path, QStringList &list);
    QStringList m_manpath; ///< Path of man directories
    QStringList m_mandbpath; ///< Path of catman directories
    QStringList section_names;

    QString mySgml2RoffPath;
    void getProgramPath();

    QBuffer m_outputBuffer; ///< Buffer for the output
    QByteArray m_manCSSFile; ///< Path to kio_man.css
};


#endif
