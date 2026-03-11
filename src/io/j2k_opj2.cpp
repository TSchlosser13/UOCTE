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

  void print_callback(const char *, void *) {
    //fputs(msg, stderr);
  }

  struct chunk_t
  {
    char *buf;
    size_t pos, size;
  };

  OPJ_SIZE_T my_read(char *buffer, OPJ_SIZE_T n_bytes, chunk_t *c)
  {
    n_bytes = min(n_bytes, c->size - c->pos);
    copy_n(c->buf + c->pos, n_bytes, buffer);
    return n_bytes ? n_bytes : (OPJ_SIZE_T)-1;
  }

  OPJ_SIZE_T my_write(void *, OPJ_SIZE_T, chunk_t *)
  {
    throw runtime_error("attemted write");
  }

  OPJ_OFF_T my_skip(OPJ_OFF_T n_bytes, chunk_t *c)
  {
    return (c->pos += n_bytes) <= c->size ? n_bytes : -1;
  }

  OPJ_BOOL my_seek(OPJ_OFF_T n_bytes, chunk_t *c)
  {
    return c->pos = n_bytes, OPJ_TRUE;
  }

  void my_free(chunk_t &)
  {
  }

  struct j2k_decoder
  {
    opj_codec_t *m;
    j2k_decoder()
      : m(opj_create_decompress(OPJ_CODEC_J2K))
    {
      if (!m)
        throw runtime_error("failed to create jpeg2000 decoder");

      opj_set_info_handler(m, print_callback, 0);
      opj_set_warning_handler(m, print_callback, 0);
      opj_set_error_handler(m, print_callback, 0);
    }

   ~j2k_decoder()
    {
      opj_destroy_codec(m);
    }

    j2k_decoder(const j2k_decoder &) = delete;
    j2k_decoder &operator=(const j2k_decoder &) = delete;

    void setup(opj_dparameters &parameters)
    {
      if (!opj_setup_decoder(m, &parameters))
        throw runtime_error("failed to setup jpeg2000 decoder");
    }
  };

  struct j2k_stream
  {
    opj_stream_t *m;
    j2k_stream(chunk_t &c)
      : m(opj_stream_default_create(OPJ_TRUE))
    {
      if (!m)
        throw runtime_error("failed to create jpeg2000 stream");

      opj_stream_set_user_data(m, &c, (opj_stream_free_user_data_fn)my_free);
      opj_stream_set_user_data_length(m, c.size);
      opj_stream_set_read_function(m, (opj_stream_read_fn)my_read);
      opj_stream_set_write_function(m, (opj_stream_write_fn)my_write);
      opj_stream_set_skip_function(m, (opj_stream_skip_fn)my_skip);
      opj_stream_set_seek_function(m, (opj_stream_seek_fn)my_seek);
    }

   ~j2k_stream()
    {
      opj_stream_destroy(m);
    }

    j2k_stream(const j2k_stream &) = delete;
    j2k_stream &operator=(const j2k_stream &) = delete;
  };

  struct j2k_image
  {
    j2k_decoder &m_decoder;
    j2k_stream &m_stream;
    opj_image_t *m;
    j2k_image(j2k_decoder &decoder, j2k_stream &stream)
      : m_decoder(decoder), m_stream(stream)
    {
      if (!opj_read_header(m_stream.m, m_decoder.m, &m))
        throw runtime_error("failed to decode jpeg2000 header");
    }

   ~j2k_image()
    {
      opj_image_destroy(m);
    }

    j2k_image(const j2k_image &) = delete;
    j2k_image &operator=(const j2k_image &) = delete;

    image<uint8_t> operator()() const
    {
      if (!opj_decode(m_decoder.m, m_stream.m, m) || !opj_end_decompress(m_decoder.m, m_stream.m))
        throw runtime_error("failed to decode jpeg2000 image");

      image<uint8_t> img(m->numcomps, m->x1 - m->x0, m->y1 - m->y0);
      for (size_t y = 0; y != img.height(); ++y)
        for (size_t x = 0; x != img.width(); ++x)
          for (size_t c = 0; c != img.channels(); ++c)
            img(c, x, y) = m->comps[c].data[y*img.width()+x];

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

  chunk_t c{buf, 0, bufsize};
  j2k_stream stream(c);
  return j2k_image(decoder, stream)();
}
