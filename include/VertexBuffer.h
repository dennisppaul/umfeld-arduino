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

#include <vector>

#include "Umfeld.h"
#include "Vertex.h"

namespace umfeld {
    class VertexBuffer {
    public:
        VertexBuffer(const VertexBuffer&)            = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;
        VertexBuffer(VertexBuffer&& other) noexcept;
        VertexBuffer& operator=(VertexBuffer&& other) noexcept;

        VertexBuffer();
        ~VertexBuffer();

        void                 add_vertex(const Vertex& vertex);
        void                 add_vertices(const std::vector<Vertex>& new_vertices);
        void                 draw();
        void                 clear();
        void                 update();
        std::vector<Vertex>& vertices_data() { return _vertices; }
        void                 init();
        void                 set_shape(int shape, bool map_to_opengl_draw_mode = true);
        int                  get_shape() const { return native_opengl_shape; }

    private:
        const int           VBO_BUFFER_CHUNK_SIZE_BYTES = 1024 * 16 * sizeof(Vertex);
        std::vector<Vertex> _vertices;
        int                 vbo                = 0;
        int                 vao                = 0;
        bool                vao_supported      = false;
        bool                initial_upload     = false;
        bool                buffer_initialized = false;
        int                 server_buffer_size = 0;
        bool                dirty              = false;
        int                 native_opengl_shape;

        static bool isContextValid();
        void        enable_vertex_attributes() const;
        static void disable_vertex_attributes();
        void        checkVAOSupport();
        bool        needs_buffer_resize(size_t current_size) const;
        bool        needs_buffer_shrink(size_t current_size) const;
        void        upload_with_resize(size_t current_size, size_t required_bytes);

        // TODO move to OGL3 collection and share with shape renderer
        static void OGL3_disable_vertex_attributes();
        static void OGL3_enable_vertex_attributes();
    };
} // namespace umfeld