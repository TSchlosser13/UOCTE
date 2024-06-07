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

#include <GL/glew.h>
#include <cmath>
#include <memory>
#include <sstream>

#include "core/oct_data.hpp"
#include "observer.hpp"
#include "gl_content.hpp"

using namespace std;

struct render_fundus
  : public gl_content
{

  render_fundus(function<void ()> &&update, const oct_scan &scan, observable<size_t> &slice)
    : gl_content(move(update)), m_width(scan.fundus.width()), m_height(scan.fundus.height()), m_depth(scan.tomogram.depth()), m_bbox(scan.range),
      m_fundus_width_in_mm(scan.size[0] * m_width / (m_bbox.maxx - m_bbox.minx)),
      m_slice(slice), m_slice_observer(slice, bind(&render_fundus::slice_changed, this))
  {
    glewInit();

    glGenTextures(1, &m_tx);
    glBindTexture(GL_TEXTURE_2D, m_tx);
    if (scan.fundus.channels() == 3)
      glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, scan.fundus.data());
    else
      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_width, m_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, scan.fundus.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_FRAMEBUFFER_SRGB);
  }

 ~render_fundus()
  {
    glDeleteTextures(1, &m_tx);
  }

  void paint(const std::function<void (int, int, const char *)> &draw_text) override
  {
    glClear(GL_COLOR_BUFFER_BIT);

    glLoadIdentity();

    // fit image inside
    double h = min(m_view_width / (m_width / double(m_height) * m_view_height), 1.0);
    double w = h * m_width / m_height;

    glBindTexture(GL_TEXTURE_2D, m_tx);
    glEnable(GL_TEXTURE_2D);
    glColor4d(1.0, 1.0, 1.0, 1.0);
    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 0.0);
    glVertex2d(-w,  h);
    glTexCoord2d(1.0, 0.0);
    glVertex2d( w,  h);
    glTexCoord2d(1.0, 1.0);
    glVertex2d( w, -h);
    glTexCoord2d(0.0, 1.0);
    glVertex2d(-w, -h);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glColor4d(0.0, 1.0, 0.0, 0.25);
    glBegin(GL_QUADS);
    glVertex2d(((2.0 * m_bbox.minx + 1.0) / m_width - 1.0) * w, ((2.0 * m_bbox.miny + 1.0) / m_height - 1.0) * -h);
    glVertex2d(((2.0 * m_bbox.maxx + 1.0) / m_width - 1.0) * w, ((2.0 * m_bbox.miny + 1.0) / m_height - 1.0) * -h);
    glVertex2d(((2.0 * m_bbox.maxx + 1.0) / m_width - 1.0) * w, ((2.0 * m_bbox.maxy + 1.0) / m_height - 1.0) * -h);
    glVertex2d(((2.0 * m_bbox.minx + 1.0) / m_width - 1.0) * w, ((2.0 * m_bbox.maxy + 1.0) / m_height - 1.0) * -h);
    glEnd();
    glDisable(GL_BLEND);

    glColor3d(0.0, 1.0, 0.0);
    glBegin(GL_LINES);
    glVertex2d(((2.0 * m_bbox.minx + 1.0) / m_width - 1.0) * w, ((2.0 * (m_bbox.miny + (m_bbox.maxy - m_bbox.miny) * (m_slice / max(m_depth - 1.0, 1.0))) + 1.0) / m_height - 1.0) * -h);
    glVertex2d(((2.0 * m_bbox.maxx + 1.0) / m_width - 1.0) * w, ((2.0 * (m_bbox.miny + (m_bbox.maxy - m_bbox.miny) * (m_slice / max(m_depth - 1.0, 1.0))) + 1.0) / m_height - 1.0) * -h);
    glEnd();

    double l = w / m_fundus_width_in_mm;
    glBegin(GL_LINES);
    glVertex2d(10.0 / m_view_height - 1.0 * m_view_width / m_view_height, 40.0 / m_view_height - 1.0);
    glVertex2d(15.0 / m_view_height + 2.0 * l - 1.0 * m_view_width / m_view_height, 40.0 / m_view_height - 1.0);
    glVertex2d(15.0 / m_view_height - 1.0 * m_view_width / m_view_height, 35.0 / m_view_height - 1.0);
    glVertex2d(15.0 / m_view_height - 1.0 * m_view_width / m_view_height, 40.0 / m_view_height + 2.0 * l - 1.0);
    glVertex2d(15.0 / m_view_height + 2.0 * l - 1.0 * m_view_width / m_view_height, 35.0 / m_view_height - 1.0);
    glVertex2d(15.0 / m_view_height + 2.0 * l - 1.0 * m_view_width / m_view_height, 45.0 / m_view_height - 1.0);
    glVertex2d(10.0 / m_view_height - 1.0 * m_view_width / m_view_height, 40.0 / m_view_height + 2.0 * l - 1.0);
    glVertex2d(20.0 / m_view_height - 1.0 * m_view_width / m_view_height, 40.0 / m_view_height + 2.0 * l - 1.0);
    glEnd();

    ostringstream os;
    os << "Slice " << 1 + m_slice << "/" << m_depth;
    draw_text(5, 15, os.str().c_str());
    draw_text(5, m_view_height - 5, "1mm");
  }

  void resize(size_t width, size_t height) override
  {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0 * width / height, 1.0 * width / height, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    m_view_width = width;
    m_view_height = height;
  }

  void mouse_press(size_t, size_t, size_t) override
  {
  }

  void mouse_release(size_t, size_t, size_t) override
  {
  }

  void mouse_move(size_t, size_t, size_t) override
  {
  }

  void mouse_wheel(size_t, size_t, int delta) override
  {
    delta /= 120;
    if (delta != 0 && int(m_slice) - delta >= 0 && m_slice - delta < m_depth)
      m_slice = m_slice - delta;
  }

  void slice_changed()
  {
    update();
  }

  GLuint m_tx;
  size_t m_width, m_height, m_depth, m_view_width, m_view_height;
  bounding_box m_bbox;
  double m_fundus_width_in_mm;
  observable<size_t> &m_slice;
  observer m_slice_observer;

};

unique_ptr<gl_content> make_render_fundus(function<void ()> &&update, const oct_scan &scan, observable<size_t> &slice)
{
  return unique_ptr<gl_content>(new render_fundus(move(update), scan, slice));
}
