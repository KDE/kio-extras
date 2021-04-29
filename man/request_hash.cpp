/* This file is part of the KDE libraries
   SPDX-FileCopyrightText: 2011 Martin Koller <kollix@aon.at>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "request_hash.h"
#include <request_gperf.h>

//---------------------------------------------------------------------

RequestNum RequestHash::getRequest(const char *str, int len)
{
    const Requests *req = Perfect_Hash::in_word_set(str, len);
    return req ? req->number : REQ_UNKNOWN;
}

//---------------------------------------------------------------------
