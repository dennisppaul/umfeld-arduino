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

#include <cstring>
// ReSharper disable once CppUnusedIncludeDirective
#include <glm/glm.hpp>
#include <iostream>

#include "UmfeldSDLOpenGL.h"
#include "VertexBuffer.h"
#include "PGraphicsOpenGL.h"

using namespace umfeld;

#ifdef UMFELD_VERTEX_BUFFER_DEBUG_OPENGL_ERRORS
#define UMFELD_VERTEX_BUFFER_CHECK_ERROR(msg) \
    do {                                      \
        checkOpenGLError(msg);                \
    } while (0)
#else
#define UMFELD_VERTEX_BUFFER_CHECK_ERROR(msg)
#endif

VertexBuffer::VertexBuffer() : native_opengl_shape(get_draw_mode(TRIANGLES)) {}

// Move constructor
VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
    : _vertices(std::move(other._vertices)), vbo(other.vbo), vao(other.vao), vao_supported(other.vao_supported), initial_upload(other.initial_upload), buffer_initialized(other.buffer_initialized), server_buffer_size(other.server_buffer_size), dirty(other.dirty), native_opengl_shape(other.native_opengl_shape) {
    // Reset the moved-from object
    other.vbo                 = 0;
    other.vao                 = 0;
    other.buffer_initialized  = false;
    other.vao_supported       = false;
    other.dirty               = false;
    other.initial_upload      = false;
    other.server_buffer_size  = 0;
    other.native_opengl_shape = get_draw_mode(TRIANGLES);
}

// Move assignment operator
VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
    if (this != &other) {
        // Clean up existing resources
        if (vbo) {
            const GLuint _vbo = vbo;
            glDeleteBuffers(1, &_vbo);
        }
        if (vao_supported && vao) {
            const GLuint _vao = vao;
            glDeleteVertexArrays(1, &_vao);
        }

        // Move data from other
        _vertices           = std::move(other._vertices);
        vbo                 = other.vbo;
        vao                 = other.vao;
        buffer_initialized  = other.buffer_initialized;
        vao_supported       = other.vao_supported;
        dirty               = other.dirty;
        initial_upload      = other.initial_upload;
        server_buffer_size  = other.server_buffer_size;
        native_opengl_shape = other.native_opengl_shape;

        // Reset the moved-from object
        other.vbo                 = 0;
        other.vao                 = 0;
        other.buffer_initialized  = false;
        other.vao_supported       = false;
        other.dirty               = false;
        other.initial_upload      = false;
        other.server_buffer_size  = 0;
        other.native_opengl_shape = get_draw_mode(TRIANGLES);
    }
    return *this;
}

void VertexBuffer::init() {
    if (buffer_initialized) {
        return;
    }

    checkVAOSupport();
    GLuint _vbo;
    glGenBuffers(1, &_vbo);
    vbo = _vbo;
    if (vbo == 0) {
        error("Failed to generate VBO");
        return;
    }
    UMFELD_VERTEX_BUFFER_CHECK_ERROR("error generting VBO");

    if (vao_supported) {
        GLuint _vao;
        glGenVertexArrays(1, &_vao);
        vao = _vao;
        // Add error checking
        if (vao == 0) {
            error("Failed to generate VAO");
            const GLuint _vbo_ = vbo;
            glDeleteBuffers(1, &_vbo_);
            vbo = 0;
            return;
        }
        UMFELD_VERTEX_BUFFER_CHECK_ERROR("error generting VAO");
    }

    buffer_initialized = true;
}

VertexBuffer::~VertexBuffer() {
    try {
        // Check if we can make OpenGL calls
        const GLenum error             = glGetError();
        const bool   context_available = (error != GL_INVALID_OPERATION);

        if (context_available) {
            if (vbo) {
                const GLuint _vbo = vbo;
                glDeleteBuffers(1, &_vbo);
                // Clear any potential errors from deletion
                glGetError();
            }
            if (vao_supported && vao) {
                const GLuint _vao = vao;
                glDeleteVertexArrays(1, &_vao);
                // Clear any potential errors from deletion
                glGetError();
            }
        }
    } catch (...) {
        // Destructors should not throw, so catch any potential exceptions
        // This shouldn't happen with OpenGL calls, but better safe than sorry
    }

    // always reset the handles
    vbo = 0;
    vao = 0;
}

void VertexBuffer::checkVAOSupport() {
    int major = 0, minor = 0;
    get_OpenGL_version(major, minor);

    if ((major > 3) || (major == 3 && minor >= 3)) {
        vao_supported = true; // OpenGL 3.3+ supports VAOs
    } else {
        // NOTE for OpenGL 2.0, check if GL_ARB_vertex_array_object is available
        const auto extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        if (extensions && strstr(extensions, "GL_ARB_vertex_array_object")) {
            vao_supported = true;
        }
    }
}

void VertexBuffer::add_vertex(const Vertex& vertex) {
    dirty = true;
    _vertices.push_back(vertex);
}

void VertexBuffer::add_vertices(const std::vector<Vertex>& new_vertices) {
    dirty = true;
    _vertices.insert(_vertices.end(), new_vertices.begin(), new_vertices.end());
}

void VertexBuffer::clear() {
    dirty = true;
    _vertices.clear();
}

void VertexBuffer::set_shape(const int shape, const bool map_to_opengl_draw_mode) {
    this->native_opengl_shape = map_to_opengl_draw_mode ? get_draw_mode(shape) : shape;
}

void VertexBuffer::draw() {
    UMFELD_VERTEX_BUFFER_CHECK_ERROR("mesh / draw begin");

    if (!buffer_initialized) {
        init();
        if (!buffer_initialized) {
            return; // Check init success
        }
    }


    UMFELD_VERTEX_BUFFER_CHECK_ERROR("mesh / init");

    if (_vertices.empty()) { return; }

    if (dirty) {
        update();
        UMFELD_VERTEX_BUFFER_CHECK_ERROR("mesh / update");
    }

    const int mode = native_opengl_shape;

    if (vao_supported) {
        if (vao == 0) { return; }
        glBindVertexArray(vao);
        UMFELD_VERTEX_BUFFER_CHECK_ERROR("mesh / binding vao arrays");
        glDrawArrays(mode, 0, _vertices.size());
        UMFELD_VERTEX_BUFFER_CHECK_ERROR("mesh / draw arrays");
        glBindVertexArray(0);
    } else {
        if (vbo == 0) { return; }
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        enable_vertex_attributes();
        glDrawArrays(mode, 0, _vertices.size());
        disable_vertex_attributes();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void VertexBuffer::update() {
    if (!buffer_initialized) {
        init();
        if (!buffer_initialized) {
            return;
        }
    }

    dirty = false;

    if (_vertices.empty()) {
        return;
    }

    const size_t current_size   = _vertices.size();
    const size_t required_bytes = current_size * sizeof(Vertex);

    // Bind once for the entire operation
    if (vao_supported && vao != 0) {
        glBindVertexArray(vao);
    }

    if (vbo == 0) {
        error("Invalid VBO in update()");
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Handle initial upload or buffer resizing
    if (!initial_upload || needs_buffer_resize(current_size)) {
        upload_with_resize(current_size, required_bytes);
        if (!initial_upload) {
            initial_upload = true;
            enable_vertex_attributes();
        }
    } else {
        // Simple sub-data upload for same-sized or smaller buffers
        glBufferSubData(GL_ARRAY_BUFFER, 0, required_bytes, _vertices.data());
    }

    // Unbind once at the end
    if (vao_supported && vao != 0) {
        glBindVertexArray(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::upload_with_resize(const size_t current_size, const size_t required_bytes) {
    // NOTE assumes VBO is already bound
    if (current_size > server_buffer_size) {
        // grow buffer with chunking strategy
        const size_t grow_size_bytes = required_bytes + VBO_BUFFER_CHUNK_SIZE_BYTES;
        glBufferData(GL_ARRAY_BUFFER, grow_size_bytes, _vertices.data(), GL_DYNAMIC_DRAW);
        UMFELD_VERTEX_BUFFER_CHECK_ERROR("upload_with_resize / glBufferData grow"); // ← ADD THIS
#ifdef UMFELD_VERTEX_BUFFER_DEBUG_OPENGL_ERRORS
        console("upload_... / Growing vertex buffer from ", server_buffer_size * sizeof(Vertex), " to ", grow_size_bytes, " bytes");
#endif
        server_buffer_size = grow_size_bytes / sizeof(Vertex); // Convert to vertex count
    } else if (needs_buffer_shrink(current_size)) {
        // shrink buffer
        glBufferData(GL_ARRAY_BUFFER, required_bytes, _vertices.data(), GL_DYNAMIC_DRAW);
        UMFELD_VERTEX_BUFFER_CHECK_ERROR("upload_with_resize / glBufferData shrink"); // ← ADD THIS
#ifdef UMFELD_VERTEX_BUFFER_DEBUG_OPENGL_ERRORS
        console("upload_... / Shrinking vertex buffer from ", server_buffer_size * sizeof(Vertex), " to ", required_bytes, " bytes");
#endif
        server_buffer_size = current_size;
    } else {
        // same size or within acceptable range - just upload data
        glBufferSubData(GL_ARRAY_BUFFER, 0, required_bytes, _vertices.data());
        UMFELD_VERTEX_BUFFER_CHECK_ERROR("upload_with_resize / glBufferSubData"); // ← ADD THIS
    }
}

bool VertexBuffer::needs_buffer_resize(const size_t current_size) const {
    return current_size > server_buffer_size || needs_buffer_shrink(current_size);
}

bool VertexBuffer::needs_buffer_shrink(const size_t current_size) const {
    const size_t shrink_threshold = VBO_BUFFER_CHUNK_SIZE_BYTES / sizeof(Vertex);
    return current_size + shrink_threshold < server_buffer_size;
}

void VertexBuffer::enable_vertex_attributes() const {
    // Ensure buffer is bound before setting up attributes
    if (vbo == 0) {
        error("Attempting to set vertex attributes with invalid VBO");
        return;
    }

    glVertexAttribPointer(Vertex::ATTRIBUTE_LOCATION_POSITION, Vertex::ATTRIBUTE_SIZE_POSITION, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_POSITION);

    glVertexAttribPointer(Vertex::ATTRIBUTE_LOCATION_NORMAL, Vertex::ATTRIBUTE_SIZE_NORMAL, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_NORMAL);

    glVertexAttribPointer(Vertex::ATTRIBUTE_LOCATION_COLOR, Vertex::ATTRIBUTE_SIZE_COLOR, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
    glEnableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_COLOR);

    glVertexAttribPointer(Vertex::ATTRIBUTE_LOCATION_TEXCOORD, Vertex::ATTRIBUTE_SIZE_TEXCOORD, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tex_coord)));
    glEnableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_TEXCOORD);

    glVertexAttribPointer(Vertex::ATTRIBUTE_LOCATION_USERDATA, Vertex::ATTRIBUTE_SIZE_USERDATA, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, userdata)));
    glEnableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_USERDATA);
}

void VertexBuffer::disable_vertex_attributes() {
    glDisableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_POSITION);
    glDisableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_NORMAL);
    glDisableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_COLOR);
    glDisableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_TEXCOORD);
    glDisableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_USERDATA);
}

// Check if OpenGL context is valid
bool VertexBuffer::isContextValid() {
    // Check if we can make basic OpenGL calls
    GLenum error = glGetError();
    if (error == GL_INVALID_OPERATION) {
        return false; // No context or invalid context
    }

    // Try to get OpenGL version - this will fail if no context
    const GLubyte* version = glGetString(GL_VERSION);
    return version != nullptr;
}
