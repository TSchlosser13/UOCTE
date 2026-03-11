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

#ifndef GL_CONTENT_HPP
#define GL_CONTENT_HPP

#include <cstddef>
#include <functional>

class gl_content
{

  std::function<void ()> m_update;

protected:

  void update() { m_update(); }

public:

  gl_content(std::function<void ()> &&update) : m_update(update) { }
  gl_content(const gl_content &) = delete;
  gl_content &operator=(const gl_content &) = delete;
  virtual ~gl_content() = default;

  virtual void paint(const std::function<void (int, int, const char *)> &draw_text) = 0;
  virtual void resize(std::size_t width, std::size_t height) = 0;
  virtual void mouse_press(std::size_t x, std::size_t y, std::size_t button) = 0;
  virtual void mouse_release(std::size_t x, std::size_t y, std::size_t button) = 0;
  virtual void mouse_move(std::size_t x, std::size_t y, std::size_t buttons) = 0;
  virtual void mouse_wheel(std::size_t x, std::size_t y, int delta) = 0;

};

#endif // inclusion guard
