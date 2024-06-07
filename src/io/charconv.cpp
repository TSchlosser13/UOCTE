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

#include "charconv.hpp"

using namespace std;

string latin1_to_utf8(const string &s)
{
  string o;
  for (unsigned char c: s)
    if (c < 128)
      o.push_back(c);
    else
    {
      o.push_back(0xc2 + (c>0xbf));
      o.push_back(0x80 + (c&0x3f));
    }

  return o;
}
