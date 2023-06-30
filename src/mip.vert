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

uniform vec3 dims;
uniform vec3 size;
varying vec3 p;
varying vec4 v;

void main(void)
{
  float n = 0.5 * length(size);
  p = gl_Vertex.xyz / gl_Vertex.w;
  gl_Position = gl_ModelViewProjectionMatrix * (vec4(size.x, -size.y, size.z, n) * gl_Vertex);
  v = (gl_Vertex + (gl_ModelViewProjectionMatrixInverse * vec4(0.0, 0.0, -2.0 * gl_Position.w, 0.0)) / vec4(size.x, -size.y, size.z, n)) / gl_Vertex.w;
}
