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

// ⚠️⚠️⚠️ Work in progress! not tested and still missing a lot e.g shrinking ⚠️⚠️⚠️

#pragma once

#include <vector>
#include <algorithm>
#include "UmfeldSDLOpenGL.h"
#include "Vertex.h"

#if defined(OPENGL_3_3_CORE) || defined(OPENGL_ES_3_0)
#define USE_VAO 1
#elif defined(OPENGL_2_0)
#define USE_VAO 0
#endif

namespace umfeld {

    // Describes one attribute in your interleaved vertex
    struct VertexAttribute {
        GLuint    index;      // shader location
        GLint     size;       // e.g. 2,3,4
        GLenum    type;       // e.g. GL_FLOAT
        GLboolean normalized; // GL_FALSE or GL_TRUE
        GLsizei   stride;     // byte stride of entire vertex
        size_t    offset;     // byte offset of this field in the vertex
    };

    class VertexBufferGeneric {
    public:
        // attrs: list of your vertex attributes
        // vertexByteSize: sizeof(YourVertexStruct)
        VertexBufferGeneric(const std::vector<VertexAttribute>& attrs, const size_t vertexByteSize)
            : attributes(attrs),
              vertexSize(vertexByteSize),
              vao(0),
              vbo(0),
              capacity(0),
              vertexCount(0) {}

        ~VertexBufferGeneric() {
            cleanup();
        }

        // Must be called once after GL context is ready (and after your loader is init)
        void init() {
#if USE_VAO
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
#endif
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            // Allocate zero bytes for now; first real allocation comes in update()
#if USE_VAO
            setupAttributes();
            glBindVertexArray(0);
#else
            glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
        }

        // Upload 'count' vertices from 'data'
        // If you exceed the current capacity, the buffer is re-allocated
        void update(const void* data, const size_t count) {
            vertexCount          = count;
            size_t requiredBytes = count * vertexSize;

            // grow buffer if needed (double strategy)
            if (requiredBytes > capacity) {
                size_t newCap = std::max(requiredBytes, capacity * 2);
                if (newCap == 0) {
                    newCap = requiredBytes;
                }
                capacity = newCap;

                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                // orphan old storage, allocate new
                glBufferData(GL_ARRAY_BUFFER, capacity, nullptr, GL_DYNAMIC_DRAW);
            }

            // upload actual data
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, requiredBytes, data);
        }

        // Draws with the given primitive mode (e.g. GL_TRIANGLES)
        void draw(const GLenum mode) const {
#if USE_VAO
            glBindVertexArray(vao);
#else
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            setupAttributes();
#endif
            glDrawArrays(mode, 0, static_cast<GLsizei>(vertexCount));
#if USE_VAO
            glBindVertexArray(0);
#else
            // (optional) disable attributes if you want
#endif
        }

        // Frees GPU resources
        void cleanup() {
            if (vbo) {
                glDeleteBuffers(1, &vbo);
                vbo = 0;
            }
#if USE_VAO
            if (vao) {
                glDeleteVertexArrays(1, &vao);
                vao = 0;
            }
#endif
        }

    private:
        void setupAttributes() const {
            for (const auto& a: attributes) {
                glEnableVertexAttribArray(a.index);
                glVertexAttribPointer(
                    a.index, a.size, a.type, a.normalized,
                    a.stride,
                    reinterpret_cast<const void*>(a.offset));
            }
        }

        std::vector<VertexAttribute> attributes;
        size_t                       vertexSize; // bytes per vertex
        GLuint                       vao, vbo;
        size_t                       capacity;    // current GPU‐side capacity in bytes
        size_t                       vertexCount; // how many vertices last uploaded
    };

    inline void use_case() {
        // Define your vertex struct:
        // struct Vertex {
        //     glm::aligned_vec4 position;
        //     glm::aligned_vec4 normal;
        //     glm::aligned_vec4 color;
        //     glm::vec3         tex_coord;
        //     float             userdata{0};
        // };

        // Describe its layout:
        const std::vector<VertexAttribute> attributes = {
            {Vertex::ATTRIBUTE_LOCATION_POSITION, Vertex::ATTRIBUTE_SIZE_POSITION, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position)},
            {Vertex::ATTRIBUTE_LOCATION_NORMAL, Vertex::ATTRIBUTE_SIZE_NORMAL, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normal)},
            {Vertex::ATTRIBUTE_LOCATION_COLOR, Vertex::ATTRIBUTE_SIZE_COLOR, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, color)},
            {Vertex::ATTRIBUTE_LOCATION_TEXCOORD, Vertex::ATTRIBUTE_SIZE_TEXCOORD, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, tex_coord)},
            {Vertex::ATTRIBUTE_LOCATION_USERDATA, Vertex::ATTRIBUTE_SIZE_USERDATA, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, userdata)},
        };

        // In your initialization (after GL loader):
        VertexBufferGeneric vertex_buffer_default(attributes, sizeof(Vertex));
        vertex_buffer_default.init();

        // Whenever your mesh data changes (even every frame):
        const std::vector<Vertex> vertices = {
            Vertex(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
            Vertex(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            Vertex(glm::vec3(0.5f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), glm::vec3(0.5f, 1.0f, 0.0f)),
        };
        vertex_buffer_default.update(vertices.data(), vertices.size());

        // In your render loop:
        vertex_buffer_default.draw(GL_TRIANGLES);
    }
} // namespace umfeld