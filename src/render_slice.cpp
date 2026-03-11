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
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <sstream>

#include "core/oct_data.hpp"
#include "observer.hpp"
#include "gl_content.hpp"

using namespace std;

struct render_slice
  : public gl_content
{

  render_slice(function<void ()> &&update, const vector<pair<const oct_scan *, observable<size_t> *>> &scans, observable<size_t> &demux, observable<size_t> &key)
    : gl_content(move(update)), m_scans(scans.size()), m_demux(demux), m_demux_observer(demux, bind(&render_slice::slice_changed, this)), m_key(key), m_key_observer(key, bind(&render_slice::key_changed, this)), m_zoom(0.0), m_cx(0.0), m_cy(0.0)
  {
    glewInit();

    for (size_t i = 0; i != scans.size(); ++i)
    {
      m_scans[i].width = scans[i].first->tomogram.width();
      m_scans[i].height = scans[i].first->tomogram.height();
      m_scans[i].depth = scans[i].first->tomogram.depth();
      m_scans[i].aspect = scans[i].first->size[0] / scans[i].first->size[1];
      m_scans[i].width_in_mm = scans[i].first->size[0];
      m_scans[i].contours = &scans[i].first->contours;
      if (m_visible.size() < scans[i].first->contours.size())
        m_visible.resize(scans[i].first->contours.size(), true);
      m_scans[i].slice = scans[i].second;
      m_scans[i].slice_observer.reset(new observer(*scans[i].second, bind(&render_slice::slice_changed, this)));

      auto e = scans[i].first->info.find("laterality");
      if (e != scans[i].first->info.end())
        m_scans[i].laterality = e->second;

      // shorthand
      size_t size = m_scans[i].width * m_scans[i].height * m_scans[i].depth;

      // find min and max intensity value, discard upper and lower 1% as outliers
      unique_ptr<uint8_t []> d(new uint8_t[size]);
      copy_n(scans[i].first->tomogram.data(), size, d.get());
      uint8_t *p01 = d.get() + size / 100;
      uint8_t *p99 = d.get() + size - (size + 99) / 100;
      nth_element(d.get(), p01, d.get() + size);
      nth_element(d.get(), p99, d.get() + size);
      uint8_t minv = *p01, maxv = *p99;

      // turn image negative and maximize contrast
      for (size_t k = 0; k != size; ++k)
        d[k] = uint8_t(numeric_limits<uint8_t>::max() * min(1.0, max(0.0, (1.0 - double(scans[i].first->tomogram.data()[k] - minv) / (maxv - minv)))));

      // load volume data
      glGenTextures(1, &m_scans[i].tx);
      glBindTexture(GL_TEXTURE_3D, m_scans[i].tx);
      glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE, m_scans[i].width, m_scans[i].height, m_scans[i].depth, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, d.get());
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    // initialize GL state
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_FRAMEBUFFER_SRGB);
  }

 ~render_slice()
  {
    for (size_t i = 0; i != m_scans.size(); ++i)
      glDeleteTextures(1, &m_scans[i].tx);
  }

  void paint(const std::function<void (int, int, const char *)> &draw_text) override
  {
    glClear(GL_COLOR_BUFFER_BIT);

    // paning & zooming
    double s = exp(log(2.0) * m_zoom / 8.0);
    glLoadIdentity();
    glScaled(s, s, s);
    glTranslated(m_cx, m_cy, 0.0);

    const scan &p = m_scans[m_demux];

    // correct aspect ratio and ensure image is inside [-1.0,1.0]x[-1.0,1.0]
    double h = min(m_view_width / (p.aspect * m_view_height), 1.0);
    double w = p.aspect * h;

    // draw image
    glBindTexture(GL_TEXTURE_3D, p.tx);
    glEnable(GL_TEXTURE_3D);
    glColor4d(1.0, 1.0, 1.0, 1.0);
    glBegin(GL_QUADS);
    glTexCoord3d(0.0, 0.0, (*p.slice+0.5)/p.depth);
    glVertex2d(-w,  h);
    glTexCoord3d(1.0, 0.0, (*p.slice+0.5)/p.depth);
    glVertex2d( w,  h);
    glTexCoord3d(1.0, 1.0, (*p.slice+0.5)/p.depth);
    glVertex2d( w, -h);
    glTexCoord3d(0.0, 1.0, (*p.slice+0.5)/p.depth);
    glVertex2d(-w, -h);
    glEnd();
    glDisable(GL_TEXTURE_3D);

    // draw contours
    glLineWidth(3.0 * s);
    glEnable(GL_BLEND);
    glColor4d(0.0, 1.0, 0.0, 0.5);
    size_t n = 0;
    for (const auto &c: *p.contours)
    {
      if (!m_visible[n++])
        continue;

      glBegin(GL_LINE_STRIP);
      for (size_t i = 0; i != c.second.width(); ++i)
        glVertex2d(((2.0 * i + 1.0) / c.second.width() - 1.0) * w, ((2.0 * c.second(i, *p.slice * c.second.height() / p.depth) + 1.0) / p.height - 1.0) * -h);
      glEnd();
    }
    glDisable(GL_BLEND);

    glLoadIdentity();
    glLineWidth(1.0);

    double x = 1.0;
    double l = s * w / p.width_in_mm;
    while (l > 0.5)
    {
      l *= 0.1;
      x *= 0.1;
    }
    if (l > 0.25)
    {
      l *= 0.5;
      x *= 0.5;
    }

    glColor3d(0.0, 1.0, 0.0);
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
    os << x << "mm";
    draw_text(5, m_view_height - 5, os.str().c_str());
    draw_text(5, 15, p.laterality == "L" ? "N" : p.laterality == "R" ? "T" : "");
    draw_text(m_view_width - 15, 15, p.laterality == "L" ? "T" : p.laterality == "R" ? "N" : "");
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

  void mouse_move(size_t x, size_t y, size_t button) override
  {
    if (button == 1)
    {
      // compute new paning
      double s = exp(log(2.0) * m_zoom / 8.0) * m_view_height;
      m_cx = min(1.0, max(-1.0, m_cx + 2.0 * (int(x) - int(m_ox)) / s));
      m_cy = min(1.0, max(-1.0, m_cy - 2.0 * (int(y) - int(m_oy)) / s));
      update();
    }
    m_ox = x;
    m_oy = y;
  }

  void mouse_wheel(size_t, size_t, int delta) override
  {
    m_zoom = max(0.0, m_zoom + delta / 120.0);
    update();
  }

  void slice_changed()
  {
    update();
  }

  void key_changed()
  {
    size_t i = (m_key != '0' ? m_key - '1' : 9);
    if (i < m_visible.size())
    {
      m_visible[i] = !m_visible[i];
      update();
    }
  }

  struct scan
  {
    GLuint tx;
    size_t width, height, depth;
    double aspect, width_in_mm;
    const map<string, image<float>> *contours;
    string laterality;
    observable<size_t> *slice;
    unique_ptr<observer> slice_observer;
  };

  vector<scan> m_scans;
  vector<bool> m_visible;
  observable<size_t> &m_demux;
  observer m_demux_observer;
  observable<size_t> &m_key;
  observer m_key_observer;
  size_t m_view_width, m_view_height, m_ox, m_oy;
  double m_zoom, m_cx, m_cy;

};

unique_ptr<gl_content> make_render_slice(function<void ()> &&update, const vector<pair<const oct_scan *, observable<size_t> *>> &scans, observable<size_t> &demux, observable<size_t> &key)
{
  return unique_ptr<gl_content>(new render_slice(move(update), scans, demux, key));
}
