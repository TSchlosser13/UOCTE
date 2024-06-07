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

#include <algorithm>
#include <stdexcept>

#include <openjpeg.h>

using namespace std;

namespace
{

  struct j2k_decoder
  {
    opj_dinfo_t *m;
    j2k_decoder()
      : m(opj_create_decompress(CODEC_J2K))
    {
      if (!m)
        throw runtime_error("failed to create jpeg2000 decoder");
    }

   ~j2k_decoder()
    {
      opj_destroy_decompress(m);
    }

    j2k_decoder(const j2k_decoder &) = delete;
    j2k_decoder &operator=(const j2k_decoder &) = delete;

    void setup(opj_dparameters &parameters)
    {
      opj_setup_decoder(m, &parameters);
    }
  };

  struct j2k_stream
  {
    opj_cio_t *m;
    j2k_stream(j2k_decoder &decoder, char *buf, size_t bufsize)
      : m(opj_cio_open(reinterpret_cast<opj_common_ptr>(decoder.m), reinterpret_cast<unsigned char *>(buf), bufsize))
    {
      if (!m)
        throw runtime_error("failed to create jpeg2000 stream");
    }

   ~j2k_stream()
    {
      opj_cio_close(m);
    }

    j2k_stream(const j2k_stream &) = delete;
    j2k_stream &operator=(const j2k_stream &) = delete;
  };

  struct j2k_image
  {
    opj_image_t *m;
    j2k_image(j2k_decoder &decoder, j2k_stream &stream)
      : m(opj_decode(decoder.m, stream.m))
    {
      if (!m)
        throw runtime_error("failed to decode jpeg2000 image");
    }

   ~j2k_image()
    {
      opj_image_destroy(m);
    }

    j2k_image(const j2k_image &) = delete;
    j2k_image &operator=(const j2k_image &) = delete;

    image<uint8_t> operator()() const
    {
      image<uint8_t> img(m->numcomps, m->x1 - m->x0, m->y1 - m->y0);
      for (size_t y = 0; y != img.height(); ++y)
        for (size_t x = 0; x != img.width(); ++x)
          for (size_t c = 0; c != img.channels(); ++c)
            img(c, x, y) = m->comps[c].data[y*img.width() + x];

      return img;
    }
  };

}

image<uint8_t> decode_j2k(char *buf, size_t bufsize)
{
  j2k_decoder decoder;

  opj_dparameters_t parameters;
  opj_set_default_decoder_parameters(&parameters);
  decoder.setup(parameters);

  j2k_stream stream(decoder, buf, bufsize);
  return j2k_image(decoder, stream)();
}
