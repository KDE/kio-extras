/*  This file is part of the KDE project

    Copyright (C) 2002,2003 Dawit Alemayehu <adawit@kde.org>
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 1999 Yves Arrouye <yves@realnames.com>

    Advanced web shortcuts
    Copyright (C) 2001 Andreas Hochsteger <e9625392@student.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef KURIIKWSFILTERENG_H
#define KURIIKWSFILTERENG_H

#include <QtCore/QMap>
#include <QtCore/QStringList>

class SearchProvider;

class KURISearchFilterEngine
{
public:
  typedef QMap <QString, QString> SubstMap;

  KURISearchFilterEngine();
  ~KURISearchFilterEngine();

  QByteArray name() const;
  char keywordDelimiter() const;
  QStringList favoriteEngineList() const;
  SearchProvider* webShortcutQuery (const QString& typedString, QString& searchTerm) const;
  SearchProvider* autoWebSearchQuery (const QString& typedString, const QString& defaultEngine = QString()) const;
  QString formatResult (const QString& url, const QString& cset1, const QString& cset2,
                        const QString& query, bool isMalformed) const;

  static KURISearchFilterEngine *self();
  void loadConfig();

protected:
  QString formatResult (const QString& url, const QString& cset1, const QString& cset2,
                        const QString& query, bool isMalformed, SubstMap& map) const;

private:
  QStringList modifySubstitutionMap (SubstMap& map, const QString& query) const;

  QString substituteQuery (const QString& url, SubstMap &map,
                           const QString& userquery, QTextCodec *codec) const;

  QString m_defaultSearchEngine;
  QStringList m_favoriteEngines;
  bool m_bWebShortcutsEnabled;
  char m_cKeywordDelimiter;
  static KURISearchFilterEngine *s_pSelf;
};

#endif // KURIIKWSFILTERENG_H
