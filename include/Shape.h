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
#include "VertexBuffer.h"

namespace umfeld {

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
        VertexBuffer*       byovbo{nullptr}; // NOTE this allows a shape to supply a dedicated vertex buffer i.e
                                             //      - `vertices` should be empty and will be ignored
                                             //      - all shapes are considered fill shape
                                             //      - only certain modes are allowed or supported ( e.g POINTS, LINES, LINE_LOOP, LINE_STRIP, TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN  )
    };
} // namespace umfeld