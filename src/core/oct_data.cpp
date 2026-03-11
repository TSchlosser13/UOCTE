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

#include "oct_data.hpp"

#include <algorithm>
#include <cstring>

using namespace std;

namespace
{
  struct reader_data
  {
    string name;
    vector<string> extensions;
    function<void (const char *, oct_subject &s)> read;
  };

  map<oct_reader *, reader_data> &oct_readers()
  {
    static map<oct_reader *, reader_data> readers;
    return readers;
  }
}

oct_subject::oct_subject(const char *path)
{
  string msg;

  // try to open file with each reader handling the extension
  auto &readers = oct_readers();
  for (auto &p : readers)
  {
    bool can_read = false;
    for (auto &s : p.second.extensions)
      if (strlen(path) >= s.length() && path + strlen(path) - s.length() == s)
        can_read = true;

    if (can_read)
    {
      try
      {
        p.second.read(path, *this);
        return;
      }
      catch (exception &e)
      {
        msg += p.second.name + ": " + e.what() + "\n";
      }
    }
  }

  throw runtime_error(msg + "Could not read the file.");
}

oct_reader::oct_reader(std::string &&name, std::vector<std::string> &&extensions, std::function<void (const char *, oct_subject &s)> &&read)
{
  oct_readers().insert(make_pair(this, reader_data{move(name), move(extensions), move(read)}));
}

oct_reader::~oct_reader()
{
  oct_readers().erase(oct_readers().find(this));
}

void foreach_oct_reader(const std::function<void (const std::string &, const std::vector<std::string> &)> &fn)
{
  for (auto &p: oct_readers())
    fn(p.second.name, p.second.extensions);
}
