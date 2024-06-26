.. Licensed to the Apache Software Foundation (ASF) under one or more
   contributor license agreements.  See the NOTICE file distributed
   with this work for additional information regarding copyright
   ownership.  The ASF licenses this file to you under the Apache
   License, Version 2.0 (the "License"); you may not use this file
   except in compliance with the License.  You may obtain a copy of
   the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
   implied.  See the License for the specific language governing
   permissions and limitations under the License.

.. include:: ../../../common.defs

.. default-domain:: cpp

TSHttpHdrStatusGet
******************

Synopsis
========

.. code-block:: cpp

    #include <ts/ts.h>

.. function:: TSHttpStatus TSHttpHdrStatusGet(TSMBuffer bufp, TSMLoc offset)

Description
===========

Retrieve the status code value from the HTTP response header identified by :arg:`bufp` and
:arg:`offset`. The value should be an enumeration value of :cpp:type:`TSHttpStatus`, although because
plugins can call :func:`TSHttpHdrStatusSet` this may not be true. If the header is not a valid
response then :enumerator:`TS_HTTP_STATUS_NONE` is returned.

See Also
========

:func:`TSHttpTxnClientRespGet`, :func:`TSHttpTxnServerRespGet`, :func:`TSHttpHdrTypeGet`.
