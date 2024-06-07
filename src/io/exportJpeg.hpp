#ifndef EXPORT_JPEG_HPP
#define EXPORT_JPEG_HPP

#include <algorithm>
#include <memory>
#include <sstream>

#include <QColor>

#include "../core/oct_data.hpp"
#include "jpge.h"

bool exportSlicesAsJpeg(const oct_scan* scan, std::__cxx11::string filename_base, std::vector<int> contourList, QColor contourColor);

#endif //EXPORT_JPEG_HPP
