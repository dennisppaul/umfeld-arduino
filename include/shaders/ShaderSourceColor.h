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
    inline ShaderSource shader_source_color{
        .vertex   = R"(
layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec4 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec3 aTexCoord;
layout(location = 4) in uint a_transform_id;
layout(location = 5) in uint aUserdata;

layout(std140) uniform Transforms {
    mat4 uModel[256];
};

out vec4 v_color;

uniform mat4 u_model_matrix;
uniform mat4 u_view_projection_matrix;

void main() {
    mat4 M;
    if (a_transform_id == 0u) {
        M = u_model_matrix;
    } else {
        M = uModel[a_transform_id - 1u];
    }
    gl_Position = u_view_projection_matrix * M * aPosition;
    v_color = aColor;
}
        )",
        .fragment = R"(
in vec4 v_color;

out vec4 v_frag_color;

void main() {
    v_frag_color = v_color;
}
        )",
        .geometry = ""};
}
