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

/*
 * consider add a fallback model matrix:

layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec4 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec3 aTexCoord;
layout(location = 4) in uint aTransformID;
layout(location = 5) in uint aUserdata;

layout(std140) uniform Transforms {
    mat4 uModel[256];
};

uniform mat4 uModelFallback;

void main() {
    mat4 M;
    if (aTransformID == 255u) {
        M = uModelFallback;
    } else {
        M = uModel[aTransformID];
    }
    gl_Position = uViewProj * M * aPosition;
    vColor = aColor;
}

// draw shapes with custom vertex buffers
for (size_t i = 0; i < chunkSize; ++i) {
    const auto* s = shapes_to_render[offset + i];
    if (s->vertex_buffer != nullptr) {
        // Set fallback transform for custom vertex buffers
        glUniformMatrix4fv(glGetUniformLocation(current_shader_program, "uModelFallback"),
                          1, GL_FALSE, &s->model[0][0]);
        s->vertex_buffer->draw();
    }
}

*/


namespace umfeld {
    inline ShaderSource shader_source_color{
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
