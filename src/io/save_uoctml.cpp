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

#include "exportJpeg.hpp"
#include "save_uoctml.hpp"

#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace std;

namespace
{

  struct file
  {
    ofstream m;
    file(const string &path, ios_base::openmode mode = ios_base::out) : m(path, mode)
    {
      if (!m)
        throw runtime_error(path + string(": error opening file"));
    }

   ~file()
    {
      m.close();
    }

    template <class T>
    file &operator<<(const T& v)
    {
      m << v;
      return *this;
    }

  };

}

void save_uoctml(const char *path, const oct_subject &subject, bool anonymize)
{
  size_t lastsep = string(path).rfind('/')+1;
  #ifdef _WIN32
  lastsep = max(lastsep, string(path).rfind('\\')+1);
  #endif

  string base(path, lastsep, string::npos);
  file o(path);
  file bin(path + string(".bin"), ios_base::binary);

  o << "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>\n";
  o << "<uoctml version=\"1.0\">\n";
  for (const auto &e: subject.info)
    if (!anonymize || e.first != "name")
      o << "  <info><key>" << e.first << "</key><value>" << e.second << "</value></info>\n";

  for (const auto &scan: subject.scans)
  {
    string path_slices(path + string("_"));
    std::vector<int> contourList;
    QColor contourColor;

    exportSlicesAsJpeg(&scan.second, path_slices, contourList, contourColor);


    o << "  <scan>\n";
    o << "    <id>" << scan.first << "</id>\n";
    for (const auto &e: scan.second.info)
      o << "    <info><key>" << e.first << "</key><value>"
        << e.second << "</value></info>\n";

    const size_t fc = scan.second.fundus.channels();
    const size_t fw = scan.second.fundus.width();
    const size_t fh = scan.second.fundus.height();

    o << "    <fundus channels=\"" << fc << "\" width=\"" << fw << "\" height=\"" << fh << "\" type=\"u8\">\n";
    o << "      <data storage=\"raw\" start=\"" << bin.m.tellp()
      << "\" size=\"" << fc * fw * fh << "\">" << base << ".bin</data>\n";
    o << "    </fundus>\n";

    bin.m.write(reinterpret_cast<const char *>(scan.second.fundus.data()), fc * fw * fh);

    o << "    <range minx=\"" << scan.second.range.minx
      << "\" miny=\"" << scan.second.range.miny
      << "\" maxx=\"" << scan.second.range.maxx
      << "\" maxy=\"" << scan.second.range.maxy << "\"/>\n";

    const size_t sw = scan.second.tomogram.width();
    const size_t sh = scan.second.tomogram.height();
    const size_t sd = scan.second.tomogram.depth();

    o << "    <size x=\"" << scan.second.size[0]
      << "\" y=\"" << scan.second.size[1]
      << "\" z=\"" << scan.second.size[2] << "\"/>\n";

    o << "    <tomogram width=\"" << sw << "\" height=\"" << sh
      << "\" depth=\"" << sd << "\" type=\"u8\">\n";
    o << "      <data storage=\"raw\" start=\"" << bin.m.tellp()
      << "\" size=\"" << sw * sh * sd << "\">" << base << ".bin</data>\n";
    o << "    </tomogram>\n";

    bin.m.write(reinterpret_cast<const char *>(scan.second.tomogram.data()), sw * sh * sd);

    for (const auto &c: scan.second.contours)
    {
      const size_t w = c.second.width();
      const size_t h = c.second.height();

      o << "    <contour width=\"" << w << "\" height=\"" << h << "\" type=\"f32\">\n";
      o << "      <name>" << c.first << "</name>\n";
      o << "      <data storage=\"raw\" start=\"" << bin.m.tellp()
        << "\" size=\"" << w * h * sizeof(float) << "\">" << base << ".bin</data>\n";
      o << "    </contour>\n";

      bin.m.write(reinterpret_cast<const char *>(c.second.data()), w * h * sizeof(float));
    }

    o << "  </scan>\n";
  }

  o << "</uoctml>\n";
}
