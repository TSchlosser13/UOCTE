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

#include "j2k.hpp"

#include <stdexcept>

#include <jasper/jasper.h>

using namespace std;

namespace
{

  struct j2k_stream
  {
    jas_stream_t *m;
    j2k_stream(char *buf, size_t size)
      : m(jas_stream_memopen(buf, size))
    {
      if (!m)
        throw runtime_error("failed to create jpeg2000 stream");
    }

   ~j2k_stream()
    {
      jas_stream_close(m);
    }

    j2k_stream(const j2k_stream &) = delete;
    j2k_stream &operator=(const j2k_stream &) = delete;
  };

  struct j2k_image
  {
    jas_image_t *m;
    j2k_image(const j2k_stream &stream)
      : m(jas_image_decode(stream.m, jas_image_getfmt(stream.m), 0))
    {
      if (!m)
        throw runtime_error("failed to decode jpeg2000");
    }

    image<uint8_t> operator()() const
    {
      image<uint8_t> img(jas_image_numcmpts(m), jas_image_width(m), jas_image_height(m));
      for (size_t y = 0; y != img.height(); ++y)
        for (size_t x = 0; x != img.width(); ++x)
          for (size_t c = 0; c != img.channels(); ++c)
            img(c, x, y) = jas_image_readcmptsample(m, c, x, y);

      return img;
    }

   ~j2k_image()
    {
      jas_image_destroy(m);
    }

    j2k_image(const j2k_image &) = delete;
    j2k_image &operator=(const j2k_image &) = delete;
  };

}

image<uint8_t> decode_j2k(char *buf, size_t bufsize)
{
  jas_init();
  return j2k_image(j2k_stream(buf, bufsize))();
}
