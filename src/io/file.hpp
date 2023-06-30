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

#ifndef FILE_HPP
#define FILE_HPP

#include <cstdio>
#include <stdexcept>
#include <cstring>

#include <qDebug>

/// convenient read access to a file
class file
{
  FILE *m;
  size_t charsIgnored;

public:

  /// open file
  file(const char *path, const char *mode)
    : m(fopen(path, mode)),
      charsIgnored(0)
  {
    if (!m)
      throw std::runtime_error(std::string("could not open \"") + path + "\"");
  }

  /// close file
 ~file()
  {
    fclose(m);
  }

  file(const file&) = delete;
  file& operator=(const file&) = delete;
  
  /// find given string in file
  void ignoreUntil(const char* t)
  {
      if (t == "")
          return;

      int c; //current character got from file
      int i = 0; //position in t
      do {
          c = fgetc(m);
          //if a letter equals, try next, else word was not found
          i = (c == t[i]) ? ++i : 0;
          //qDebug() << QString("compare %1 with %2, --> i = %3 - at pos %4").arg(c).arg(t[i-1]).arg(i).arg(ftell(m));
          if (i == std::strlen(t)) { //found
              fseek(m, ftell(m)-strlen(t), SEEK_SET);
              charsIgnored = ftell(m);
              return;
          }
      } while(c != EOF);
  }

  /// fetch binary value from file
  template <class T>
  bool fetch(T &t)
  {
    return fread(&t, sizeof(T), 1, m) == 1;
  }

  /// read multiple bytes into buffer
  template <class T>
  size_t read(T *t, size_t n)
  {
    return fread(t, sizeof(T), n, m);
  }

  /// current file position
  size_t pos() const
  {
    return ftell(m);
  }

  /// set file position
  bool set(size_t pos)
  {
    return fseek(m, pos + charsIgnored, SEEK_SET) == 0;
  }

  /// file ok?
  operator const void*() const
  {
    return !feof(m) && !ferror(m) ? this : 0;
  }

};

#endif // inclusion guard
