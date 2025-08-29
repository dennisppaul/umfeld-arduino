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
// TODO add texture support to point shader
namespace umfeld {
    inline ShaderSource shader_source_point{
        .vertex   = R"(
layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec4 aNormal; // offset
layout(location = 2) in vec4 aColor;
layout(location = 4) in uint a_transform_id;

layout(std140) uniform Transforms {
    mat4 uModel[256];
};

out vec4 v_color;

uniform mat4 u_model_matrix;
uniform mat4 u_projection_matrix;
uniform mat4 u_view_matrix;

uniform vec4 u_viewport;
uniform int  u_perspective;

void main() {
    mat4 M;
    if (a_transform_id == 0u) {
        M = u_model_matrix;
    } else {
        M = uModel[a_transform_id - 1u];
    }
    mat4 modelviewMatrix =  u_view_matrix * M;
    mat4 projectionMatrix = u_projection_matrix;
    vec2 offset = aNormal.xy;

    vec4 pos = modelviewMatrix * aPosition;
    vec4 clip = projectionMatrix * pos;

    // Perspective ---
    // convert from world to clip by multiplying with projection scaling factor
    // invert Y, projections in Processing invert Y
    vec2 perspScale = (projectionMatrix * vec4(1, -1, 0, 0)).xy;

    // formula to convert from clip space (range -1..1) to screen space (range 0..[width or height])
    // screen_p = (p.xy/p.w + <1,1>) * 0.5 * u_viewport.zw

    // No Perspective ---
    // multiply by W (to cancel out division by W later in the pipeline) and
    // convert from screen to clip (derived from clip to screen above)
    vec2 noPerspScale = clip.w / (0.5 * u_viewport.zw);

    gl_Position.xy = clip.xy + offset.xy * mix(noPerspScale, perspScale, float(u_perspective > 0));
    gl_Position.zw = clip.zw;

    v_color = aColor;
}
        )",
        .fragment = R"(
in vec4 v_color;

out vec4 v_frag_color;

void main() {
    v_frag_color = v_color;
}
        )"};
}
