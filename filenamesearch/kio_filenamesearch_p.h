/*
    SPDX-FileCopyrightText: 2014 Alex Richardson <arichardson.kde@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef FILENAMESEARCH_P_H
#define FILENAMESEARCH_P_H

// Copied from kio/src/core/kiogloba_p.h

#include <qplatformdefs.h>

#ifdef Q_OS_WIN
// windows just sets the mode_t access rights bits to the same value for user+group+other.
// This means using the Linux values here is fine.
#ifndef S_IRUSR
#define S_IRUSR 0400
#endif
#ifndef S_IRGRP
#define S_IRGRP 0040
#endif
#ifndef S_IROTH
#define S_IROTH 0004
#endif

#ifndef S_IWUSR
#define S_IWUSR 0200
#endif
#ifndef S_IWGRP
#define S_IWGRP 0020
#endif
#ifndef S_IWOTH
#define S_IWOTH 0002
#endif

#ifndef S_IXUSR
#define S_IXUSR 0100
#endif
#ifndef S_IXGRP
#define S_IXGRP 0010
#endif
#ifndef S_IXOTH
#define S_IXOTH 0001
#endif

#ifndef S_IRWXU
#define S_IRWXU S_IRUSR | S_IWUSR | S_IXUSR
#endif
#ifndef S_IRWXG
#define S_IRWXG S_IRGRP | S_IWGRP | S_IXGRP
#endif
#ifndef S_IRWXO
#define S_IRWXO S_IROTH | S_IWOTH | S_IXOTH
#endif
Q_STATIC_ASSERT(S_IRUSR == _S_IREAD && S_IWUSR == _S_IWRITE && S_IXUSR == _S_IEXEC);

// these three will never be set in st_mode
#ifndef S_ISUID
#define S_ISUID 04000 // SUID bit does not exist on windows
#endif
#ifndef S_ISGID
#define S_ISGID 02000 // SGID bit does not exist on windows
#endif
#ifndef S_ISVTX
#define S_ISVTX 01000 // sticky bit does not exist on windows
#endif

// Windows does not have S_IFBLK and S_IFSOCK, just use the Linux values, they won't conflict
#ifndef S_IFBLK
#define S_IFBLK 0060000
#endif
#ifndef S_IFSOCK
#define S_IFSOCK 0140000
#endif

#endif // Q_OS_WIN

#endif // FILENAMESEARCH_P_H