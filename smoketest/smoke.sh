#!/bin/bash
#
# SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
#
# SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

set -e

export KDE_FORK_SLAVES=1

# enter a valid localhost login here
host=$1
if [ "$host" = "" ]; then
    echo "Need to pass base uri as argument... e.g. smb://foo/srv"
    exit 1
fi
dir='kio_smoke_test'
uri="$host/$dir"

kioclient5 remove $uri || true
kioclient5 copy $dir $uri

# upload
kioclient5 copy file1 $uri/file1
if [ "$(kioclient5 cat $uri/file1)" != "content1" ]; then
    echo "Reading file1 failed!"
    exit 1
fi

# remote rename
kioclient5 move $uri/file1 $uri/file2
if [ "$(kioclient5 cat $uri/file2)" != "content1" ]; then
    echo "Moving to file2 failed!"
    exit 1
fi

if kioclient5 cat $uri/file1; then
    echo "Tried to move file1 to file2 but now both exist!"
    exit 1
fi

# remote copy
kioclient5 copy $uri/file2 $uri/file1

# both exist
kioclient5 cat $uri/file1
kioclient5 cat $uri/file2

# remote remove
kioclient5 remove $uri/file1
if kioclient5 cat $uri/file1; then
    echo "Tried to remove file1 but still exists!"
    exit 1
fi

# ls
kioclient5 ls $uri # not checking output, too lazy

# download
rm -rfv file2
kioclient5 copy $uri/file2 file://`pwd`/file2 # download
if [ ! -e file2 ]; then
    echo "failed to download :("
    exit 1
fi
rm -rfv file2

# delete remote dir again
kioclient5 remove $uri
