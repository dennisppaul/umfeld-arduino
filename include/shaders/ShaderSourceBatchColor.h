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
    inline ShaderSource shader_source_batch_color{
        .vertex   = R"(
layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec4 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec3 aTexCoord;
layout(location = 4) in uint aTransformID;
layout(location = 5) in uint aUserdata;

out vec4 vColor;

layout(std140) uniform Transforms {
    mat4 uModel[256];
};

uniform mat4 uViewProj;

void main() {
    mat4 M = uModel[aTransformID];
    gl_Position = uViewProj * M * aPosition;
    vColor = aColor;
}
        )",
        .fragment = R"(
in vec4 vColor;
out vec4 fragColor;
void main() {
    fragColor = vColor;
}
        )",
        .geometry = ""};
}
