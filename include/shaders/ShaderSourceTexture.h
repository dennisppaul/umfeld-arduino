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
    inline ShaderSource shader_source_texture{
        .vertex   = R"(
layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec4 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec3 aTexCoord;
layout(location = 4) in uint aTransformID;
layout(location = 5) in uint aUserdata;

layout(std140) uniform Transforms {
    mat4 uModel[256];
};

out vec4 vColor;
out vec2 vTexCoord;

uniform mat4 uModelMatrixFallback;
uniform mat4 uViewProjectionMatrix;

void main() {
    mat4 M;
    if (aTransformID == 0u) {
        M = uModelMatrixFallback;
    } else {
        M = uModel[aTransformID - 1u];
    }
    gl_Position = uViewProjectionMatrix * M * aPosition;
    vTexCoord   = aTexCoord.xy;
    vColor      = aColor;
}
        )",
        .fragment = R"(
in vec4 vColor;
in vec2 vTexCoord;

out vec4 FragColor;

uniform sampler2D uTextureUnit;

void main() {
    FragColor = texture(uTextureUnit, vTexCoord) * vColor;
}
        )"};
}
