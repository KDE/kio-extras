/*
 *   SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <kio/udsentry.h>
#include <kio_version.h>

#include <initializer_list>
#include <utility>

/*
 * UDSEntry keeps string values and number values in two separate vectors since KIO 6.29.
 * Filling one vector and then the other is faster than alternating between them, and the
 * pre-allocation is per vector as well. The helpers below give a worker one way to write
 * that which also builds against the older single-vector UDSEntry.
 *
 * Once KF_MIN_VERSION reaches 6.29 this header can go away. Call KIO::UDSEntry::reserveStrings
 * and KIO::UDSEntry::reserveNumbers in place of reserveEntry, and the two KIO::UDSEntry::insert
 * overloads taking an initializer list in place of insertStrings and insertNumbers.
 */
namespace KIOExtras
{

/*
 * Pre-allocates room for stringFields string values and numberFields number values.
 */
inline void reserveEntry(KIO::UDSEntry &entry, int stringFields, int numberFields)
{
#if KIO_VERSION >= QT_VERSION_CHECK(6, 29, 0)
    entry.reserveStrings(stringFields);
    entry.reserveNumbers(numberFields);
#else
    entry.reserve(stringFields + numberFields);
#endif
}

/*
 * Inserts string fields, pre-allocating room for all of them first.
 */
inline void insertStrings(KIO::UDSEntry &entry, std::initializer_list<std::pair<uint, const QString &>> fieldValuePairs)
{
#if KIO_VERSION >= QT_VERSION_CHECK(6, 29, 0)
    entry.insert(fieldValuePairs);
#else
    entry.reserve(entry.count() + int(fieldValuePairs.size()));
    for (const auto &[field, value] : fieldValuePairs) {
        entry.fastInsert(field, value);
    }
#endif
}

/*
 * Inserts number fields, pre-allocating room for all of them first.
 */
inline void insertNumbers(KIO::UDSEntry &entry, std::initializer_list<std::pair<uint, long long>> fieldValuePairs)
{
#if KIO_VERSION >= QT_VERSION_CHECK(6, 29, 0)
    entry.insert(fieldValuePairs);
#else
    entry.reserve(entry.count() + int(fieldValuePairs.size()));
    for (const auto &[field, value] : fieldValuePairs) {
        entry.fastInsert(field, value);
    }
#endif
}

}
