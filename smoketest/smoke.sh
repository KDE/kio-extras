#!/bin/bash
#
# Copyright (C) 2019 Harald Sitter <sitter@kde.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3 of
# the License or any later version accepted by the membership of
# KDE e.V. (or its successor approved by the membership of KDE
# e.V.), which shall act as a proxy defined in Section 14 of
# version 3 of the license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
