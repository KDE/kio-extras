/*
 *  man.h - part of the KDE Help Center
 *
 *  Copyright (c) 1999 Matthias Elter (me@kde.org)
 *
 *  based on kdehelp code (c) Martin R. Jones
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __man_h__
#define __man_h__

#include <qlist.h>
#include <qstring.h>

#define MAN_SECTIONS		"1:2:3:4:5:6:7:8:9:n"
#define MAN_ENV			"MANPATH"
#define MAN_MAXSECTIONS		20
#define MAN_MAXPATHS		50

class ManPage
{
 public:

    ManPage (const QString& name = "", const QString& location = "")
	{ m_name = name; m_location = location; }

    const QString& name() const { return m_name; }
    const QString& location() const { return m_location; }

 private:

    QString m_name;
    QString m_location;
};

class ManSection
{
 public:

    ManSection(const QString& name);
    ~ManSection();

    int  numPages() { return m_numPages; }
    const QString& name() { return m_name; }
    const QString& description() { return m_description; }

    void readSection();
    void readDir(const QString& dirName);
    QList<ManPage> *pages() { return &m_pages; }

 private:

    QList<ManPage> m_pages;

    QString m_name;
    QString m_description;

    int	 m_numPages;
    bool m_isRead;

    // static search path for man pages
    static QString searchPath[MAN_MAXPATHS];
    static int numPaths;
    static int sectCount;
};

class ManParser
{
 public:

    ManParser();
    ~ManParser();

    int  readLocation(const QString& location);
    const QString& location() { return m_location; }

    QString *page() { return &m_page; }

 private:

    QString m_page;
    QString m_location;

    static int instance;
};

#endif
