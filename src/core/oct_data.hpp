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

#ifndef OCT_DATA_HPP
#define OCT_DATA_HPP

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "image.hpp"
#include "volume.hpp"

/// axis-parallel bounding box
struct bounding_box
{
  std::size_t minx; ///< left
  std::size_t maxx; ///< right
  std::size_t miny; ///< lower
  std::size_t maxy; ///< upper
};

/// one OCT C-scan
struct oct_scan
{

  ///< photographic eye image
  image<uint8_t> fundus;

  /// OCT C-scan range in fundus pixel coordinates
  bounding_box range;

  /// scan volume size in millimeter
  float size[3];

  /// reconstructed OCT C-scan
  volume<uint8_t> tomogram;

  /// list of contours
  std::map<std::string, image<float>> contours;

  /// scan info
  /**
   * Should include at least the following key-value pairs:
   * - "scan date" for aquision date (human readable)
   * - "laterality"  for which eye was examined ("L"/"R")
   */
  std::map<std::string, std::string> info;

};

/// collection of OCT scans of a subject
struct oct_subject
{

  /// subject scans
  std::map<std::string, oct_scan> scans;

  /// subject info
  /**
   * Should include at least the following keys-value pairs:
   * "name" patient's name (human readable)
   * "birth date" patient's date of birth (human readable)
   * "sex" patient's sex ("F"/"M")
   */
  std::map<std::string, std::string> info;

  /// load oct file
  oct_subject(const char *path);

};

/// OCT data reader description
struct oct_reader
{
  oct_reader(std::string &&name, std::vector<std::string> &&extensions, std::function<void (const char *, oct_subject &s)> &&read);
  oct_reader(oct_reader &&) = delete;
  oct_reader(const oct_reader &) = delete;
  oct_reader &operator=(oct_reader &&) = delete;
  oct_reader &operator=(const oct_reader &) = delete;
 ~oct_reader();
};

void foreach_oct_reader(const std::function<void (const std::string &, const std::vector<std::string> &)> &fn);

#endif // inclusion guard
