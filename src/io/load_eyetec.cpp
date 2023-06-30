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
#include <sstream>
#include <stdexcept>

#include <archive.h>
#include <archive_entry.h>

#include "../core/oct_data.hpp"
#include "xml.hpp"

using namespace std;

namespace
{

  struct eyetec_info
  {
    oct_subject &subject;
    map<string, string> cur_paths;
    string cur;
    size_t series_id, instance_id;
    string series_date, content_date, path, type, laterality;
    map<string, pair<string, oct_scan *>> paths;

    eyetec_info(oct_subject &subject) : subject(subject) { }

    void start(string &&name, const char **)
    {
      cur.clear();
      if (name == "Contents")
        cur_paths.clear();
    }

    void end(string &&name)
    {
      if (name == "PatientNameGroup1")
        subject.info["name"] = cur;
      else if (name == "PatientBirthDate")
        subject.info["birth date"] = cur;
      else if (name == "PatientSex")
        subject.info["sex"] = cur;
      else if (name == "EthnicGroup")
        subject.info["ethnicity"] = cur;
      else if (name == "ManufacturerModelName" || name == "DeviceSerialNumber" || name == "SoftwareVersion" || name == "SoftwareName")
        subject.info[name] = cur;
      else if (name == "SeriesNumber")
        series_id = atoi(cur.c_str());
      else if (name == "ContentLaterality")
        laterality = cur;
      else if (name == "InstanceNumber")
        instance_id = atoi(cur.c_str());
      else if (name == "ContentDateTime")
        content_date = cur;
      else if (name == "Name")
        path = cur;
      else if (name == "Type")
        type = cur;
      else if (name == "FileDetails")
        cur_paths[path] = type;
      else if (name == "Contents")
      {
        ostringstream o;
        o << series_id << "." << instance_id;
        oct_scan &s = subject.scans[o.str()];
        s.info["scan date"] = content_date;
        s.info["laterality"] = laterality;
        for (auto &e: cur_paths)
          paths[e.first] = make_pair(e.second, &s);
      }
    }

    void data(const char *data, size_t len)
    {
      cur.append(data, len);
    }
  };

  char buf[10240];

  ssize_t my_read(struct archive *, void *parent, const void **buffer)
  {
    *buffer = buf;
    return archive_read_data(static_cast<struct archive *>(parent), buf, 10240);
  }

  int my_open(struct archive *, void *)
  {
    return ARCHIVE_OK;
  }

  int my_close(struct archive *, void *)
  {
    return ARCHIVE_OK;
  }

  void archive_read_skip_n(struct archive *a, size_t n)
  {
    char x;
    while (n != 0)
      archive_read_data(a, &x, 1), --n;
  }

  void read_contours(oct_scan &scan, struct archive *parent)
  {
    int r;
    struct archive *a = archive_read_new();
    archive_read_support_filter_gzip(a);
    archive_read_support_format_raw(a);
    r = archive_read_open(a, parent, my_open, my_read, my_close);
    if (r != ARCHIVE_OK)
      throw runtime_error(archive_error_string(a));

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
      uint32_t width, height;
      for (size_t i = 0; i != 10; ++i)
      {
        archive_read_skip_n(a, 4);
        archive_read_data(a, &width, 4);
        archive_read_data(a, &height, 4);

        ostringstream os;
        os << "CONTOUR" << i;
        image<float> &img = scan.contours[os.str()];
        img = image<float>(1, width, height);

        archive_read_skip_n(a, 8);
        unique_ptr<uint16_t []> p(new uint16_t[width * height]);
        archive_read_data(a, &p[0], width * height * sizeof(uint16_t));
        transform(&p[0], &p[width*height], img.data(), [&](uint16_t v){return v / 1.7f;});

        archive_read_skip_n(a, width * height + 128 + 4);
      }
    }

    archive_read_close(a);
    r = archive_read_free(a);
    if (r != ARCHIVE_OK)
      throw runtime_error(archive_error_string(a));
  }

  void read_fundus(oct_scan &scan, struct archive *parent)
  {
    int r;
    struct archive *a = archive_read_new();
    archive_read_support_filter_gzip(a);
    archive_read_support_format_raw(a);
    r = archive_read_open(a, parent, my_open, my_read, my_close);
    if (r != ARCHIVE_OK)
      throw runtime_error(archive_error_string(a));

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
      uint32_t width, height;
      archive_read_skip_n(a, 4);
      archive_read_data(a, &width, 4);
      archive_read_data(a, &height, 4);

      // skip eye image
      archive_read_skip_n(a, 16);
      archive_read_skip_n(a, width * height);

      archive_read_skip_n(a, 128);
      archive_read_data(a, &width, 4);
      archive_read_data(a, &height, 4);

      scan.fundus = image<uint8_t>(1, width, height);

      archive_read_skip_n(a, 16);
      archive_read_data(a, scan.fundus.data(), width * height);

      scan.range.minx = 0;
      scan.range.maxx = width;
      scan.range.miny = 0;
      scan.range.maxy = height;

      archive_read_skip_n(a, 128);
      archive_read_data(a, &width, 4);
      archive_read_data(a, &height, 4);

      // skip maximum intensity projection
      archive_read_skip_n(a, 16);
      archive_read_skip_n(a, width * height);
    }

    archive_read_close(a);
    r = archive_read_free(a);
    if (r != ARCHIVE_OK)
      throw runtime_error(archive_error_string(a));
  }

  void read_volume(oct_scan &scan, struct archive *parent)
  {
    int r;
    struct archive *a = archive_read_new();
    archive_read_support_filter_gzip(a);
    archive_read_support_format_raw(a);
    r = archive_read_open(a, parent, my_open, my_read, my_close);
    if (r != ARCHIVE_OK)
      throw runtime_error(archive_error_string(a));

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
      uint32_t width, height, depth;
      archive_read_skip_n(a, 4);
      archive_read_data(a, &width, 4);
      archive_read_data(a, &height, 4);
      archive_read_data(a, &depth, 4);

      scan.tomogram = volume<uint8_t>(width, height, depth);

      scan.size[0] =   12.0f;
      scan.size[1] = 0.0017f * height;
      scan.size[2] =    9.0f;

      archive_read_skip_n(a, 24);
      for (size_t z = 0; z != depth; ++z)
      {
        archive_read_data(a, &scan.tomogram(0, 0, z), width * height);
        archive_read_skip_n(a, 128 + 24);
      }
    }

    archive_read_close(a);
    r = archive_read_free(a);
    if (r != ARCHIVE_OK)
      throw runtime_error(archive_error_string(a));
  }

  const char PATH_PREFIX[] = "PatientsFiles/";
  const char DB_PATH[] = "DBData.xml";

  void load(const char *path, oct_subject &subject)
  {
    eyetec_info info(subject);

    int r;
    struct archive *a = archive_read_new();
    archive_read_support_filter_none(a);
    archive_read_support_format_zip(a);
    r = archive_read_open_filename(a, path, 10240);
    if (r != ARCHIVE_OK)
      throw runtime_error(archive_error_string(a));

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
      if (strncmp(archive_entry_pathname(entry), PATH_PREFIX, strlen(PATH_PREFIX)) != 0)
        continue;

      if (strcmp(archive_entry_pathname(entry) + strlen(PATH_PREFIX), DB_PATH) != 0)
        continue;

      using placeholders::_1;
      using placeholders::_2;
      char buf[1024];
      xml x(bind(&eyetec_info::start, &info, _1, _2), bind(&eyetec_info::end, &info, _1), bind(&eyetec_info::data, &info, _1, _2));
      while (true)
      {
        auto size = archive_read_data(a, buf, sizeof(buf));
        if (size < 0)
          throw runtime_error(archive_error_string(a));
        x(buf, size, size == 0);
        if (size == 0)
          break;
      }
    }

    archive_read_close(a);
    r = archive_read_free(a);
    if (r != ARCHIVE_OK)
      throw runtime_error(archive_error_string(a));

    // reopen file
    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_zip(a);
    r = archive_read_open_filename(a, path, 10240);
    if (r != ARCHIVE_OK)
      throw runtime_error(archive_error_string(a));

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
      if (strncmp(archive_entry_pathname(entry), PATH_PREFIX, strlen(PATH_PREFIX)) != 0)
        continue;

      auto i = info.paths.find(archive_entry_pathname(entry) + strlen(PATH_PREFIX));
      if (i == info.paths.end())
        continue;

      if (i->second.first == "AnalysedData")
        read_contours(*i->second.second, a);
      else if (i->second.first == "Images")
        read_fundus(*i->second.second, a);
      else if (i->second.first == "Tomograms")
        read_volume(*i->second.second, a);
    }

    r = archive_read_free(a);
    if (r != ARCHIVE_OK)
      throw runtime_error(archive_error_string(a));
  }

  oct_reader r("Eyetec", {".exd"}, load);
}
