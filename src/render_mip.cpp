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
#include <iostream>

#include <QCoreApplication>
#include <QGLShaderProgram>

#include "core/oct_data.hpp"
#include "observer.hpp"
#include "gl_content.hpp"

using namespace std;

namespace
{

  void make_blackbody_colormap()
  {
    unique_ptr<uint8_t []> d(new uint8_t[256*4]);
    for (size_t i = 0; i != 256; ++i)
    {
      if (i < 55)
      {
        d[4*i+0] = (i * 255) / 55;
        d[4*i+1] = 0;
        d[4*i+2] = 0;
        d[4*i+3] = i;
      }
      else if (i < 55+182)
      {
        d[4*i+0] = 255;
        d[4*i+1] = ((i - 55) * 255) / 182;
        d[4*i+2] = 0;
        d[4*i+3] = i;
      }
      else
      {
        d[4*i+0] = 255;
        d[4*i+1] = 255;
        d[4*i+2] = ((i - 55 - 183) * 255) / 18;
        d[4*i+3] = i;
      }
    }
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, d.get());
  }

  template <class T>
  void denoise(volume<T> &img)
  {
    unique_ptr<T []> n(new T[img.width() * img.height() * img.depth()]);
    T *p = n.get(), *q = img.data();

    vector<T> v;
    v.reserve(5 * 5);
    for (int z = 0; z != int(img.depth()); ++z)
    {
      int min_z = -min(0, z), max_z = min(0, int(img.depth()) - 1 - z);
      for (int y = 0; y != int(img.height); ++y)
      {
        int min_y = -min(1, y), max_y = min(1, int(img.height()) -  1 - y);
        for (int x = 0; x != int(img.width); ++x)
        {
          int min_x = -min(1, x), max_x = min(1, int(img.width()) - 1 - x);
          for (int k = min_z; k <= max_z; ++k)
            for (int j = min_y; j <= max_y; ++j)
              for (int i = min_x; i <= max_x; ++i)
                v.push_back(*(q + (k*img.height()+j)*img.width()+i));

          nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
          q++;
          *p++ = v[v.size() / 2];
          v.clear();
        }
      }
    }

    img.data = move(n);
  }

  struct contour_info
  {
    size_t width, height;
    unique_ptr<float [][3]> pos;
    unique_ptr<float [][3]> norm;
    bool visible;

    contour_info(size_t width, size_t height)
      : width(width),
        height(height),
        pos(new float [width*height][3]),
        norm(new float[width*height][3]),
        visible(true)
    { }
  };
}

struct render_mip
  : public gl_content
{

  render_mip(function<void ()> &&update, const oct_scan &scan, observable<size_t> &key)
    : gl_content(move(update)), m_tomogram(scan.tomogram), m_contours(),
      m_key(key), m_key_observer(key, bind(&render_mip::key_changed, this)), m_frustum(true), xRot(0), yRot(0), zRot(0)
  {
    glewInit();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glEnable(GL_FRAMEBUFFER_SRGB);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat lightPosition[4] = { 0.5, 5.0, 7.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    QString shader_path = QCoreApplication::applicationDirPath() + "/../share/uocte/shader/";
    m_shader.addShaderFromSourceFile(QGLShader::Vertex, shader_path + "mip.vert");
    m_shader.addShaderFromSourceFile(QGLShader::Fragment, shader_path + "mip.frag");
    m_shader.link();
    cout << m_shader.log().toStdString();

    glGenTextures(4, m_tx);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_tx[0]);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE, m_tomogram.width(), m_tomogram.height(), m_tomogram.depth(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_tomogram.data());
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, m_tx[1]);
    make_blackbody_colormap();
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_tx[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_tx[3]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    m_shader.bind();
    m_shader.setUniformValue("tx", 0);
    m_shader.setUniformValue("colormap", 1);
    m_shader.setUniformValue("color", 2);
    m_shader.setUniformValue("depth", 3);
    m_shader.setUniformValue("dims", float(m_tomogram.width()), float(m_tomogram.height()), float(m_tomogram.depth()));
    m_shader.setUniformValue("size", scan.size[0], scan.size[1], scan.size[2]);
    m_shader.release();

    float n = sqrt(scan.size[0]*scan.size[0]+scan.size[1]*scan.size[1]+scan.size[2]*scan.size[2]) * 0.5f;
    for (size_t i = 0; i != scan.contours.size(); ++i)
    {
      auto it = scan.contours.begin();
      advance(it, i);
      const image<float> &c = it->second;

      m_contours.push_back(contour_info{c.width(), c.height()});

      const size_t vw = c.width();
      const size_t vh = c.height();
      const float iw = scan.size[0] / vw;
      const float ih = scan.size[2] / vh;
      const float iv = scan.size[1] / m_tomogram.height();
      for (size_t y = 0; y != vh; ++y)
      {
        for (size_t x = 0; x != vw; ++x)
        {
          float dx = ((x > 0 ? float(c(x-0, y-0)) - c(x-1, y-0) : 0.0f) + (x + 1 < vw ? float(c(x+1, y+0)) - c(x+0, y+0) : 0.0f)) * iv / iw;
          float dy = ((y > 0 ? float(c(x-0, y-0)) - c(x-0, y-1) : 0.0f) + (y + 1 < vh ? float(c(x+0, y+1)) - c(x+0, y+0) : 0.0f)) * iv / ih;
          float il = 1.0f / sqrt(1.0f + dx*dx + dy*dy);
          m_contours[i].pos[y*vw+x][0] = ((x + 0.5f) * iw - scan.size[0] * 0.5f) / n;
          m_contours[i].pos[y*vw+x][1] = (scan.size[1] * 0.5f - (c(x, y) + 0.5f) * iv) / n;
          m_contours[i].pos[y*vw+x][2] = ((y + 0.5f) * ih - scan.size[2] * 0.5f) / n;
          m_contours[i].norm[y*vw+x][0] = il * dx;
          m_contours[i].norm[y*vw+x][1] = il;
          m_contours[i].norm[y*vw+x][2] = il * dy;
        }
      }
    }
  }

 ~render_mip()
  {
    glDeleteTextures(4, m_tx);
  }

  void paint(const std::function<void (int, int, const char *)> &/*draw_text*/) override
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glLoadIdentity();
    glRotatef(xRot, 1.0, 0.0, 0.0);
    glRotatef(yRot, 0.0, 1.0, 0.0);
    glRotatef(zRot, 0.0, 0.0, 1.0);

    glColor3f(1.0f, 1.0f, 1.0f);
    for (contour_info &c: m_contours)
    {
      if (!c.visible)
        continue;

      for (size_t y = 1; y < c.height; ++y)
      {
        glBegin(GL_TRIANGLE_STRIP);
        for (size_t x = 0; x != c.width; ++x)
        {
          glNormal3fv(c.norm[(y-1)*c.width+x]);
          glVertex3fv(c.pos[(y-1)*c.width+x]);
          glNormal3fv(c.norm[(y-0)*c.width+x]);
          glVertex3fv(c.pos[(y-0)*c.width+x]);
        }
        glEnd();
      }
    }
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_tx[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, m_tx[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_tx[2]);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, m_color.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, m_color.get());
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_tx[3]);
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, m_depth.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, m_depth.get());

    m_shader.bind();

    glEnable(GL_CULL_FACE);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(+0.5f, -0.5f, -0.5f);
    glVertex3f(+0.5f, -0.5f, +0.5f);
    glVertex3f(-0.5f, -0.5f, +0.5f);
    glVertex3f(-0.5f, +0.5f, +0.5f);
    glVertex3f(-0.5f, +0.5f, -0.5f);
    glVertex3f(+0.5f, +0.5f, -0.5f);
    glVertex3f(+0.5f, -0.5f, -0.5f);
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(+0.5f, +0.5f, +0.5f);
    glVertex3f(-0.5f, +0.5f, +0.5f);
    glVertex3f(-0.5f, -0.5f, +0.5f);
    glVertex3f(+0.5f, -0.5f, +0.5f);
    glVertex3f(+0.5f, -0.5f, -0.5f);
    glVertex3f(+0.5f, +0.5f, -0.5f);
    glVertex3f(-0.5f, +0.5f, -0.5f);
    glVertex3f(-0.5f, +0.5f, +0.5f);
    glEnd();
    glDisable(GL_CULL_FACE);

    m_shader.release();
  }

  void resize(size_t width, size_t height) override
  {
    this->width = width;
    this->height = height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (m_frustum)
    {
      double PI = 3.14159265358979323846264338327950288419716939937510;
      double d = 1.0 / sin(15.0 / 180.0 * PI);
      double h = (d - 1.0) * tan(15.0 / 180.0 * PI);
      glFrustum(-h * width / height, h * width / height, -h, h, d - 1.0, d + 1.0);
      glTranslated(0.0, 0.0, -d);
    }
    else
    {
      glOrtho(-1.0 * width / height, 1.0 * width / height, -1.0, 1.0, -1.0, 1.0);
    }

    glMatrixMode(GL_MODELVIEW);

    m_depth.reset(new float[width*height]);
    m_color.reset(new uint8_t[4*width*height]);
    m_shader.bind();
    m_shader.setUniformValue("view", float(width), float(height));
    m_shader.release();
  }

  void mouse_press(size_t, size_t, size_t) override
  {
  }

  void mouse_release(size_t, size_t, size_t) override
  {
  }

  void mouse_move(size_t x, size_t y, size_t buttons) override
  {
    int dx = int(x) - int(this->x);
    int dy = int(y) - int(this->y);

    if (buttons & 1)
    {
      xRot += dy;
      yRot += dx;
    }
    if (buttons & 2)
    {
      xRot += dy;
      zRot += dx;
    }

    this->x = x;
    this->y = y;

    update();
  }

  void mouse_wheel(size_t, size_t, int) override
  {
  }

  void key_changed()
  {
    size_t i = (m_key != '0' ? m_key - '1' : 9);
    if (i < m_contours.size())
    {
      m_contours[i].visible = !m_contours[i].visible;
      update();
    }
  }

  const volume<uint8_t> &m_tomogram;
  vector<contour_info> m_contours;
  observable<size_t> &m_key;
  observer m_key_observer;
  QGLShaderProgram m_shader;
  bool m_frustum;
  unique_ptr<float []> m_depth;
  unique_ptr<uint8_t []> m_color;
  double xRot, yRot, zRot;
  size_t x, y, width, height;
  GLuint m_tx[4];

};

unique_ptr<gl_content> make_render_mip(function<void ()> &&update, const oct_scan &scan, observable<size_t> &key)
{
  return unique_ptr<gl_content>(new render_mip(move(update), scan, key));
}
