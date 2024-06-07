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

uniform sampler1D colormap;
uniform sampler2D color;
uniform sampler2D depth;
uniform sampler3D tx;
uniform vec3 dims;
uniform vec2 view;
varying vec3 p;
varying vec4 v;

void main(void)
{
  vec3 q = p + (v.xyz / v.w - p) * max(0.0, gl_FragCoord.z - texture2D(depth, gl_FragCoord.xy / view).r);
  q = dims * (q + 0.5); // where the ray enters the volume in texel space
  vec3 d = normalize(dims * (v.xyz - p * v.w)); // ray direction in texel space
  vec3 r = vec3(abs(d.x) >= 1e-6 ? 1.0 / d.x : 1e6, abs(d.y) >= 1e-6 ? 1.0 / d.y : 1e6, abs(d.z) >= 1e-6 ? 1.0 / d.z : 1e6); // work around division by zero
  vec3 t; // current texel's center in texel space
  t.x = (r.x < 0.0 ? ceil(q.x) - 0.5 : floor(q.x) + 0.5);
  t.y = (r.y < 0.0 ? ceil(q.y) - 0.5 : floor(q.y) + 0.5);
  t.z = (r.z < 0.0 ? ceil(q.z) - 0.5 : floor(q.z) + 0.5);
  vec3 l = (t + 0.5 * sign(r) - q) * r; // next texel space x,y,z intersection with current texel boundaries

  vec4 c = texture2D(color, gl_FragCoord.xy / view);
  float v = 0.0, to = 0.0, tc = 0.0;
  while (all(greaterThanEqual(t, vec3(0.0, 0.0, 0.0))) && all(lessThanEqual(t, dims))) // inside volume
  {
    to = tc; // remember last intersection
    tc = min(min(l.x, l.y), l.z); // next intersection

    v = max(v, texture3D(tx, t / dims).r);

    // skip to next texel
    vec3 b = vec3(float(tc == l.x), float(tc == l.y), float(tc == l.z));
    t += b * sign(r);
    l += b * abs(r);
  }

  gl_FragColor = (texture1D(colormap, v) + texture2D(color, gl_FragCoord.xy / view)) / 2.0;
}
