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

#include <glm/glm.hpp>

#include "UmfeldConstants.h"

#define MAKE_UNIFORM(var) \
    Uniform var { .name = #var }

namespace umfeld {
    struct ShaderUniforms {

        struct Uniform {
            uint32_t    id = UNINITIALIZED;
            const char* name;
        };

        // cached uniform locations
        // TODO move to UmfeldTypes and replace GLuint with uint32_t
        //      remove all OpenGL deps
        //      maybe rename to `DefaultShaderUniforms`
        enum UNIFORM_LOCATION_STATE : uint32_t {
            UNINITIALIZED = 0xFFFFFFFE,
            NOT_FOUND     = 0xFFFFFFFF,
            INITIALIZED   = 0, // NOTE `0` is the first valid value
        };

        Uniform u_model_matrix{.name = "u_model_matrix"};
        Uniform u_projection_matrix{.name = "u_projection_matrix"};
        Uniform u_view_matrix{.name = "u_view_matrix"};
        Uniform u_view_projection_matrix{.name = "u_view_projection_matrix"};
        Uniform u_texture_unit{.name = "u_texture_unit"};
        Uniform u_viewport{.name = "u_viewport"};
        Uniform u_perspective{.name = "u_perspective"};
        Uniform u_scale{.name = "u_scale"};
        Uniform ambient{.name = "ambient"}; /* lighting uniforms */
        Uniform specular{.name = "specular"};
        Uniform emissive{.name = "emissive"};
        Uniform shininess{.name = "shininess"};
        Uniform lightCount{.name = "lightCount"};
        Uniform lightPosition{.name = "lightPosition"};
        Uniform lightNormal{.name = "lightNormal"};
        Uniform lightAmbient{.name = "lightAmbient"};
        Uniform lightDiffuse{.name = "lightDiffuse"};
        Uniform lightSpecular{.name = "lightSpecular"};
        Uniform lightFalloff{.name = "lightFalloff"};
        MAKE_UNIFORM(lightSpot);
        // MAKE_UNIFORM(normalMatrix);

        static bool is_uniform_available(const uint32_t loc) { return loc != UNINITIALIZED && loc != NOT_FOUND; }
    };

    struct ShaderProgram {
        uint32_t       id{0};
        ShaderUniforms uniforms;
    };

    struct ShapeState {
        ShapeMode mode{POLYGON};
        bool      started{false};

        void reset() {
            mode    = POLYGON;
            started = false;
        }
    };

    struct StrokeState {
        float point_weight{1};
        float stroke_weight{1};
        int   stroke_join_mode{BEVEL_FAST};
        int   stroke_cap_mode{SQUARE};
        float stroke_join_round_resolution{glm::radians(20.0f)};
        float stroke_cap_round_resolution{glm::radians(20.0f)}; // // 20° resolution i.e 18 segment for whole circle
        float stroke_join_miter_max_angle{163.0f};
    };

    struct ColorState : glm::vec4 {
        bool active = false;
    };

    struct StyleState {
        ColorState stroke;
        ColorState fill;
        float      strokeWeight;
        // TODO add style values like tint, blend mode, etc.
    };

    struct LightingState {
        // Light type constants
        static constexpr int MAX_LIGHTS  = 8;
        static constexpr int AMBIENT     = 0;
        static constexpr int DIRECTIONAL = 1;
        static constexpr int POINT       = 2;
        static constexpr int SPOT        = 3;

        // Light arrays
        int       lightType[MAX_LIGHTS]{};
        glm::vec4 lightPositions[MAX_LIGHTS]{};
        glm::vec3 lightNormals[MAX_LIGHTS]{};
        glm::vec3 lightAmbientColors[MAX_LIGHTS]{};
        glm::vec3 lightDiffuseColors[MAX_LIGHTS]{};
        glm::vec3 lightSpecularColors[MAX_LIGHTS]{};
        glm::vec3 lightFalloffCoeffs[MAX_LIGHTS]{};
        glm::vec2 lightSpotParams[MAX_LIGHTS]{};

        // Current light settings
        glm::vec3 currentLightSpecular         = glm::vec3(0.0f);
        float     currentLightFalloffConstant  = 1.0f;
        float     currentLightFalloffLinear    = 0.0f;
        float     currentLightFalloffQuadratic = 0.0f;

        // Lighting state
        int lightCount = 0;

        // Shader uniforms (for convenience, can be set directly)
        glm::mat3 normalMatrix = glm::mat3(1.0f);
        glm::mat4 texMatrix    = glm::mat4(1.0f);

        glm::vec4 ambient   = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        glm::vec4 specular  = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        glm::vec4 emissive  = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        float     shininess = 64.0f;
    };
} // namespace umfeld