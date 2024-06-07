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
#include <stdexcept>

#include "../core/oct_data.hpp"
#include "file.hpp"
#include "xml.hpp"

using namespace std;

namespace
{

  struct parse
  {
    oct_subject &subject;
    const string dirname;
    string cur, scan_id, key, value, cname, path;
    map<string, string> info, *cur_info;
    bounding_box scan_range;
    float scan_size[3];
    size_t fundus_channels, fundus_width, fundus_height;
    size_t tomogram_width, tomogram_height, tomogram_depth;
    size_t contour_width, contour_height;
    size_t pos, length;
    pair<string, size_t> fundus_pos, tomogram_pos;
    map<string, tuple<string, size_t, size_t, size_t>> contour_pos;

    parse(oct_subject &subject, const string &dirname)
      : subject(subject), dirname(dirname), cur_info(&subject.info)
    {
    }

    const char *find(const char **attr, const string &key)
    {
      while (*attr && key != *attr)
        attr += 2;

      if (!*attr)
        throw runtime_error("missing attribute \"" + key + "\"");

      return attr[1];
    }

    void start(const string &name, const char **attr)
    {
      cur.clear();
      try
      {
        if (name == "uoctml")
        {
          if (find(attr, "version") != string("1.0"))
            throw runtime_error("unsupported version");
        }
        else if (name == "scan")
        {
          cur_info = &info;
        }
        else if (name == "fundus")
        {
          fundus_channels = atoi(find(attr, "channels"));
          fundus_width = atoi(find(attr, "width"));
          fundus_height = atoi(find(attr, "height"));
          if (find(attr, "type") != string("u8"))
            throw runtime_error("unknown type");
        }
        else if (name == "range")
        {
          scan_range.minx = atoi(find(attr, "minx"));
          scan_range.miny = atoi(find(attr, "miny"));
          scan_range.maxx = atoi(find(attr, "maxx"));
          scan_range.maxy = atoi(find(attr, "maxy"));
        }
        else if (name == "size")
        {
          scan_size[0] = atof(find(attr, "x"));
          scan_size[1] = atof(find(attr, "y"));
          scan_size[2] = atof(find(attr, "z"));
        }
        else if (name == "tomogram")
        {
          tomogram_width = atoi(find(attr, "width"));
          tomogram_height = atoi(find(attr, "height"));
          tomogram_depth = atoi(find(attr, "depth"));
          if (find(attr, "type") != string("u8"))
            throw runtime_error("unknown type");
        }
        else if (name == "contour")
        {
          contour_width = atoi(find(attr, "width"));
          contour_height = atoi(find(attr, "height"));
          if (find(attr, "type") != string("f32"))
            throw runtime_error("unknown type");
        }
        else if (name == "data")
        {
          pos = atoi(find(attr, "start"));
          length = atoi(find(attr, "size"));
          if (find(attr, "storage") != string("raw"))
            throw runtime_error("unknown storage");
        }
        else if (name != "info" && name != "key" && name != "value" && name != "id" && name != "name")
          throw runtime_error("unknown tag");
      }
      catch (exception &e)
      {
        throw runtime_error("tag \"" + name + "\": " + e.what());
      }
    }

    void end(const string &name)
    {
      try
      {
        if (name == "id")
          scan_id = cur;
        else if (name == "key")
          key = cur;
        else if (name == "value")
          value = cur;
        else if (name == "info")
          cur_info->insert(make_pair(key, value));
        else if (name == "data")
          path = cur;
        else if (name == "name")
          cname = cur;
        else if (name == "fundus")
        {
          fundus_pos = make_pair(path, pos);
          if (length != fundus_channels * fundus_width * fundus_height)
            throw runtime_error("fundus data size mismatch");
        }
        else if (name == "tomogram")
        {
          tomogram_pos = make_pair(path, pos);
          if (length != tomogram_width * tomogram_height * tomogram_depth)
            throw runtime_error("tomogram data size mismatch");
        }
        else if (name == "contour")
        {
          contour_pos[cname] = make_tuple(path, contour_width, contour_height, pos);
          if (length != 4 * contour_width * contour_height)
            throw runtime_error("contour data size mismatch");
        }
        else if (name == "scan")
        {
          cur_info = &subject.info;

          oct_scan &scan = subject.scans[scan_id];
          scan.info = info;
          info.clear();

          scan.range = scan_range;
          copy_n(scan_size, 3, scan.size);
          {
            scan.fundus = image<uint8_t>(fundus_channels, fundus_width, fundus_height);
            file f((dirname + fundus_pos.first).c_str(), "rb");
            f.set(fundus_pos.second);
            f.read(scan.fundus.data(), fundus_channels * fundus_width * fundus_height);
          }
          {
            scan.tomogram = volume<uint8_t>(tomogram_width, tomogram_height, tomogram_depth);
            file f((dirname + tomogram_pos.first).c_str(), "rb");
            f.set(tomogram_pos.second);
            f.read(scan.tomogram.data(), tomogram_width * tomogram_height * tomogram_depth);
          }
          for (const auto &c: contour_pos)
          {
            scan.contours[c.first] = image<float>(1, get<1>(c.second), get<2>(c.second));
            file f((dirname + get<0>(c.second)).c_str(), "rb");
            f.set(get<3>(c.second));
            f.read(scan.contours[c.first].data(), get<1>(c.second) * get<2>(c.second));
          }

          contour_pos.clear();
        }
        else if (name != "uoctml" && name != "range" && name != "size" && name != "data")
          throw runtime_error("unknown tag");
      }
      catch (exception &e)
      {
        throw runtime_error("tag \"" + name + "\": " + e.what());
      }
    }

    void data(const char *data, size_t len)
    {
      cur.append(data, len);
    }

  };

  void load(const char *path, oct_subject &subject)
  {
    size_t lastsep = string(path).rfind('/')+1;
    #ifdef _WIN32
    lastsep = max(lastsep, string(path).rfind('\\')+1);
    #endif
    string dirname(path, 0, lastsep);

    parse p(subject, dirname);
    {
      using placeholders::_1;
      using placeholders::_2;
      file f(path, "rb");
      string cur;
      xml x(bind(&parse::start, ref(p), _1, _2), bind(&parse::end, ref(p), _1), bind(&parse::data, ref(p), _1, _2));
      char buf[1024];
      while (f)
      {
        size_t r = f.read(buf, sizeof(buf));
        x(buf, r, r == 0);
      }
    }
  }

  oct_reader r("UOCTML", {".uoctml"}, load);

}
