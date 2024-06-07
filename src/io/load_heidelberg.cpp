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

#include <qDebug>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "../core/oct_data.hpp"
#include "charconv.hpp"
#include "file.hpp"

using namespace std;

namespace
{

  /// convert Julian date
  void date(int J, int &year, int &month, int &day)
  {
    int f = J + 1401 + (((4 * J + 274277) / 146097) * 3) / 4 - 38;
    int e = 4 * f + 3;
    int g = (e % 1461) / 4;
    int h = 5 * g + 2;
    day = (h % 153) / 5 + 1;
    month = (h / 153 + 2) % 12 + 1;
    year = (e / 1461) - 4716 + (12 + 2 - month) / 12;
  }

  /// unsigned 16-bit float helper
  struct ufloat16
  {
    uint16_t m:10;
    uint16_t e:6;

    operator float() const
    {
      if (e == 0)
        return ldexp(m / float(1<<10), -62);
      else if (e == 63)
        return 0.0f;
      else
        return ldexp(1.0f + m / float(1<<10), int(e)-63);
    }
  };

  typedef struct
  {
    char magic[12];
    uint32_t v_100;
    uint16_t minus1[9], zero;
  } mdb_header_t;

  typedef struct
  {
    char magic[12];
    uint32_t v_100;
    uint16_t minus1[9], zero_1;
    uint32_t count, cur, prev, id;
  } mdb_dir_t;

  typedef struct
  {
    uint32_t pos, start, size, zero_1, patient_id, study_id, series_id, slice_id;
    uint16_t e, zero_2;
    uint32_t tag, id;
  } mdb_dirent_t;

  typedef struct
  {
    char magic[12];
    uint32_t a, b, pos, size, zero, patient_id, study_id, series_id, slice_id;
    uint16_t ind, f;
    uint32_t tag, id;
  } chunk_t;

  const char magic1[] = {0x43, 0x4D, 0x44, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  const char magic2[] = {0x4D, 0x44, 0x62, 0x4D, 0x44, 0x69, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00};
  const char magic3[] = {0x4D, 0x44, 0x62, 0x44, 0x69, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  const char magic4[] = {0x4D, 0x44, 0x62, 0x44, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00};

  void load(const char *path, oct_subject &subject)
  {
    map<string, oct_scan> &scans = subject.scans;
    map<string, string> &info = subject.info;

    file f(path, "r+b");
    
    //ignore information before header
    f.ignoreUntil("CMDb");
    
    mdb_header_t h;
    f.fetch(h);
    if (strncmp(h.magic, magic1, sizeof(magic1)) != 0 || h.v_100 != 100)
      throw runtime_error("error reading e2e header");

    mdb_dir_t d;
    f.fetch(d);
    if (strncmp(d.magic, magic2, sizeof(magic2)) != 0 || d.v_100 != 100)
      throw runtime_error("error reading e2e directory");

    // get all file positions of directory headers
    vector<uint32_t> dirs;
    uint32_t cur = d.cur;
    do
    {
      dirs.push_back(cur);
      f.set(dirs.back());
      f.fetch(d);
      if (strncmp(d.magic, magic3, sizeof(magic3)) != 0 || d.v_100 != 100)
        throw runtime_error("error reading e2e directory");

      cur = d.prev;
    } while (cur != 0);

    // get all chunks by traversing all dirs
    vector<uint32_t> chunks;
    map<uint32_t, uint32_t> num_slices;
    while (!dirs.empty())
    {
      f.set(dirs.back());
      f.fetch(d);
      dirs.pop_back();

      for (size_t i = 0; i != d.count; ++i)
      {
        mdb_dirent_t e;
        f.fetch(e);

        if (e.series_id != 0xffffffff)
          num_slices[e.series_id] = max(num_slices[e.series_id], (e.slice_id + 2) / 2);

        if (e.start > e.pos)
          chunks.push_back(e.start);
      }
    }

    for (uint32_t pos: chunks)
    {
      f.set(pos);
      chunk_t c;
      if (!f.fetch(c))
        throw runtime_error("read chunk error");

      if (strncmp(c.magic, magic4, sizeof(magic4)) != 0)
        throw runtime_error("chunk header error");

      if (c.tag == 0x40000000) // image data
      {
        ostringstream o;
        o << c.series_id;
        oct_scan &s = scans[o.str()];
        uint32_t size, x, y, height, width;
        f.fetch(size);
        f.fetch(x);
        f.fetch(y);
        f.fetch(height);
        f.fetch(width);

        if (c.ind == 0) // fundus image
        {
          s.fundus = image<uint8_t>(1, width, height);
          f.read(s.fundus.data(), width * height);

          // guessed from XML files
          s.range.minx = width / 6;
          s.range.maxx = 5 * width / 6;
          s.range.miny = height / 4;
          s.range.maxy = 3 * height / 4;

          s.size[0] = 6;
          s.size[1] = 492 * 0.0039;
          s.size[2] = 4.5;
        }
        else // normal image
        {
          if (c.slice_id > num_slices[c.series_id] * 2)
            throw runtime_error("broken slice sequence");

          if (!s.tomogram.data())
            s.tomogram = volume<uint8_t>(width, height, num_slices[c.series_id]);

          unique_ptr<ufloat16 []> p(new ufloat16[width*height]);
          f.read(p.get(), width*height);

          // convert and flip
          uint8_t *q = &s.tomogram(0, 0, num_slices[c.series_id] - 1 - c.slice_id / 2);
          for (size_t y = 0; y != height; ++y)
            for (size_t x = 0; x != width; ++x)
              *q++ = 256 * pow(p[y*width+x], 1.0f / 2.4f);
        }
      }
      else if (c.tag == 0x00002723) // contour data
      {
        ostringstream o;
        o << c.series_id;
        oct_scan &s = scans[o.str()];
        if (c.slice_id > num_slices[c.series_id] * 2)
          throw runtime_error("broken slice sequence");

        uint32_t dummy, name, width;
        f.fetch(dummy);
        f.fetch(name);
        f.fetch(dummy);
        f.fetch(width);

        ostringstream os;
        os << "CONTOUR" << name;

        image<float> &img = s.contours[os.str()];
        if (!img.data())
          img = image<float>(1, width, num_slices[c.series_id]);

        unique_ptr<float []> p(new float[width]);
        f.read(p.get(), width);

        // convert and flip contours
        float *q = &img(0, num_slices[c.series_id] - 1 - c.slice_id / 2);
        for (size_t x = 0; x != width; ++x)
          *q++ = (p[x] != numeric_limits<float>::max() && p[x] != 0.0 ? p[x] : 0.0 / 0.0); // simple hack to drop triangles using invalid values
      }
      else if (c.tag == 0x00000009)
      {
        char s[67]; uint32_t birthday;
        f.read(s, 31);
        s[31] = 0;
        info["name"] = latin1_to_utf8(s);

        f.read(s, 66); s[66] = 0;
        if (strlen(s))
          info["name"] = latin1_to_utf8(s) + ", " + info["name"];

        f.fetch(birthday);
        {
          int year, month, day;
          ostringstream o;
          o.fill('0');
          date(birthday/64 - 14558805, year, month, day);
          o << year << "/" << setw(2) << month << "/" << setw(2) << day;
          info["birth date"] = o.str();
        }

        f.read(s, 1);
        s[1] = 0;
        info["sex"] = s;

        ostringstream o;
        o << c.patient_id;
        info["ID"] = o.str();
      }
      else if (c.tag == 0x0000000B)
      {
        ostringstream o;
        o << c.series_id;
        oct_scan &s = scans[o.str()];
        char v[14];
        f.fetch(v);
        f.read(v, 1); v[1] = 0;
        s.info["laterality"] = latin1_to_utf8(v);
      }
    }
  }

  oct_reader regist("Heidelberg Spectralis OCT", {".e2e", ".E2E"}, load);
}
