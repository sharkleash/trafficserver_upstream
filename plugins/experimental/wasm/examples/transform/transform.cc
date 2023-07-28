/**
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

#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class ExampleRootContext : public RootContext
{
public:
  explicit ExampleRootContext(uint32_t id, std::string_view root_id) : RootContext(id, root_id) {}

  bool onStart(size_t) override;
};

class ExampleContext : public Context
{
public:
  explicit ExampleContext(uint32_t id, RootContext *root) : Context(id, root) {}

  FilterHeadersStatus onRequestHeaders(uint32_t headers, bool end_of_stream) override;
  FilterDataStatus onResponseBody(size_t body_buffer_length, bool end_of_stream) override;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext), ROOT_FACTORY(ExampleRootContext),
                                                      "myproject");

bool
ExampleRootContext::onStart(size_t)
{
  logInfo(std::string("onStart"));
  return true;
}

FilterHeadersStatus
ExampleContext::onRequestHeaders(uint32_t headers, bool end_of_stream)
{
  // print UA
  auto ua = getRequestHeader("User-Agent");
  logInfo(std::string("UA ") + std::string(ua->view()));

  return FilterHeadersStatus::Continue;
}

FilterDataStatus
ExampleContext::onResponseBody(size_t body_buffer_length, bool end_of_stream)
{
  logInfo(std::string("inside onResponseBody"));

  size_t buffered_size;
  uint32_t flags;
  getBufferStatus(WasmBufferType::HttpResponseBody, &buffered_size, &flags);
  auto body = getBufferBytes(WasmBufferType::HttpResponseBody, 0, buffered_size);
  logError(std::string("onBody ") + std::string(body->view()));

  return FilterDataStatus::Continue;
}
