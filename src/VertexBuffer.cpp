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

// ReSharper disable once CppUnusedIncludeDirective
#include <glm/glm.hpp>
#include <iostream>

#include "VertexBuffer.h"
#include "PGraphicsOpenGL.h"

using namespace umfeld;

void VertexBuffer::init() {
    if (buffer_initialized) {
        return;
    }
    checkVAOSupport();
    glGenBuffers(1, &vbo);
    if (vao_supported) {
        glGenVertexArrays(1, &vao);
    }
    buffer_initialized = true;
}

VertexBuffer::~VertexBuffer() {
    if (vbo) {
        glDeleteBuffers(1, &vbo);
    }
    if (vao_supported && vao) {
        glDeleteVertexArrays(1, &vao);
    }
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

void VertexBuffer::resize_buffer() {
    const size_t client_buffer_size_bytes = _vertices.size() * sizeof(Vertex);
    const size_t server_buffer_size_bytes = server_buffer_size * sizeof(Vertex);

    if (client_buffer_size_bytes > server_buffer_size_bytes) {
        const size_t growSize = client_buffer_size_bytes + VBO_BUFFER_CHUNK_SIZE_BYTES;
        // std::cout << "Growing vertex buffer from " << server_buffer_size_bytes << " to " << growSize << " bytes" << std::endl;
        glBindBuffer(GL_ARRAY_BUFFER, vbo); // Ensure VBO is bound
        glBufferData(GL_ARRAY_BUFFER, growSize, _vertices.data(), GL_DYNAMIC_DRAW);
        server_buffer_size = growSize / sizeof(Vertex);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    if (client_buffer_size_bytes < server_buffer_size_bytes) {
        const size_t shrinkSize = client_buffer_size_bytes;
        // std::cout << "Shrinking vertex buffer to " << client_buffer_size_bytes << " bytes" << std::endl;
        glBindBuffer(GL_ARRAY_BUFFER, vbo); // Ensure VBO is bound
        glBufferData(GL_ARRAY_BUFFER, shrinkSize, _vertices.data(), GL_DYNAMIC_DRAW);
        server_buffer_size = shrinkSize / sizeof(Vertex);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void VertexBuffer::draw() {
    if (!buffer_initialized) { init(); }

    if (_vertices.empty()) {
        return;
    }

    if (dirty) {
        dirty = false;
        update();
    }
    const int mode = get_draw_mode(shape);

    if (vao_supported) {
        glBindVertexArray(vao);
        glDrawArrays(mode, 0, _vertices.size());
        glBindVertexArray(0);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        enable_vertex_attributes();

        glDrawArrays(mode, 0, _vertices.size());

        disable_vertex_attributes();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void VertexBuffer::upload() {
    if (_vertices.empty()) {
        return;
    }

    if (vao_supported) {
        glBindVertexArray(vao);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    const size_t current_size   = _vertices.size();
    const size_t required_bytes = current_size * sizeof(Vertex);

    if (current_size > server_buffer_size) {
        // NOTE buffer needs to grow - reallocate entire buffer
        glBufferData(GL_ARRAY_BUFFER, required_bytes, _vertices.data(), GL_DYNAMIC_DRAW);
        server_buffer_size = current_size;
    } else {
        // NOTE buffer is same size or smaller - use sub data upload
        glBufferSubData(GL_ARRAY_BUFFER, 0, required_bytes, _vertices.data());
    }

    enable_vertex_attributes();

    if (vao_supported) {
        glBindVertexArray(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::update() {
    if (!buffer_initialized) {
        init();
    }

    if (_vertices.empty()) {
        return;
    }

    const size_t current_size   = _vertices.size();
    const size_t required_bytes = current_size * sizeof(Vertex);

    // Bind once for the entire operation
    if (vao_supported) {
        glBindVertexArray(vao);
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
    if (vao_supported) {
        glBindVertexArray(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::upload_with_resize(const size_t current_size, const size_t required_bytes) {
    if (current_size > server_buffer_size) {
        // Grow buffer with chunking strategy
        const size_t grow_size = required_bytes + VBO_BUFFER_CHUNK_SIZE_BYTES;
        glBufferData(GL_ARRAY_BUFFER, grow_size, _vertices.data(), GL_DYNAMIC_DRAW);
        server_buffer_size = grow_size / sizeof(Vertex);
    } else if (needs_buffer_shrink(current_size)) {
        // Shrink buffer
        glBufferData(GL_ARRAY_BUFFER, required_bytes, _vertices.data(), GL_DYNAMIC_DRAW);
        server_buffer_size = current_size;
    } else {
        // Same size or within acceptable range - just upload data
        glBufferSubData(GL_ARRAY_BUFFER, 0, required_bytes, _vertices.data());
    }
}

bool VertexBuffer::needs_buffer_resize(const size_t current_size) const {
    return current_size > server_buffer_size || needs_buffer_shrink(current_size);
}

bool VertexBuffer::needs_buffer_shrink(const size_t current_size) const {
    const size_t shrink_threshold = VBO_BUFFER_CHUNK_SIZE_BYTES / sizeof(Vertex);
    return current_size + shrink_threshold < server_buffer_size;
}

void VertexBuffer::enable_vertex_attributes() {
    glVertexAttribPointer(ATTRIBUTE_LOCATION_POSITION, ATTRIBUTE_SIZE_POSITION, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(ATTRIBUTE_LOCATION_POSITION);
    glVertexAttribPointer(ATTRIBUTE_LOCATION_NORMAL, ATTRIBUTE_SIZE_NORMAL, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(ATTRIBUTE_LOCATION_NORMAL);
    glVertexAttribPointer(ATTRIBUTE_LOCATION_COLOR, ATTRIBUTE_SIZE_COLOR, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
    glEnableVertexAttribArray(ATTRIBUTE_LOCATION_COLOR);
    glVertexAttribPointer(ATTRIBUTE_LOCATION_TEXCOORD, ATTRIBUTE_SIZE_TEXCOORD, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tex_coord)));
    glEnableVertexAttribArray(ATTRIBUTE_LOCATION_TEXCOORD);
    glVertexAttribPointer(ATTRIBUTE_LOCATION_USERDATA, ATTRIBUTE_SIZE_USERDATA, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, userdata)));
    glEnableVertexAttribArray(ATTRIBUTE_LOCATION_USERDATA);
}

void VertexBuffer::disable_vertex_attributes() {
    glDisableVertexAttribArray(ATTRIBUTE_LOCATION_POSITION);
    glDisableVertexAttribArray(ATTRIBUTE_LOCATION_NORMAL);
    glDisableVertexAttribArray(ATTRIBUTE_LOCATION_COLOR);
    glDisableVertexAttribArray(ATTRIBUTE_LOCATION_TEXCOORD);
    glDisableVertexAttribArray(ATTRIBUTE_LOCATION_USERDATA);
}