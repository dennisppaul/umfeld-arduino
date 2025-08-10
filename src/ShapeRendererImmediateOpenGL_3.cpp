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

#include <cfloat> // for FLT_MAX
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>
#include <iostream>
#include <map>

#include "UmfeldFunctionsAdditional.h"
#include "ShapeRendererBatchOpenGL_3.h"

namespace umfeld {
    void ShapeRendererBatchOpenGL_3::init() {
        initShaders();
        initBuffers();
    }

    void ShapeRendererBatchOpenGL_3::beginShape(ShapeMode mode, bool filled, bool transparent, uint32_t texture_id, const glm::mat4& model_transform_matrix) {
        if (shapeInProgress) {
            warning("beginShape() called while another shape is in progress");
        }

        currentShape             = Shape{};
        currentShape.mode        = mode;
        currentShape.filled      = filled;
        currentShape.transparent = transparent;
        currentShape.texture_id  = texture_id;
        currentShape.model       = model_transform_matrix;
        currentShape.vertices.clear(); // Ensure clean state
        shapeInProgress = true;
    }

    void ShapeRendererBatchOpenGL_3::setVertices(std::vector<Vertex>&& vertices) {
        if (!shapeInProgress) {
            error("Error: setVertices() called without beginShape()");
            return;
        }
        currentShape.vertices = std::move(vertices);
    }

    void ShapeRendererBatchOpenGL_3::setVertices(const std::vector<Vertex>& vertices) {
        if (!shapeInProgress) {
            error("Error: setVertices() called without beginShape()");
            return;
        }
        currentShape.vertices = vertices;
    }

    void ShapeRendererBatchOpenGL_3::vertex(const Vertex& v) {
        if (!shapeInProgress) {
            error("Error: vertex() called without beginShape()");
            return;
        }
        currentShape.vertices.push_back(v);
    }

    void ShapeRendererBatchOpenGL_3::endShape(bool closed) {
        (void) closed; // TODO ignored for now
        if (!shapeInProgress) {
            std::cerr << "Error: endShape() called without beginShape()" << std::endl;
            return;
        }
        if (currentShape.vertices.empty()) {
            std::cerr << "Warning: endShape() called with no vertices" << std::endl;
        }
        submitShape(currentShape);
        shapeInProgress = false;
    }

    void ShapeRendererBatchOpenGL_3::flush(const glm::mat4& view_projection_matrix) {
        if (shapes.empty()) {
            return;
        }

        // Use unordered_map for better performance with many textures
        std::unordered_map<GLuint, TextureBatch> textureBatches;
        textureBatches.reserve(8); // Reasonable default

        for (auto& s: shapes) {
            TextureBatch& batch = textureBatches[s.texture_id];
            batch.textureID     = s.texture_id;

            if (s.transparent) {
                batch.transparentShapes.push_back(&s);
            } else {
                batch.opaqueShapes.push_back(&s);
            }
        }

        // Compute depth and sort transparents
        for (auto& [textureID, batch]: textureBatches) {
            for (auto* s: batch.transparentShapes) {
                const glm::vec4 centerWS = s->model * glm::vec4(s->centerOS, 1.0f);
                const glm::vec4 centerVS = view_projection_matrix * centerWS;
                s->depth                 = centerVS.z / centerVS.w; // Proper NDC depth
            }
            std::sort(batch.transparentShapes.begin(), batch.transparentShapes.end(),
                      [](const Shape* A, const Shape* B) { return A->depth > B->depth; }); // Back to front
        }

        glBindVertexArray(vao);

        // Opaque pass
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        for (auto& [textureID, batch]: textureBatches) {
            renderBatch(batch.opaqueShapes, view_projection_matrix, textureID);
        }

        // Transparent pass
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        for (auto& [textureID, batch]: textureBatches) {
            renderBatch(batch.transparentShapes, view_projection_matrix, textureID);
        }

        glDepthMask(GL_TRUE);
        glBindVertexArray(0);
        shapes.clear();
    }

    void ShapeRendererBatchOpenGL_3::submitShape(Shape& s) {
        switch (shape_center_compute_strategy) {
            case AXIS_ALIGNED_BOUNDING_BOX: {
                if (s.vertices.empty()) {
                    s.centerOS = glm::vec3(0, 0, 0);
                    break;
                }
                glm::vec3 minP(FLT_MAX), maxP(-FLT_MAX);
                for (const auto& v: s.vertices) {
                    minP = glm::min(minP, glm::vec3(v.position));
                    maxP = glm::max(maxP, glm::vec3(v.position));
                }
                s.centerOS = (minP + maxP) * 0.5f;
                break;
            }
            case CENTER_OF_MASS: {
                if (s.vertices.empty()) {
                    s.centerOS = glm::vec3(0, 0, 0);
                    break;
                }
                glm::vec4 center(0, 0, 0, 1);
                for (const auto& v: s.vertices) { center += v.position; }
                center /= static_cast<float>(s.vertices.size());
                s.centerOS = center;
                break;
            }
            case ZERO_CENTER:
            default:
                s.centerOS = glm::vec3(0, 0, 0);
                break;
        }
        shapes.push_back(std::move(s)); // Move to avoid copy
    }

    GLuint ShapeRendererBatchOpenGL_3::compileShader(const char* src, const GLenum type) {
        const GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[512];
            glGetShaderInfoLog(s, 512, nullptr, buf);
            std::cerr << "Shader compile error: " << buf << std::endl;
        }
        return s;
    }

    GLuint ShapeRendererBatchOpenGL_3::createShaderProgram(const char* vsSrc, const char* fsSrc) {
        const GLuint vs      = compileShader(vsSrc, GL_VERTEX_SHADER);
        const GLuint fs      = compileShader(fsSrc, GL_FRAGMENT_SHADER);
        const GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        GLint ok;
        glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok) {
            char buf[512];
            glGetProgramInfoLog(program, 512, nullptr, buf);
            std::cerr << "Link error: " << buf << std::endl;
        }
        glDeleteShader(vs); // Clean up shader objects
        glDeleteShader(fs);
        return program;
    }

    void ShapeRendererBatchOpenGL_3::setupUniformBlocks(const GLuint program) {
        const GLuint blockIndex = glGetUniformBlockIndex(program, "Transforms");
        glUniformBlockBinding(program, blockIndex, 0);
    }

    void ShapeRendererBatchOpenGL_3::initShaders() {

        // NOTE for OpenGL ES 3.0 change from:
        //      ```glsl
        //      #version 330 core
        //      ```
        //      to:
        //      ```glsl
        //      #version 300 es
        //      precision mediump float;
        //      precision mediump int;
        //      precision mediump sampler2D;
        //      ```
        //      and create shader source with dynamic array size
        //      ```c
        //      std::string transformsDefine = "#define MAX_TRANSFORMS " + std::to_string(MAX_TRANSFORMS) + "\n";
        //      const auto texturedVS = transformsDefine + R"(#version 330 core
        //      ```

        const auto texturedVS = R"(#version 330 core
layout(location=0) in vec4 aPosition;
layout(location=1) in vec4 aNormal;
layout(location=2) in vec4 aColor;
layout(location=3) in vec3 aTexCoord;
layout(location=4) in uint aTransformID;
layout(std140) uniform Transforms {
    mat4 uModel[256];
};
uniform mat4 uViewProj;
out vec2 vTexCoord;
out vec4 vColor;
void main() {
    mat4 M = uModel[aTransformID];
    gl_Position = uViewProj * M * aPosition;
    vTexCoord = aTexCoord.xy;
    vColor = aColor;
}
)";

        const auto texturedFS = R"(#version 330 core
in vec2 vTexCoord;
in vec4 vColor;
out vec4 fragColor;
uniform sampler2D uTex;
void main() {
    fragColor = texture(uTex, vTexCoord) * vColor;
}
)";

        const auto untexturedVS = R"(#version 330 core
layout(location=0) in vec4 aPosition;
layout(location=1) in vec4 aNormal;
layout(location=2) in vec4 aColor;
layout(location=3) in vec3 aTexCoord;
layout(location=4) in uint aTransformID;
layout(std140) uniform Transforms {
    mat4 uModel[256];
};
uniform mat4 uViewProj;
out vec4 vColor;
void main() {
    mat4 M = uModel[aTransformID];
    gl_Position = uViewProj * M * aPosition;
    vColor = aColor;
}
)";

        const auto untexturedFS = R"(#version 330 core
in vec4 vColor;
out vec4 fragColor;
void main() {
    fragColor = vColor;
}
)";

        texturedShaderProgram   = createShaderProgram(texturedVS, texturedFS);
        untexturedShaderProgram = createShaderProgram(untexturedVS, untexturedFS);

        setupUniformBlocks(texturedShaderProgram);
        setupUniformBlocks(untexturedShaderProgram);

        // Cache uniform locations
        texturedUniforms.uViewProj   = glGetUniformLocation(texturedShaderProgram, "uViewProj");
        texturedUniforms.uTex        = glGetUniformLocation(texturedShaderProgram, "uTex");
        untexturedUniforms.uViewProj = glGetUniformLocation(untexturedShaderProgram, "uViewProj");
    }

    void ShapeRendererBatchOpenGL_3::initBuffers() {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glEnableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_POSITION);
        glVertexAttribPointer(Vertex::ATTRIBUTE_LOCATION_POSITION, Vertex::ATTRIBUTE_SIZE_POSITION, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
        glEnableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_NORMAL);
        glVertexAttribPointer(Vertex::ATTRIBUTE_LOCATION_NORMAL, Vertex::ATTRIBUTE_SIZE_NORMAL, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
        glEnableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_COLOR);
        glVertexAttribPointer(Vertex::ATTRIBUTE_LOCATION_COLOR, Vertex::ATTRIBUTE_SIZE_COLOR, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
        glEnableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_TEXCOORD);
        glVertexAttribPointer(Vertex::ATTRIBUTE_LOCATION_TEXCOORD, Vertex::ATTRIBUTE_SIZE_TEXCOORD, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tex_coord)));
        glEnableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_TRANSFORM_ID);
        glVertexAttribIPointer(Vertex::ATTRIBUTE_LOCATION_TRANSFORM_ID, Vertex::ATTRIBUTE_SIZE_TRANSFORM_ID, GL_UNSIGNED_SHORT, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, transform_id)));
        glEnableVertexAttribArray(Vertex::ATTRIBUTE_LOCATION_USERDATA);
        glVertexAttribIPointer(Vertex::ATTRIBUTE_LOCATION_USERDATA, Vertex::ATTRIBUTE_SIZE_USERDATA, GL_UNSIGNED_SHORT, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, userdata)));

        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, MAX_TRANSFORMS * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

        glBindVertexArray(0);

        // Pre-allocate frame buffers
        frameVertices.reserve(4096); // NOTE maybe decrease this for mobile to 1024
        frameMatrices.reserve(MAX_TRANSFORMS);
    }

    size_t ShapeRendererBatchOpenGL_3::estimateTriangleCount(const Shape& s) {
        const size_t n = s.vertices.size();
        if (n < 3 || !s.filled) {
            return 0;
        }

        switch (s.mode) {
            case TRIANGLES: return (n / 3) * 3;
            case TRIANGLE_STRIP:
            case TRIANGLE_FAN:
            case POLYGON: return (n - 2) * 3;
            case QUADS: return (n / 4) * 6;
            case QUAD_STRIP: return n >= 4 ? ((n / 2) - 1) * 6 : 0;
            default: return 0;
        }
    }

    void ShapeRendererBatchOpenGL_3::tessellateToTriangles(const Shape& s, std::vector<Vertex>& out, const uint16_t transformID) {
        const auto&  v = s.vertices;
        const size_t n = v.size();
        if (n < 3 || !s.filled) {
            return;
        }

        auto pushTri = [&](size_t i0, size_t i1, size_t i2) {
            Vertex a       = v[i0];
            a.transform_id = transformID;
            out.push_back(a);
            Vertex b       = v[i1];
            b.transform_id = transformID;
            out.push_back(b);
            Vertex c       = v[i2];
            c.transform_id = transformID;
            out.push_back(c);
        };

        switch (s.mode) {
            case TRIANGLES: {
                const size_t m = n / 3 * 3;
                for (size_t i = 0; i < m; ++i) {
                    Vertex vv       = v[i];
                    vv.transform_id = transformID;
                    out.push_back(vv);
                }
                break;
            }
            case TRIANGLE_STRIP: {
                for (size_t k = 2; k < n; ++k) {
                    if ((k & 1) == 0) {
                        pushTri(k - 2, k - 1, k);
                    } else {
                        pushTri(k - 1, k - 2, k);
                    }
                }
                break;
            }
            case TRIANGLE_FAN:
            case POLYGON: {
                for (size_t i = 2; i < n; ++i) {
                    pushTri(0, i - 1, i);
                }
                break;
            }
            case QUADS: {
                const size_t q = (n / 4) * 4;
                for (size_t i = 0; i + 3 < q; i += 4) {
                    pushTri(i + 0, i + 1, i + 2);
                    pushTri(i + 0, i + 2, i + 3);
                }
                break;
            }
            case QUAD_STRIP: {
                for (size_t i = 0; i + 3 < n; i += 2) {
                    pushTri(i + 0, i + 1, i + 3);
                    pushTri(i + 0, i + 3, i + 2);
                }
                break;
            }
            default: break;
        }
    }

    void ShapeRendererBatchOpenGL_3::renderBatch(const std::vector<Shape*>& shapesToRender, const glm::mat4& viewProj, const GLuint textureID) {
        if (shapesToRender.empty()) {
            return;
        }

        const GLuint          shader   = (textureID == TEXTURE_NONE) ? untexturedShaderProgram : texturedShaderProgram;
        const ShaderUniforms& uniforms = (textureID == TEXTURE_NONE) ? untexturedUniforms : texturedUniforms;

        glUseProgram(shader);
        glUniformMatrix4fv(uniforms.uViewProj, 1, GL_FALSE, &viewProj[0][0]);

        if (textureID != TEXTURE_NONE) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(uniforms.uTex, 0);
        }

        // Process in chunks to respect MAX_TRANSFORMS limit
        for (size_t offset = 0; offset < shapesToRender.size(); offset += MAX_TRANSFORMS) {
            const size_t chunkSize = std::min(static_cast<size_t>(MAX_TRANSFORMS), shapesToRender.size() - offset);

            // Upload transforms for this chunk
            frameMatrices.clear();
            frameMatrices.reserve(chunkSize);
            for (size_t i = 0; i < chunkSize; ++i) {
                frameMatrices.push_back(shapesToRender[offset + i]->model);
            }

            glBindBuffer(GL_UNIFORM_BUFFER, ubo);
            glBufferSubData(GL_UNIFORM_BUFFER, 0,
                            static_cast<GLsizeiptr>(frameMatrices.size() * sizeof(glm::mat4)),
                            frameMatrices.data());

            // Estimate and reserve vertex space
            frameVertices.clear();
            size_t totalEstimate = 0;
            for (size_t i = 0; i < chunkSize; ++i) {
                totalEstimate += estimateTriangleCount(*shapesToRender[offset + i]);
            }
            frameVertices.reserve(totalEstimate);

            // Tessellate shapes in this chunk
            for (size_t i = 0; i < chunkSize; ++i) {
                const auto* s = shapesToRender[offset + i];
                tessellateToTriangles(*s, frameVertices, static_cast<uint16_t>(i));
            }

            if (!frameVertices.empty()) {
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER,
                             static_cast<GLsizeiptr>(frameVertices.size() * sizeof(Vertex)),
                             frameVertices.data(), GL_DYNAMIC_DRAW);
                glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(frameVertices.size()));
            }
        }
    }

} // namespace umfeld