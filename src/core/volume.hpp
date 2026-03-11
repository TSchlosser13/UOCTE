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

#ifndef VOLUME_HPP
#define VOLUME_HPP

#include <cassert>
#include <memory>

/// volume data
template <class T>
class volume
{

  std::size_t m_dims[3]; ///< volume dimensions
  std::unique_ptr<T []> m_data; ///< volume data, row-wise storage

public:

  /// create empty volume
  volume()
    : m_dims{0, 0, 0}, m_data{}
  {
  }

  /// create volume with given dimensions
  volume(std::size_t width, std::size_t height, std::size_t depth)
    : m_dims{width, height, depth}, m_data{new T[width * height * depth]}
  {
  }

  /// give volume width
  std::size_t width() const
  {
    return m_dims[0];
  }

  /// give volume height
  std::size_t height() const
  {
    return m_dims[1];
  }

  /// give volume depth
  std::size_t depth() const
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

  /// access voxel at position
  T &operator()(std::size_t x, std::size_t y, std::size_t z)
  {
    assert(x < width());
    assert(y < height());
    assert(z < depth());
    return m_data[(z*height()+y)*width()+x];
  }

  /// access voxel at position
  const T &operator()(std::size_t x, std::size_t y, std::size_t z) const
  {
    assert(x < width());
    assert(y < height());
    assert(z < depth());
    return m_data[(z*height()+y)*width()+x];
  }

};

#endif // inclusion guard
