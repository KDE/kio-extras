/* This file is part of the KDE Project
   Copyright (c) 2005 Kévin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef HOMEDIRNOTIFY_H
#define HOMEDIRNOTIFY_H

#include <kurl.h>
#include <kdirnotify.h>

#include <qmap.h>

class HomeDirNotify : public KDirNotify
{
K_DCOP

public:
	HomeDirNotify();

k_dcop:
	virtual ASYNC FilesAdded (const KURL &directory);
	virtual ASYNC FilesRemoved (const KURL::List &fileList);
	virtual ASYNC FilesChanged (const KURL::List &fileList);

private:
	KURL toHomeURL(const KURL &url);
	KURL::List toHomeURLList(const KURL::List &list);
	
	QMap<QString,KURL> m_homeFoldersMap;
};

#endif
