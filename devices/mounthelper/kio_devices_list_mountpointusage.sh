#!/bin/bash
#   This file is part of the KDE project
#   Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Library General Public
#   License as published by the Free Software Foundation; either
#   version 2 of the License, or (at your option) any later version.

#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Library General Public License for more details.

#   You should have received a copy of the GNU Library General Public License
#   along with this library; see the file COPYING.LIB.  If not, write to
#   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#   Boston, MA 02111-1307, USA.





if test  "x$#" != "x1"; then
	exit 1
fi

if test ! -x "/usr/sbin/lsof"; then
	kdialog --sorry "<qt>For this feature to work you need the tool <B>lsof</B> installed in &quot;/usr/sbin/lsof&quot; and your user needs the permission to execute it<BR>*) If you do not have root permissions, ask your system administrator. <BR>*)If you don't know how to install this tool, look into the documentation of your operating system distribution</qt>"
	exit 1
fi
	
deviceidentifier=`echo $1 | perl -p -i -e "s/devices:\///"`
echo identifier is:  $deviceidentifier

dcop kded mountwatcher basicDeviceInfo  $deviceidentifier

dcop kded mountwatcher basicDeviceInfo $deviceidentifier |(
read dummy
read dummy2
read mountpoint
read dummy3
read mounted

if test "x`expr substr $mountpoint 1 6`" != "xfile:/" ; then
	kdialog --error "<B>This device is not supported by this feature</B><BR>If this happens, please send a bug report containing information which device type this is"
	exit 1	
fi

echo the Mountpoint for given identifier is: $mountpoint

if test "x$mounted" != "xtrue"; then
	kdialog --error "<B>This device is not mounted</B><BR>If this happens, please send a bug report containing information which device type this is"
	exit 1
fi

if test "$USER" != "root"; then
	kdialog --yesno "<qt>You are not the <b>root</b> user, you will only see information concerning your user<br>Do you want to switch to the root user?</qt>"
	if test "$?" = "0"; then
		USER=root
		kdesu --nonewdcop  -u root -c $0 $1
		exit
	fi
fi

mountpointstringlength=`expr length $mountpoint`
mountpoint=`expr substr $mountpoint 6 $mountpointstringlength`

/usr/sbin/lsof | grep $mountpoint  | (
	echo TEST
	opid="xxx"
	items=""
	while read app pid user rest; do
		if test "x$opid" != "x$pid" ; then
			 echo $app $user $pid
			 items="$items  $pid  $pid--$app--$user  1"
		fi
		opid=$pid
	done
	echo $items
	killlist=

	if test -z "$items" ; then
		kdialog --msgbox "No task is using mountpoint $mountpoint"
		exit 0
	fi
	for killit in `kdialog --separate-output --checklist "Tasks using files and directories below mountpoint  $mountpoint <BR> <B>(PID--APP--USER)</B> <BR> Choose applications to kill (Be carefull)"  $items`;  do
		kill $killit
	done
)

)
