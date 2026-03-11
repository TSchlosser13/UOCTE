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

#ifndef XML_HPP
#define XML_HPP

#include <functional>
#include <memory>

class xml
{

  struct impl;
  const std::unique_ptr<impl> m;

public:

  xml(std::function<void (const char *, const char **)> &&start,
    std::function<void (const char *)> &&end,
    std::function<void (const char *, std::size_t)> &&data);
 ~xml();

  void operator()(const char *data, std::size_t bufsize, bool isFinal) const;

};

#endif // inclusion guard
