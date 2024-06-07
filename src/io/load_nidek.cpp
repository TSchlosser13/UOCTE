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
#include <cstring>
#include <iomanip>
#include <stdexcept>
#include <sstream>

#include "../core/oct_data.hpp"
#include "file.hpp"
#include "xml.hpp"

using namespace std;

namespace
{

  void mystart(string &cur, string &&, const char **)
  {
    cur.clear();
  }

  void myend(map<string, string> &m, string &cur, const string &name)
  {
    if (name == "Serial" || name == "Version" || name == "Model" ||
        name == "Product" || name == "Manufacture" ||
        name == "ScanPattern" || name == "ScanPointA" || name == "ScanCenterX" ||
        name == "ScanCenterY" || name == "ScanWidth1" ||
        name == "OCTDepthResolution" || name == "SLOPixelSpacing" || name == "CCDPixelSpacing" ||
        (m["ScanPattern"] == "MaculaMap" && (name == "ScanType" || name == "ScanPointB" || name == "ScanWidth2")))
      m[name] = cur;
    else if (name == "Eye")
      m["laterality"] = cur;
    else if (name == "ReleaseDate")
      m["scan date"] = cur;
  }

  void mydata(string &cur, const char *data, size_t len)
  {
    cur.append(data, len);
  }

  image<uint8_t> read_bmp(const string &path)
  {
    uint32_t start, width, height;

    file f(path.c_str(), "rb");
    f.set(10);
    f.fetch(start);
    f.set(18);
    f.fetch(width);
    f.fetch(height);

    image<uint8_t> img(1, width, height);

    // read and flip image
    f.set(start);
    for (size_t y = 0; y != height; ++y)
      f.read(&img(0, height - 1 - y), width);

    return img;
  }

  void load(const char *path, oct_subject &subject)
  {
    oct_scan &scan = subject.scans[""];
    {
      using placeholders::_1;
      using placeholders::_2;
      file f(path, "rb");
      string cur;
      xml x(bind(::mystart, ref(cur), _1, _2), bind(::myend, ref(scan.info), ref(cur), _1), bind(::mydata, ref(cur), _1, _2));
      char buf[1024];
      while (f)
      {
        size_t r = f.read(buf, sizeof(buf));
        x(buf, r, r == 0);
      }
    }

    string base(path, 0, strlen(path) - 5); // remove "x.xml"

    scan.fundus = read_bmp(base + ".bmp");

    if (scan.info["ScanPattern"] == "MaculaMap")
    {
      scan.size[2] = atof(scan.info["ScanWidth2"].c_str()) * 0.3f;
      scan.info.erase(scan.info.find("ScanWidth2"));

      size_t num_slices = atoi(scan.info["ScanPointB"].c_str());
      scan.info.erase(scan.info.find("ScanPointB"));
      for (size_t i = 0; i != num_slices; ++i)
      {
        ostringstream os;
        os.fill('0');
        os << base << "oct_c_" << setw(3) << i + 1 << ".bmp";
        image<uint8_t> img = read_bmp(os.str());

        if (i == 0)
          scan.tomogram = volume<uint8_t>(img.width(), img.height(), num_slices);
        else
        {
          if (img.width() != scan.tomogram.width() || img.height() != scan.tomogram.height())
            throw runtime_error("size mismatch in bmp image");
        }

        copy_n(&img(0, 0), img.width() * img.height(), &scan.tomogram(0, 0, i));
      }
    }
    else
    {
      scan.size[2] = 0;
      auto img = read_bmp(base + "oct_c_xh1.bmp");
      scan.tomogram = volume<uint8_t>(img.width(), img.height(), 1);
      copy_n(&img(0, 0), img.width() * img.height(), &scan.tomogram(0, 0, 0));
    }

    scan.size[0] = atof(scan.info["ScanWidth1"].c_str()) * 0.3f;
    scan.size[1] = atof(scan.info["OCTDepthResolution"].c_str()) * scan.tomogram.height() / 1000.0f;
    scan.info.erase(scan.info.find("ScanWidth1"));
    scan.info.erase(scan.info.find("OCTDepthResolution"));

    scan.range.minx = 1 + atoi(scan.info["ScanCenterX"].c_str()) - 0.5f * scan.size[0] / atof(scan.info["SLOPixelSpacing"].c_str());
    scan.range.miny = 1 + atoi(scan.info["ScanCenterY"].c_str()) - 0.5f * scan.size[2] / atof(scan.info["SLOPixelSpacing"].c_str());
    scan.range.maxx = 1 + atoi(scan.info["ScanCenterX"].c_str()) + 0.5f * scan.size[0] / atof(scan.info["SLOPixelSpacing"].c_str());
    scan.range.maxy = 1 + atoi(scan.info["ScanCenterY"].c_str()) + 0.5f * scan.size[2] / atof(scan.info["SLOPixelSpacing"].c_str());
    scan.info.erase(scan.info.find("ScanCenterX"));
    scan.info.erase(scan.info.find("ScanCenterY"));
    scan.info.erase(scan.info.find("SLOPixelSpacing"));
    scan.info.erase(scan.info.find("CCDPixelSpacing"));
    scan.info.erase(scan.info.find("ScanPointA"));

    file f((base + "oct_m.dat").c_str(), "rb");

    uint32_t num_slices, num_contours, size;
    f.set(24);
    f.fetch(num_slices);
    if (num_slices != scan.tomogram.depth())
      throw runtime_error("unexpected number of slices");

    f.fetch(size);
    num_contours = (size - 12) / sizeof(uint16_t) / scan.tomogram.width();
    vector<image<float> *> images(num_contours);
    for (size_t k = 0; k != num_contours; ++k)
    {
      ostringstream os;
      os << "CONTOUR" << k;
      images[k] = &scan.contours[os.str()];
      *images[k] = image<float>(1, scan.tomogram.width(), scan.tomogram.depth());
    }

    unique_ptr<uint16_t []> p(new uint16_t[scan.tomogram.width()]);
    for (size_t z = 0; z != num_slices; ++z)
    {
      f.set(f.pos() + 12);
      for (size_t k = 0; k != num_contours; ++k)
      {
        f.read(p.get(), scan.tomogram.width());
        copy_n(&p[0], scan.tomogram.width(), images[k]->data() + z * scan.tomogram.width());
      }
    }
  }

  oct_reader r("Nidek OCT", {"x.xml"}, load);
}
