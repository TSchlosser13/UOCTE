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

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>

#include "../core/oct_data.hpp"
#include "charconv.hpp"
#include "file.hpp"
#include "j2k.hpp"

#include <iostream>

using namespace std;

namespace
{

  image<uint8_t> read_jp2_image(file &f, size_t channels, size_t width, size_t height)
  {
    uint32_t size;
    f.fetch(size);
    unique_ptr<char []> buffer(new char[size]);
    f.read(buffer.get(), size);

    image<uint8_t> img = decode_j2k(buffer.get(), size);
    if (img.channels() != channels || img.width() != width || img.height() != height)
      throw runtime_error("size mismatch in jpeg2000 image");

    return img;
  }

  template <class S_T, class T>
  void read_raw(file &f, size_t width, size_t height, image<T> &v)
  {
    unique_ptr<S_T []> d(new S_T[width * height]);
    S_T *p = d.get(), *q = p + width * height;
    while (f && p != q)
      p += f.read(p, q - p);

    v = image<T>(1, width, height);
    copy_n(d.get(), width * height, v.data());
  }

  struct date
  {
    uint16_t year, month, day;
  };

  struct date_time
    : public date
  {
    uint16_t hour, minute, second;
  };

  struct bbox
  {
    uint32_t minx, miny, maxx, maxy;
  };

  struct bcircle
  {
    uint32_t x, y, r, zero;
  };

  void load(const char *path, oct_subject &subject)
  {
    oct_scan &scan = subject.scans[""];
    file f(path, "rb");

    char tag[32], s[512], type;

    // read header
    f.read(s, 7);
    if (strncmp(s, "FOCT", 4) != 0)
      throw runtime_error(string("not a valid FDA file: \"") + path + "\"");

    if (strncmp(s+4, "FDA", 3) == 0)
    {
      scan.info["fixation"] = "macula";
    }
    else if (strncmp(s+4, "FAA", 3) == 0)
    {
      scan.info["fixation"] = "external";
    }

    f.read(s, 4*2); /* 0x00000002 0x000003e8 */

    while (true)
    {
      uint32_t resume, size, bpp, width, height, depth;

      uint8_t tsize;
      f.fetch(tsize); /* chunk name length */

      if (tsize == 0) /* no chunk */
        break;

      f.read(tag, tsize); /* chunk name */
      tag[tsize] = 0; /* add zero terminator */
      f.fetch(size); /* chunk size */

      resume = f.pos() + size;

      if (strcmp(tag, "@ALIGN_INFO") == 0)
      {
        uint16_t x;
        uint32_t num_slices, align_info[5];
        f.fetch(x); /* 0 */
        f.fetch(num_slices);
        f.fetch(align_info);
        f.read(s, 32); /* "8.0.1.2204" */
      }
      else if (strcmp(tag, "@ANTERIOR_CALIB_INFO") == 0)
      {
        f.read(s, 16); /* "2.1.0" */
        f.set(f.pos() + 84 * 8); /* doubles */
      }
      else if (strcmp(tag, "@CAPTURE_INFO_02") == 0)
      {
        uint16_t x;
        f.fetch(x); // have_fundus = x & 0x200;
        f.read(s, 0x68); /* 0 */
        date_time aquisition;
        f.fetch(aquisition);
        ostringstream o;
        o.fill('0');
        o << aquisition.year << "/" << setw(2) << aquisition.month << "/" << setw(2) << aquisition.day << " ";
        o << setw(2) << aquisition.hour << ":" << setw(2) << aquisition.minute << ":" << setw(2) << aquisition.second;
        scan.info["scan date"] = o.str();
      }
      else if (strcmp(tag, "@CONTOUR_INFO") == 0)
      {
        uint16_t x;
        f.read(s, 20);
        f.fetch(x); /* x=0 -> RETINA_* or FLA_*, values are uint16_t, x=0x100 -> CORNEA_*, values are doubles */
        f.fetch(width);
        f.fetch(height);
        f.fetch(size);
        s[20] = 0;
        string cid = latin1_to_utf8(s);
        if (size == width * height * 2)
          ::read_raw<uint16_t>(f, width, height, scan.contours[cid]);
        else if (size == width * height * 8)
          ::read_raw<double>(f, width, height, scan.contours[cid]);
        else
          throw runtime_error("unexpected image parameters");

        // flip contours
        for (size_t y = 0; y != height / 2; ++y)
          swap_ranges(&scan.contours[cid](0, y), &scan.contours[cid](0, y+1), &scan.contours[cid](0, height - 1 - y));

        // invert contours
        for (size_t y = 0; y != height; ++y)
          for (size_t x = 0; x != width; ++x)
            scan.contours[cid](x, y) = scan.tomogram.height() - scan.contours[cid](x, y);

        f.read(s, 32); /* "8.0.1.20198" */
      }
      else if (strcmp(tag, "@EFFECTIVE_SCAN_RANGE") == 0)
      {
        bbox ranges_fundus, ranges_trc;
        f.fetch(ranges_fundus);
        f.fetch(ranges_trc);
      }
      else if (strcmp(tag, "@FAST_Q2_INFO") == 0)
      {
        float fast_q2[14];
        f.fetch(fast_q2);
      }
      else if (strcmp(tag, "@FDA_FILE_INFO") == 0)
      {
        f.read(s, 2*4); /* 0x00000002 0x000003e8 */
        f.read(s, 32); /* "8.0.1.20198 */
      }
      else if (strcmp(tag, "@GLA_LITTMANN_01") == 0)
      {
        f.read(s, 13*4); /* 0 .. 0, -1, 1 */
      }
      else if (strcmp(tag, "@HW_INFO_03") == 0)
      {
        f.read(s, 16); scan.info["hw model"] = latin1_to_utf8(s);
        f.read(s, 16); scan.info["hw serial"] = latin1_to_utf8(s);
        f.read(s, 32); /* 0 */
        f.read(s, 16); scan.info["hw versions[0]"] = latin1_to_utf8(s);
        date_time build;
        f.fetch(build);
        ostringstream o;
        o.fill('0');
        o << build.year << "/" << setw(2) << build.month << "/" << setw(2) << build.day << " ";
        o << setw(2) << build.hour << ":" << setw(2) << build.minute << ":" << setw(2) << build.second;
        scan.info["hw build date"] = o.str();
        f.read(s, 8); /* 0 */
        f.read(s, 16); scan.info["hw versions[1]"] = latin1_to_utf8(s);
        f.read(s, 16); scan.info["hw versions[2]"] = latin1_to_utf8(s);
        f.read(s, 16); scan.info["hw versions[3]"] = latin1_to_utf8(s);
        f.read(s, 16); scan.info["hw versions[4]"] = latin1_to_utf8(s);
        f.read(s, 16); scan.info["hw versions[5]"] = latin1_to_utf8(s);
      }
      else if (strcmp(tag, "@IMG_FUNDUS") == 0)
      {
        f.fetch(width);
        f.fetch(height);
        f.fetch(bpp); /* 0x00000018 */
        f.fetch(depth); /* 0x00000001 */
        f.read(s, 4); /* 0x00000a02 */
        if (depth != 1)
          throw runtime_error("unexpected image parameters");

        auto i = ::read_jp2_image(f, bpp / 8, width, height);

        // swap red and blue channels
        for (size_t y = 0; y != height; ++y)
          for (size_t x = 0; x != width; ++x)
            for (size_t c = 0; c != 3; ++c)
              swap(i(c, x, y), i(2 - c, x, y));
      }
      else if (strcmp(tag, "@IMG_JPEG") == 0)
      {
        f.fetch(type);
        switch (type)
        {
          case 0: scan.info["type"] = "line"; break;
          case 2: scan.info["type"] = "volume"; break;
          case 3: scan.info["type"] = "radial line"; break;
          case 7: scan.info["type"] = "7 line"; break;
          case 11: scan.info["type"] = "5 line cross"; break;
        }
        uint32_t img_jpeg_param[2];
        f.fetch(img_jpeg_param);
        f.fetch(width);
        f.fetch(height);
        f.fetch(depth);
        f.read(s, 4); /* 0x00000a02 */

        scan.tomogram = volume<uint8_t>(width, height, depth);
        for (size_t z = 0; z != depth; ++z)
        {
          uint32_t size;
          f.fetch(size);
          unique_ptr<char []> buffer(new char[size]);
          f.read(buffer.get(), size);

          image<uint8_t> img = decode_j2k(buffer.get(), size);
          if (img.channels() != 1 || img.width() != width || img.height() != height)
            throw runtime_error("size mismatch in jpeg2000 image");

          copy_n(img.data(), width * height, &scan.tomogram(0, 0, z));
        }
      }
      else if (strcmp(tag, "@IMG_MOT_COMP_03") == 0)
      {
        f.read(s, 1); /* 0x00 */
        f.fetch(width);
        f.fetch(height);
        f.fetch(bpp); /* 0x00000010 */
        f.fetch(depth); /* 0x00000009 */
        f.read(s, 17); /* 17x 0x00 */
        f.fetch(size);
        if (bpp == 16 && size == bpp / 8 * width * height * depth)
          f.set(f.pos() + size);
          //::read_raw<uint16_t>(f, width * height * depth, mot);
        else
          throw runtime_error("unexpected image parameters");
      }
      else if (strcmp(tag, "@IMG_PROJECTION") == 0)
      {
        f.fetch(width);
        f.fetch(height);
        f.fetch(bpp); /* 0x00000008 */
        f.read(s, 4); /* 0x01000002 */
        ::read_jp2_image(f, bpp / 8, width, height);
      }
      else if (strcmp(tag, "@IMG_TRC_02") == 0)
      {
        f.fetch(width);
        f.fetch(height);
        f.fetch(bpp); /* 0x00000018 */
        f.fetch(depth); /* 0x00000002 */
        f.read(s, 1); /* 0x01 */
        for (size_t i = 0; i != depth; ++i)
          scan.fundus = ::read_jp2_image(f, bpp / 8, width, height);
      }
      else if (strcmp(tag, "@MAIN_MODULE_INFO") == 0)
      {
        f.read(s, 0x80); /* 3DOCT-2000.exe */
        f.read(s, 4*2); /* 8,0,1,0x5616 */
        f.read(s, 0x80); /* 0 */
      }
      else if (strcmp(tag, "@PARAM_OBS_02") == 0)
      {
        int16_t x;
        f.fetch(x); /* 1 */
        f.fetch(x); /* -1 */
        f.fetch(x); /* -1 */
        f.read(s, 12); /* "D7000" */
        f.read(s, 24); /* "JPEG Fine */
        f.fetch(x); /* 0x300 */
        f.fetch(x); /* 1 */
        f.fetch(x); /* 0 */
        f.read(s, 24); /* "Color Temperature" */
        f.fetch(x); /* 0x2014 */
        f.read(s, 12); /* 0 */
        float param_obs;
        f.fetch(param_obs);
      }
      else if (strcmp(tag, "@PARAM_SCAN_04") == 0)
      {
        uint16_t param_scan1[6];
        double param_res[3];
        double param_scan2[2];
        f.fetch(param_scan1);
        f.fetch(param_res);
        f.fetch(param_scan2);
        scan.size[0] = param_res[0];
        scan.size[1] = param_res[2];
        scan.size[2] = param_res[1];
      }
      else if (strcmp(tag, "@PARAM_TRC_02") == 0)
      {
        uint16_t param_trc[2];
        f.fetch(param_trc);
      }
      else if (strcmp(tag, "@PATIENTEXT_INFO") == 0)
      {
        f.read(s, 64); /* 0 */
      }
      else if (strcmp(tag, "@PATIENT_INFO_02") == 0)
      {
        uint8_t x;
        f.read(s, 32); subject.info["ID"] = latin1_to_utf8(s);
        f.read(s, 32); subject.info["name"] = latin1_to_utf8(s);
        f.read(s, 32); subject.info["name"] = latin1_to_utf8(s) + ", " + subject.info["name"];
        f.read(s, 8); /* 0 */
        f.fetch(x);
        bool have_dates = ((x & 2) == 0);
        date birth;
        f.fetch(birth);
        ostringstream o;
        o.fill('0');
        o << birth.year << "/" << setw(2) << birth.month << "/" << setw(2) << birth.day;
        if (have_dates)
          subject.info["birth date"] = o.str();
        f.read(s, 504); /* mostly 0, study date somewhere in there */
      }
      else if (strcmp(tag, "@REGIST_INFO") == 0)
      {
        uint8_t x;
        f.fetch(x); /* 0 */
        int32_t regist_info[2];
        f.fetch(regist_info);
        bbox regist_fundus;
        f.fetch(regist_fundus);
        f.read(s, 32); /* "8.0.1.20198" */
        bbox regist_trc;
        f.fetch(regist_trc);
        double d[4];
        f.read(d, 4); /* 1.0 */
        f.read(s, 0x30); /* 0 */
        if (regist_trc.maxy != 0)
        {
          scan.range.minx = regist_trc.minx;
          scan.range.miny = regist_trc.miny;
          scan.range.maxx = regist_trc.maxx;
          scan.range.maxy = regist_trc.maxy;
        }
        else
        {
          scan.range.minx = regist_trc.minx - regist_trc.maxx;
          scan.range.miny = regist_trc.miny - regist_trc.maxx;
          scan.range.maxx = regist_trc.minx + regist_trc.maxx;
          scan.range.maxy = regist_trc.miny + regist_trc.maxx;
        }
      }
      else if (strcmp(tag, "@REPORT_INFO") == 0)
      {
        f.read(s, 7); /* 0 */
      }
      else if (strcmp(tag, "@RESULT_CORNEA_CURVE") == 0)
      {
        f.read(s, 20); /* name of cornea curve */
        s[20] = 0;
        //image<double> &i = cornea_curve[s];
        //image<double> &ip = cornea_curve_param[s];
        f.fetch(width);
        f.fetch(depth);
        f.read(s, 32); /* "8.0.1.21781" */
        //ip.set_size(1, 1, 1, depth);
        //f.read(&ip(0, 0, 0, 0), depth);
        f.set(f.pos() + depth * sizeof(double));
        //i.set_size(1, width, 1, depth);
        //f.read(&i(0, 0, 0, 0), width * depth);
        f.set(f.pos() + width * depth * sizeof(double));
      }
      else if (strcmp(tag, "@RESULT_CORNEA_THICKNESS") == 0)
      {
        f.read(s, 32); /* "8.0.1.21781" */
        f.read(s, 20); /* "CORNEA" or "CORNEALEPI" */
        s[20] = 0;
        //image<double> &i = cornea_thickness[s];
        f.fetch(width);
        f.fetch(depth);
        //i.set_size(1, width, 1, depth);
        //f.read(&i(0, 0, 0, 0), width * depth);
        f.set(f.pos() + width * depth * sizeof(double));
      }
      else if (strcmp(tag, "@THUMBNAIL") == 0)
      {
        f.fetch(size);
        f.set(f.pos() + size);
      }

      f.set(resume);
    }

    scan.size[1] *= scan.tomogram.height() / 1000.0f;
  }

  oct_reader r("Topcon OCT", {".fda"}, load);
}
