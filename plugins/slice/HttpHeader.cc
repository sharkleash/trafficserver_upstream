/** @file
  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

#include "HttpHeader.h"

#include "slice.h"

#include <cinttypes>
#include <cstdlib>
#include <cstring>

TSHttpType
HttpHeader::type() const
{
  if (isValid()) {
    return TSHttpHdrTypeGet(m_buffer, m_lochdr);
  } else {
    return TS_HTTP_TYPE_UNKNOWN;
  }
}

TSHttpStatus
HttpHeader::status() const
{
  TSHttpStatus res = TS_HTTP_STATUS_NONE;
  if (isValid()) {
    res = TSHttpHdrStatusGet(m_buffer, m_lochdr);
  }
  return res;
}

bool
HttpHeader::setStatus(TSHttpStatus const newstatus)
{
  if (!isValid()) {
    return false;
  }

  return TS_SUCCESS == TSHttpHdrStatusSet(m_buffer, m_lochdr, newstatus);
}

char *
HttpHeader::urlString(int *const urllen) const
{
  char *urlstr = nullptr;
  TSAssert(nullptr != urllen);

  TSMLoc             locurl = nullptr;
  TSReturnCode const rcode  = TSHttpHdrUrlGet(m_buffer, m_lochdr, &locurl);
  if (nullptr != locurl) {
    if (TS_SUCCESS == rcode) {
      urlstr = TSUrlStringGet(m_buffer, locurl, urllen);
    } else {
      *urllen = 0;
    }
    TSHandleMLocRelease(m_buffer, m_lochdr, locurl);
  }

  return urlstr;
}

bool
HttpHeader::setUrl(TSMBuffer const bufurl, TSMLoc const locurl)
{
  if (!isValid()) {
    return false;
  }

  TSMLoc       locurlout = nullptr;
  TSReturnCode rcode     = TSHttpHdrUrlGet(m_buffer, m_lochdr, &locurlout);
  if (TS_SUCCESS != rcode) {
    return false;
  }

  // copy the url
  rcode = TSUrlCopy(m_buffer, locurlout, bufurl, locurl);

  // set url active
  if (TS_SUCCESS == rcode) {
    rcode = TSHttpHdrUrlSet(m_buffer, m_lochdr, locurlout);
  }

  TSHandleMLocRelease(m_buffer, m_lochdr, locurlout);

  return TS_SUCCESS == rcode;
}

bool
HttpHeader::setReason(char const *const valstr, int const vallen)
{
  if (isValid()) {
    return TS_SUCCESS == TSHttpHdrReasonSet(m_buffer, m_lochdr, valstr, vallen);
  } else {
    return false;
  }
}

char const *
HttpHeader::getCharPtr(CharPtrGetFunc func, int *const len) const
{
  char const *res = nullptr;
  if (isValid()) {
    int reslen = 0;
    res        = func(m_buffer, m_lochdr, &reslen);

    if (nullptr != len) {
      *len = reslen;
    }
  }

  if (nullptr == res && nullptr != len) {
    *len = 0;
  }

  return res;
}

bool
HttpHeader::hasKey(char const *const key, int const keylen) const
{
  if (!isValid()) {
    return false;
  }

  TSMLoc const locfield(TSMimeHdrFieldFind(m_buffer, m_lochdr, key, keylen));
  if (nullptr != locfield) {
    TSHandleMLocRelease(m_buffer, m_lochdr, locfield);
    return true;
  }

  return false;
}

bool
HttpHeader::removeKey(char const *const keystr, int const keylen)
{
  if (!isValid()) {
    return false;
  }

  bool status = true;

  TSMLoc const locfield = TSMimeHdrFieldFind(m_buffer, m_lochdr, keystr, keylen);
  if (nullptr != locfield) {
    int const rcode = TSMimeHdrFieldRemove(m_buffer, m_lochdr, locfield);
    status          = (TS_SUCCESS == rcode);
    TSHandleMLocRelease(m_buffer, m_lochdr, locfield);
  }

  return status;
}

bool
HttpHeader::valueForKey(char const *const keystr, int const keylen, char *const valstr, int *const vallen, int const index) const
{
  if (!isValid()) {
    *vallen = 0;
    return false;
  }

  bool status = false;

  TSMLoc const locfield = TSMimeHdrFieldFind(m_buffer, m_lochdr, keystr, keylen);

  if (nullptr != locfield) {
    int               getlen = 0;
    char const *const getstr = TSMimeHdrFieldValueStringGet(m_buffer, m_lochdr, locfield, index, &getlen);

    int const valcap = *vallen;
    if (nullptr != getstr && 0 < getlen && getlen < (valcap - 1)) {
      char *const endp = stpncpy(valstr, getstr, getlen);

      *vallen = endp - valstr;
      status  = (*vallen < valcap);

      if (status) {
        *endp = '\0';
      }
    }
    TSHandleMLocRelease(m_buffer, m_lochdr, locfield);
  } else {
    *vallen = 0;
  }

  return status;
}

bool
HttpHeader::setKeyVal(char const *const keystr, int const keylen, char const *const valstr, int const vallen, int const index)
{
  if (!isValid()) {
    return false;
  }

  bool status(false);

  TSMLoc locfield(TSMimeHdrFieldFind(m_buffer, m_lochdr, keystr, keylen));

  if (nullptr != locfield) {
    status = TS_SUCCESS == TSMimeHdrFieldValueStringSet(m_buffer, m_lochdr, locfield, index, valstr, vallen);
  } else {
    int rcode = TSMimeHdrFieldCreateNamed(m_buffer, m_lochdr, keystr, keylen, &locfield);

    if (TS_SUCCESS == rcode) {
      rcode = TSMimeHdrFieldValueStringSet(m_buffer, m_lochdr, locfield, index, valstr, vallen);
      if (TS_SUCCESS == rcode) {
        rcode  = TSMimeHdrFieldAppend(m_buffer, m_lochdr, locfield);
        status = (TS_SUCCESS == rcode);
      }
    }
  }

  if (nullptr != locfield) {
    TSHandleMLocRelease(m_buffer, m_lochdr, locfield);
  }

  return status;
}

bool
HttpHeader::timeForKey(char const *const keystr, int const keylen, time_t *const timeval) const
{
  TSAssert(nullptr != timeval);

  if (!isValid()) {
    *timeval = 0;
    return false;
  }

  bool status = false;

  TSMLoc const locfield = TSMimeHdrFieldFind(m_buffer, m_lochdr, keystr, keylen);

  if (nullptr != locfield) {
    *timeval = TSMimeHdrFieldValueDateGet(m_buffer, m_lochdr, locfield);
    TSHandleMLocRelease(m_buffer, m_lochdr, locfield);
  } else {
    *timeval = 0;
  }

  return status;
}

bool
HttpHeader::setKeyTime(char const *const keystr, int const keylen, time_t const timeval)
{
  if (!isValid()) {
    return false;
  }

  bool status(false);

  TSMLoc locfield(TSMimeHdrFieldFind(m_buffer, m_lochdr, keystr, keylen));

  if (nullptr == locfield) {
    DEBUG_LOG("Creating header %.*s", keylen, keystr);
    TSMimeHdrFieldCreateNamed(m_buffer, m_lochdr, keystr, keylen, &locfield);
  }

  if (nullptr != locfield) {
    if (TS_SUCCESS == TSMimeHdrFieldValueDateSet(m_buffer, m_lochdr, locfield, timeval)) {
      if (TS_SUCCESS == TSMimeHdrFieldAppend(m_buffer, m_lochdr, locfield)) {
        status = true;
        DEBUG_LOG("Set header %.*s to %jd", keylen, keystr, static_cast<intmax_t>(timeval));
      }
    }
    TSHandleMLocRelease(m_buffer, m_lochdr, locfield);
  }

  return status;
}

std::string
HttpHeader::toString() const
{
  std::string res;

  if (isValid()) {
    TSIOBuffer const iobufp = TSIOBufferCreate();
    TSHttpHdrPrint(m_buffer, m_lochdr, iobufp);
    TSIOBufferReader const reader = TSIOBufferReaderAlloc(iobufp);

    if (nullptr != reader) {
      TSIOBufferBlock block = TSIOBufferReaderStart(reader);
      bool            done  = false;
      while (!done && nullptr != block) {
        int64_t     avail    = 0;
        char const *blockptr = TSIOBufferBlockReadStart(block, reader, &avail);
        if (0 < avail) {
          res.append(blockptr, avail);
        }
        block = TSIOBufferBlockNext(block);
      }

      TSIOBufferReaderFree(reader);
    }

    TSIOBufferDestroy(iobufp);
  }

  if (res.empty()) {
    res = "<null>";
  }

  return res;
}

/////// HdrMgr

TSParseResult
HdrMgr::populateFrom(TSHttpParser const http_parser, TSIOBufferReader const reader, HeaderParseFunc const parsefunc,
                     int64_t *const bytes)
{
  TSParseResult parse_res = TS_PARSE_CONT;

  if (nullptr == m_buffer) {
    m_buffer = TSMBufferCreate();
  }
  if (nullptr == m_lochdr) {
    m_lochdr = TSHttpHdrCreate(m_buffer);
  }

  int64_t avail = TSIOBufferReaderAvail(reader);
  if (0 < avail) {
    TSIOBufferBlock block    = TSIOBufferReaderStart(reader);
    int64_t         consumed = 0;

    parse_res = TS_PARSE_CONT;

    while (nullptr != block && 0 < avail) {
      int64_t           blockbytes = 0;
      char const *const bstart     = TSIOBufferBlockReadStart(block, reader, &blockbytes);

      char const *ptr    = bstart;
      char const *endptr = ptr + blockbytes;

      parse_res = parsefunc(http_parser, m_buffer, m_lochdr, &ptr, endptr);

      int64_t const bytes_parsed(ptr - bstart);

      consumed += bytes_parsed;
      avail    -= bytes_parsed;

      if (TS_PARSE_CONT == parse_res) {
        block = TSIOBufferBlockNext(block);
      } else {
        break;
      }
    }
    TSIOBufferReaderConsume(reader, consumed);

    if (nullptr != bytes) {
      *bytes = consumed;
    }
  } else if (nullptr != bytes) {
    *bytes = 0;
  }

  return parse_res;
}
