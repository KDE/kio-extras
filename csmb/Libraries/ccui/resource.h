/* Name: resource.h

   Description: This file is a part of the ccui library.

   Author:	Oleg Noskov (olegn@corel.com)

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


#ifndef _RESOURCE_H_
#define _RESOURCE_H_

//translate macro provided for QT compatibility
#define	tr(x)	x
//display date format - it is locale specific
//this format line will be changed for each locale
#define IDS_FORMAT	"%s %s %d, %d"

#define IDS_COMMAND_FORMAT	"%s/%s %02d %02d %02d %02d %04d %02d"

#define IDS_DATE_DIR		"/etc"
#define IDS_DATE_COMMAND	"set_dtime.sh"

#define IDS_DATE_ERROR	"Error setting system date."
#define IDS_DATE_ERROR1	"Date"

#endif//_RESOURCE_H_
