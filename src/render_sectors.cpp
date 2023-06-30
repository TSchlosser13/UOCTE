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

#include <iostream>

#include "core/oct_data.hpp"
#include "observer.hpp"
#include "gl_content.hpp"

using namespace std;

struct render_sectors
  : public gl_content
{

  render_sectors(function<void ()> &&update, const oct_scan &scan)
    : gl_content(move(update)), m_scan(scan), m_aspect(scan.size[0] / scan.size[2]), m_width_in_mm(scan.size[0]),
      c(0), no(0), nr(0), nl(0), nu(0), fo(0), fr(0), fl(0), fu(0)
  {
    glewInit();
    
    auto i = scan.info.find("laterality");
    if (i != scan.info.end())
      m_laterality = i->second;

    // setup GL
    glGenTextures(1, &m_tx);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glBindTexture(GL_TEXTURE_2D, m_tx);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 1, 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    m_base = scan.contours.begin();
    m_other = scan.contours.end();

    if (m_base == m_other)
      return;

    --m_other;

    //ignore NaN as last contour (found in E2E files)
    //use center of image as example for validation
    float test = m_other->second(m_other->second.width()/2,m_other->second.height()/2);
    if (test != test)
        --m_other;

    if (m_base == m_other)
        return;
  }

 ~render_sectors()
  {
    glDeleteTextures(1, &m_tx);
  }

  void init(const image<float> &o1, const image<float> &o2)
  {
    size_t X = o1.width();
    size_t Y = o1.height();

    if (X != o2.width() || Y != o2.height())
      return;

    c = 0;
    no = 0;
    nr = 0;
    nl = 0;
    nu  = 0;
    fo = 0;
    fr = 0;
    fl = 0;
    fu = 0;
    t = 0;

    unique_ptr<uint8_t []> depth(new uint8_t [X*Y]);

    size_t nc = 0, nno = 0, nnr = 0, nnl = 0, nnu = 0, nfo = 0, nfr = 0, nfl = 0, nfu = 0, n = 0;
    for (size_t y = 0; y != Y; ++y)
    {
      for (size_t x = 0; x != X; ++x)
      {
        double d = abs(m_scan.size[1] / m_scan.tomogram.height() * (o2(x, y) - o1(x, y))); // thickness in mm
        double mx = m_scan.size[0] * (2 * x + 1.0 - X) / 2 / X; // x-distance in mm
        double my = m_scan.size[2] * (2 * y + 1.0 - Y) / 2 / Y; // y-distance in mm
        double r2 = mx*mx + my*my; // square of distance

        if (d != d) // skip NaNs
        {
          depth[y*X+x] = 255;
          continue;
        }

        // total volume
        if (r2 <= 9.0)
        {
          t += d;
          ++n;
        }

        depth[y*X+x] = min(255.0 / 0.5 * d, 255.0);

        // sector thickness
        if (r2 <= 0.25)
        {
          c += d;
          ++nc;
        }
        else if (r2 <= 2.25)
        {
          if (mx <= my)
          {
            if (mx + my >= 0)
            {
              nu += d;
              ++nnu;
            }
            else
            {
              nl += d;
              ++nnl;
            }
          }
          else
          {
            if (mx + my >= 0)
            {
              nr += d;
              ++nnr;
            }
            else
            {
              no += d;
              ++nno;
            }
          }
        }
        else if (r2 <= 9.0)
        {
          if (mx <= my)
          {
            if (mx + my >= 0)
            {
              fu += d;
              ++nfu;
            }
            else
            {
              fl += d;
              ++nfl;
            }
          }
          else
          {
            if (mx + my >= 0)
            {
              fr += d;
              ++nfr;
            }
            else
            {
              fo += d;
              ++nfo;
            }
          }
        }
      }
    }

    // average
    c /= nc;
    no /= nno;
    nr /= nnr;
    nl /= nnl;
    nu /= nnu;
    fo /= nfo;
    fr /= nfr;
    fl /= nfl;
    fu /= nfu;
    v = t * m_scan.size[0] * m_scan.size[2] / X / Y;
    t /= n;

    glBindTexture(GL_TEXTURE_2D, m_tx);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, X, Y, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, depth.get());
  }

  void paint(const std::function<void (int, int, const char *)> &draw_text) override
  {
    glClear(GL_COLOR_BUFFER_BIT);

    if (m_other == m_scan.contours.end())
      return;

    init(m_base->second, m_other->second);

    glLoadIdentity();

    // correct aspect ratio and ensure image is inside [-1.0,1.0]x[-1.0,1.0]
    double h = min(m_view_width / (m_aspect * m_view_height), 1.0);
    double w = m_aspect * h;

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

    // draw circles and their sectors
    double PI = 3.14159265358979323846264338327950288419716939937510;
    glColor3d(0.0, 0.0, 0.0);
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i != 32; ++i)
      glVertex2d(6.0 * w / m_width_in_mm * cos(i * PI / 16), 6.0 * w / m_width_in_mm * sin(i * PI / 16));
    glVertex2d(6.0 * w / m_width_in_mm, 0.0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i != 32; ++i)
      glVertex2d(3.0 * w / m_width_in_mm * cos(i * PI / 16), 3.0 * w / m_width_in_mm * sin(i * PI / 16));
    glVertex2d(3.0 * w / m_width_in_mm, 0.0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i != 32; ++i)
      glVertex2d(1.0 * w / m_width_in_mm * cos(i * PI / 16), 1.0 * w / m_width_in_mm * sin(i * PI / 16));
    glVertex2d(1.0 * w / m_width_in_mm, 0.0);
    glEnd();
    glBegin(GL_LINES);
    glVertex2d(-6.0 * sqrt(0.5) * w / m_width_in_mm, -6.0 * sqrt(0.5) * w / m_width_in_mm);
    glVertex2d(-1.0 * sqrt(0.5) * w / m_width_in_mm, -1.0 * sqrt(0.5) * w / m_width_in_mm);
    glVertex2d( 6.0 * sqrt(0.5) * w / m_width_in_mm, -6.0 * sqrt(0.5) * w / m_width_in_mm);
    glVertex2d( 1.0 * sqrt(0.5) * w / m_width_in_mm, -1.0 * sqrt(0.5) * w / m_width_in_mm);
    glVertex2d(-6.0 * sqrt(0.5) * w / m_width_in_mm,  6.0 * sqrt(0.5) * w / m_width_in_mm);
    glVertex2d(-1.0 * sqrt(0.5) * w / m_width_in_mm,  1.0 * sqrt(0.5) * w / m_width_in_mm);
    glVertex2d( 6.0 * sqrt(0.5) * w / m_width_in_mm,  6.0 * sqrt(0.5) * w / m_width_in_mm);
    glVertex2d( 1.0 * sqrt(0.5) * w / m_width_in_mm,  1.0 * sqrt(0.5) * w / m_width_in_mm);
    glEnd();

    {
      ostringstream os;
      os << int(1000.0 * c + 0.5);
      draw_text(m_view_width / 2 - 10, m_view_height / 2 + 5, os.str().c_str());
    }
    {
      ostringstream os;
      os << int(1000.0 * no + 0.5);
      draw_text(m_view_width / 2 - 10, (-2.0 * w / m_width_in_mm + 1.0) * 0.5 * m_view_height + 5, os.str().c_str());
    }
    {
      ostringstream os;
      os << int(1000.0 * nl + 0.5);
      draw_text(-2.0 * w / m_width_in_mm * 0.5 * m_view_height + m_view_width / 2 - 10, m_view_height / 2 + 5, os.str().c_str());
    }
    {
      ostringstream os;
      os << int(1000.0 * nr + 0.5);
      draw_text( 2.0 * w / m_width_in_mm * 0.5 * m_view_height + m_view_width / 2 - 10, m_view_height / 2 + 5, os.str().c_str());
    }
    {
      ostringstream os;
      os << int(1000.0 * nu + 0.5);
      draw_text(m_view_width / 2 - 10, ( 2.0 * w / m_width_in_mm + 1.0) * 0.5 * m_view_height + 5, os.str().c_str());
    }
    {
      ostringstream os;
      os << int(1000.0 * fo + 0.5);
      draw_text(m_view_width / 2 - 10, (-4.5 * w / m_width_in_mm + 1.0) * 0.5 * m_view_height + 5, os.str().c_str());
    }
    {
      ostringstream os;
      os << int(1000.0 * fl + 0.5);
      draw_text(-4.5 * w / m_width_in_mm * 0.5 * m_view_height + m_view_width / 2 - 10, m_view_height / 2 + 5, os.str().c_str());
    }
    {
      ostringstream os;
      os << int(1000.0 * fr + 0.5);
      draw_text( 4.5 * w / m_width_in_mm * 0.5 * m_view_height + m_view_width / 2 - 10, m_view_height / 2 + 5, os.str().c_str());
    }
    {
      ostringstream os;
      os << int(1000.0 * fu + 0.5);
      draw_text(m_view_width / 2 - 10, ( 4.5 * w / m_width_in_mm + 1.0) * 0.5 * m_view_height + 5, os.str().c_str());
    }

    glColor3d(0.0, 1.0, 0.0);
    draw_text(5, m_view_height / 2 + 5, m_laterality == "L" ? "N" : m_laterality == "R" ? "T" : "");
    draw_text(m_view_width - 15, m_view_height / 2 + 5, m_laterality == "L" ? "T" : m_laterality == "R" ? "N" : "");
    draw_text(5, 15, m_base->first.c_str());
    draw_text(5, 25, m_other->first.c_str());
    {
      ostringstream os;
      os << "avg. thickness: " << 1000.0 * t;
      draw_text(5, m_view_height - 15, os.str().c_str());
    }
    {
      ostringstream os;
      os << "total volume: " << v;
      draw_text(5, m_view_height - 5, os.str().c_str());
    }
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
    if (delta > 0
            && m_other != --m_scan.contours.end()) {
        ++m_other;
        //ignore NaN as last contour (found in E2E files)
        //use center of image as example for validation
        float test = m_other->second(m_other->second.width()/2,m_other->second.height()/2);
        if (test != test)
            --m_other;
    }
    if (delta < 0
            && m_other != m_base
            //prevent m_base and m_other from being equal
            && m_other != ++m_scan.contours.begin())
      --m_other;

    update();
  }

  const oct_scan &m_scan;
  map<string, image<float>>::const_iterator m_base, m_other;
  GLuint m_tx;
  size_t m_width, m_height;
  double m_aspect, m_width_in_mm, c, no, nr, nl, nu, fo, fr, fl, fu, t, v;
  string m_laterality;
  size_t m_view_width, m_view_height;

};

unique_ptr<gl_content> make_render_sectors(function<void ()> &&update, const oct_scan &scan)
{
  return unique_ptr<gl_content>(new render_sectors(move(update), scan));
}
