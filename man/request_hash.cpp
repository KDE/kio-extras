/* This file is part of the KDE libraries
   Copyright (C) 2011 Martin Koller <kollix@aon.at>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "request_hash.h"
#include "request_gperf.c"

//---------------------------------------------------------------------

RequestNum RequestHash::getRequest(const char *str, int len)
{
  const Requests *req = Perfect_Hash::in_word_set(str, len);
  return req ? req->number : REQ_UNKNOWN;
}

//---------------------------------------------------------------------
