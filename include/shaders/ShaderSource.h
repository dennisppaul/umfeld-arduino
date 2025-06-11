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

namespace umfeld {
    struct ShaderSource {
        // NOTE that it is important to start the header immediately with `#version` to avoid
        //      issues with the shader compiler ( e.g on Windows ).
#if defined(OPENGL_ES_3_0)
        std::string header = R"(#version 300 es
                                precision mediump float;
                                precision mediump int;
        )";
#elif defined(OPENGL_3_3_CORE)
        inline static std::string header = R"(#version 330 core
        )";
#elif defined(OPENGL_2_0)
        std::string header = R"(#version 110
        )";
#endif
        const char* vertex;
        const char* fragment;
        const char* geometry;

        static std::string get_versioned_source(const std::string& source) {
            if (source.empty()) {
                return "";
            }
            // NOTE add a newline after the header to ensure proper formatting
            if (header.back() != '\n') {
                return header + '\n' + source;
            }
            return header + source;
        }
    };
} // namespace umfeld
