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
#include "UmfeldTypes.h"

namespace umfeld {

    class VertexBuffer;
    class PShader;

    struct Shape {
        ShapeMode           mode{POLYGON};
        StrokeState         stroke;
        bool                filled{true};
        std::vector<Vertex> vertices;
        glm::mat4           model{};
        glm::vec3           center_object_space{};
        bool                transparent{};
        bool                closed{false};
        float               depth{};
        uint16_t            texture_id{TEXTURE_NONE};
        bool                light_enabled{false};
        LightingState       lighting;
        /**
         * a shape can supply a custom vertex buffer.
         * - vertex buffer must be initialized and valid
         * - `vertices` will be ignored ( and can be left empty )
         * - shapes are ( maybe ) rendered in a dedicated path
         */
        VertexBuffer* vertex_buffer{nullptr};
        /**
         * a shape can supply a custom shader.
         * - shader
         */
        PShader* shader{nullptr};
    };
} // namespace umfeld