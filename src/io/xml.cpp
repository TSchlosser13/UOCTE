/*
 * Copyright 2015 TU Chemnitz
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "xml.hpp"

#include <sstream>
#include <stdexcept>

#include <expat.h>

using namespace std;

struct xml::impl
{
  XML_Parser p;
  std::function<void (const char *, const char **)> start;
  std::function<void (const char *)> end;
  std::function<void (const char *, std::size_t)> data;
  
  static void xml_start(void *userData, const XML_Char *name, const XML_Char **atts);
  static void xml_end(void *userData, const XML_Char *name);
  static void xml_data(void *userData, const XML_Char *data, int len);
};

void xml::impl::xml_start(void *userData, const XML_Char *name, const XML_Char **attr)
{
  static_cast<impl *>(userData)->start(name, attr);
}

void xml::impl::xml_end(void *userData, const XML_Char *name)
{
  static_cast<impl *>(userData)->end(name);
}

void xml::impl::xml_data(void *userData, const XML_Char *data, int len)
{
  static_cast<impl *>(userData)->data(data, len);
}

xml::xml(function<void (const char *, const char **)> &&start, function<void (const char *)> &&end, function<void (const char *, size_t)> &&data)
  : m(new impl{XML_ParserCreate(NULL), move(start), move(end), move(data)})
{
  XML_SetUserData(m->p, m.get());
  XML_SetElementHandler(m->p, impl::xml_start, impl::xml_end);
  XML_SetDefaultHandler(m->p, impl::xml_data);
}

xml::~xml()
{
  XML_ParserFree(m->p);
}

void xml::operator()(const char *buf, size_t bufsize, bool is_final) const
{
  if (!XML_Parse(m->p, buf, bufsize, is_final))
  {
    ostringstream os;
    os << XML_GetCurrentLineNumber(m->p) << ":" << XML_GetCurrentColumnNumber(m->p) << ": error: " << XML_ErrorString(XML_GetErrorCode(m->p));
    throw runtime_error(os.str());
  }
}
