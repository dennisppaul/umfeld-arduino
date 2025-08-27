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
    inline ShaderSource shader_source_color_lights{
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

out vec4 vColor;
out vec4 vBackColor;

uniform mat4 u_model_matrix;
uniform mat4 u_view_projection_matrix;
uniform mat4 u_view_matrix;
// uniform mat3 normalMatrix; // TODO "normalMatrix as Transform" add it via Transform block later

uniform vec4 ambient;
uniform vec4 specular;
uniform vec4 emissive;
uniform float shininess;

uniform int lightCount;
uniform vec4 lightPosition[8];
uniform vec3 lightNormal[8];
uniform vec3 lightAmbient[8];
uniform vec3 lightDiffuse[8];
uniform vec3 lightSpecular[8];
uniform vec3 lightFalloff[8];
uniform vec2 lightSpot[8];

const float zero_float = 0.0;
const float one_float = 1.0;
const vec3 zero_vec3 = vec3(0);

float falloffFactor(vec3 lightPos, vec3 vertPos, vec3 coeff) {
    vec3 lpv = lightPos - vertPos;
    vec3 dist = vec3(one_float);
    dist.z = dot(lpv, lpv);
    dist.y = sqrt(dist.z);
    return one_float / dot(dist, coeff);
}

float spotFactor(vec3 lightPos, vec3 vertPos, vec3 lightNorm, float minCos, float spotExp) {
    vec3 lpv = normalize(lightPos - vertPos);
    vec3 nln = -one_float * lightNorm;
    float spotCos = dot(nln, lpv);
    return spotCos <= minCos ? zero_float : pow(spotCos, spotExp);
}

float lambertFactor(vec3 lightDir, vec3 vecNormal) {
    return max(zero_float, dot(lightDir, vecNormal));
}

float blinnPhongFactor(vec3 lightDir, vec3 vertPos, vec3 vecNormal, float shine) {
    vec3 np = normalize(vertPos);
    vec3 ldp = normalize(lightDir - np);
    return pow(max(zero_float, dot(ldp, vecNormal)), shine);
}

void main() {
    mat4 M;
    if (a_transform_id == 0u) {
        M = u_model_matrix;
    } else {
        M = uModel[a_transform_id - 1u];
    }
    mat4 MV = u_view_matrix * M;
    gl_Position = u_view_projection_matrix * M * aPosition;

    // TODO "normalMatrix as Transform" better get this from transform
    //mat3 normalMatrix = mat3(transpose(inverse(MV)));
    mat3 normalMatrix = mat3(MV);
    vec3 n = normalMatrix * aNormal.xyz;

    vec3 ecVertex = vec3(MV * aPosition);
    vec3 ecNormal = normalize(n);
    vec3 ecNormalInv = -ecNormal;

    vec3 totalAmbient = vec3(0.0);
    vec3 totalFrontDiffuse = vec3(0.0);
    vec3 totalFrontSpecular = vec3(0.0);
    vec3 totalBackDiffuse = vec3(0.0);
    vec3 totalBackSpecular = vec3(0.0);

    for (int i = 0; i < 8; i++) {
        if (i >= lightCount) break;

        vec3 lightPos = lightPosition[i].xyz;
        bool isDir = lightPosition[i].w < 1.0;
        float spotCos = lightSpot[i].x;
        float spotExp = lightSpot[i].y;

        vec3 lightDir;
        float falloff = 1.0;
        float spotf = 1.0;

        if (isDir) {
            lightDir = -lightNormal[i];
        } else {
            falloff = falloffFactor(lightPos, ecVertex, lightFalloff[i]);
            lightDir = normalize(lightPos - ecVertex);
        }

        if (spotExp > 0.0) {
            spotf = spotFactor(lightPos, ecVertex, lightNormal[i], spotCos, spotExp);
        }

        if (any(greaterThan(lightAmbient[i], zero_vec3))) {
            totalAmbient += lightAmbient[i] * falloff;
        }

        if (any(greaterThan(lightDiffuse[i], zero_vec3))) {
            totalFrontDiffuse += lightDiffuse[i] * falloff * spotf * lambertFactor(lightDir, ecNormal);
            totalBackDiffuse += lightDiffuse[i] * falloff * spotf * lambertFactor(lightDir, ecNormalInv);
        }

        if (any(greaterThan(lightSpecular[i], zero_vec3))) {
            totalFrontSpecular += lightSpecular[i] * falloff * spotf * blinnPhongFactor(lightDir, ecVertex, ecNormal, shininess);
            totalBackSpecular += lightSpecular[i] * falloff * spotf * blinnPhongFactor(lightDir, ecVertex, ecNormalInv, shininess);
        }
    }

    vColor = vec4(totalAmbient, 0.0) * ambient +
             vec4(totalFrontDiffuse, 1.0) * aColor +
             vec4(totalFrontSpecular, 0.0) * specular +
             vec4(emissive.rgb, 0.0);

    vBackColor = vec4(totalAmbient, 0.0) * ambient +
                 vec4(totalBackDiffuse, 1.0) * aColor +
                 vec4(totalBackSpecular, 0.0) * specular +
                 vec4(emissive.rgb, 0.0);
}
        )",
        .fragment = R"(
in vec4 vColor;
in vec4 vBackColor;

out vec4 FragColor;

void main() {
    FragColor = gl_FrontFacing ? vColor : vBackColor;
}
        )",
        .geometry = ""};
}
