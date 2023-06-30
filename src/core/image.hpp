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

#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <cassert>
#include <cstdint>
#include <memory>

/// image data
/**
 * The image's origin is lower left.
 */
template <class T>
class image
{

  std::size_t m_dims[3]; ///< image dimensions
  std::unique_ptr<T []> m_data; ///< image data, row-wise storage

public:

  /// create empty image
  image()
    : m_dims{0, 0}, m_data{}
  {
  }

  /// create image with given dimensions
  image(std::size_t channels, std::size_t width, std::size_t height)
    : m_dims{channels, width, height}, m_data{new T[channels * width * height]}
  {
  }

  /// give image number of channels
  std::size_t channels() const
  {
    return m_dims[0];
  }

  /// give image width
  std::size_t width() const
  {
    return m_dims[1];
  }

  /// give image height
  std::size_t height() const
  {
    return m_dims[2];
  }

  /// raw data access
  T *data()
  {
    return m_data.get();
  }

  /// raw data access
  const T *data() const
  {
    return m_data.get();
  }

  /// access pixel at position
  T &operator()(std::size_t x, std::size_t y)
  {
    assert(1 == channels());
    assert(x < width());
    assert(y < height());
    return m_data[y*width()+x];
  }

  /// access pixel at position
  const T &operator()(std::size_t x, std::size_t y) const
  {
    assert(1 == channels());
    assert(x < width());
    assert(y < height());
    return m_data[y*width()+x];
  }

  /// access pixel at position
  T &operator()(std::size_t c, std::size_t x, std::size_t y)
  {
    assert(c < channels());
    assert(x < width());
    assert(y < height());
    return m_data[(y*width()+x)*channels()+c];
  }

  /// access pixel at position
  const T &operator()(std::size_t c, std::size_t x, std::size_t y) const
  {
    assert(c < channels());
    assert(x < width());
    assert(y < height());
    return m_data[(y*width()+x)*channels()+c];
  }

};

#endif // inclusion guard
