/*
 *   Copyright (C) 2013 - 2016 by Ivan Cukic <ivan.cukic@kde.org>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License or (at your option) version 3 or any later version
 *   accepted by the membership of KDE e.V. (or its successor approved
 *   by the membership of KDE e.V.), which shall act as a proxy
 *   defined in Section 14 of version 3 of the license.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UTILS_QSQLQUERYITERATOR_H
#define UTILS_QSQLQUERYITERATOR_H

#include <QSqlQuery>
#include <QVariant>

template <typename ResultSet>
class NextValueIterator {
public:
    enum Type {
        NormalIterator,
        EndIterator
    };

    NextValueIterator(ResultSet &query, Type type = NormalIterator)
        : m_query(query)
        , m_type(type)
    {
        if (type != EndIterator) {
            m_query.next();
        }
    }

    inline bool operator!= (const NextValueIterator<ResultSet> &other) const
    {
        Q_UNUSED(other);
        return m_query.isValid();
    }

    inline NextValueIterator<ResultSet> &operator*()
    {
        return *this;
    }

    inline QVariant operator[] (int index) const
    {
        return m_query.value(index);
    }

    inline QVariant operator[] (const QString &name) const
    {
        return m_query.value(name);
    }

    inline NextValueIterator<ResultSet> &operator ++()
    {
        m_query.next();
        return *this;
    }

private:
    ResultSet &m_query;
    Type m_type;

};

NextValueIterator<QSqlQuery> begin(QSqlQuery &query);
NextValueIterator<QSqlQuery> end(QSqlQuery &query);


#endif /* UTILS_QSQLQUERYITERATOR_H */

