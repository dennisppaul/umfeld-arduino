/*
 * Umfeld
 *
 * This file is part of the *Umfeld* library (https://github.com/dennisppaul/umfeld).
 * Copyright (c) 2025 Dennis P Paul.
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "ShaderSource.h"

namespace umfeld {
    inline ShaderSource shader_source_fullscreen{
        .vertex   = R"(
out vec2 vUV;
void main() {
    // Single fullscreen triangle (covers NDC [-1,1]^2)
    const vec2 pos[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 p = pos[gl_VertexID];
    vUV = p * 0.5 + 0.5;   // map from NDC to [0,1]
    vUV = vec2(vUV.x, 1.0 - vUV.y); // flip texture coords
    gl_Position = vec4(p, 0.0, 1.0);
}
        )",
        .fragment = R"(
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTextureUnit;
void main() {
    FragColor = texture(uTextureUnit, vUV);
}
        )"};
}
