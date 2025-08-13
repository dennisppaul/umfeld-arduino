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

namespace umfeld {
    struct StrokeState {
        float stroke_weight{1};
        int   stroke_join_mode{BEVEL_FAST};
        int   stroke_cap_mode{PROJECT};
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
} // namespace umfeld