// This file is part of the KDE libraries
// Copyright (C) 2011 Martin Koller <kollix@aon.at>

#include "request_hash.h"
#include "request_gperf.c"

//---------------------------------------------------------------------

RequestNum RequestHash::getRequest(const char *str, int len)
{
  const Requests *req = Perfect_Hash::in_word_set(str, len);
  return req ? req->number : REQ_UNKNOWN;
}

//---------------------------------------------------------------------
