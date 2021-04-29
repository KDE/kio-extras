/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef MOLLETNETWORK_EXPORT_H
#define MOLLETNETWORK_EXPORT_H

#ifndef MOLLETNETWORK_EXPORT
// building the library?
# if defined(MAKE_MOLLETNETWORK_LIB)
#  define MOLLETNETWORK_EXPORT Q_DECL_EXPORT
// using the library
# else
#  define MOLLETNETWORK_EXPORT Q_DECL_IMPORT
# endif
#endif

# ifndef MOLLETNETWORK_EXPORT_DEPRECATED
#  define MOLLETNETWORK_EXPORT_DEPRECATED QT_DEPRECATED MOLLETNETWORK_EXPORT
# endif

#endif
