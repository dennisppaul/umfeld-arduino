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

#include "UmfeldConstants.h"
#include "UmfeldFunctionsAdditional.h"
#include "ShapeRendererOpenGL_3.h"

namespace umfeld {
    void ShapeRendererOpenGL_3::init(PGraphics* g, const std::vector<int> shader_programs) {
        graphics = g;
        initShaders(shader_programs);
        initBuffers();
    }

    void ShapeRendererOpenGL_3::beginShape(const ShapeMode  mode,
                                           const bool       filled,
                                           const bool       transparent,
                                           const uint32_t   texture_id,
                                           const glm::mat4& model_transform_matrix) {
        if (shapeInProgress) {
            warning("beginShape() called while another shape is in progress");
        }

        currentShape             = Shape{};
        currentShape.mode        = mode;
        currentShape.filled      = filled;
        currentShape.transparent = transparent;
        currentShape.texture_id  = texture_id;
        currentShape.model       = model_transform_matrix;
        currentShape.vertices.clear();
        shapeInProgress = true;
    }

    void ShapeRendererOpenGL_3::setVertices(std::vector<Vertex>&& vertices) {
        if (!shapeInProgress) {
            error("Error: setVertices() called without beginShape()");
            return;
        }
        currentShape.vertices = std::move(vertices);
    }

    void ShapeRendererOpenGL_3::setVertices(const std::vector<Vertex>& vertices) {
        if (!shapeInProgress) {
            error("Error: setVertices() called without beginShape()");
            return;
        }
        currentShape.vertices = vertices;
    }

    void ShapeRendererOpenGL_3::vertex(const Vertex& v) {
        if (!shapeInProgress) {
            error("Error: vertex() called without beginShape()");
            return;
        }
        currentShape.vertices.push_back(v);
    }

    void ShapeRendererOpenGL_3::endShape(const bool closed) {
        (void) closed; // TODO ignored for now
        if (!shapeInProgress) {
            std::cerr << "Error: endShape() called without beginShape()" << std::endl;
            return;
        }
        if (currentShape.vertices.empty()) {
            std::cerr << "Warning: endShape() called with no vertices" << std::endl;
        }
        currentShape.closed = closed;
        submitShape(currentShape);
        shapeInProgress = false;
    }

    void ShapeRendererOpenGL_3::flush(const glm::mat4& view_projection_matrix) {
        if (graphics != nullptr) {
            if (graphics->get_render_mode() == RENDER_MODE_SORTED_BY_Z_ORDER) {
                console_once("render_mode: rendering in z-order ( + batches + depth sorted )");
                flush_sort_by_z_order(view_projection_matrix);
            } else if (graphics->get_render_mode() == RENDER_MODE_SORTED_BY_SUBMISSION_ORDER) {
                console_once("render_mode: rendering in submission order");
                flush_submission_order(view_projection_matrix);
            } else if (graphics->get_render_mode() == RENDER_MODE_IMMEDIATELY) {
                console_once("rendering shapes immediately");
                flush_immediately(view_projection_matrix);
            }
        }
    }

    void ShapeRendererOpenGL_3::flush_immediately(const glm::mat4& view_projection_matrix) {
        flush_sort_by_z_order(view_projection_matrix);
    }

    void ShapeRendererOpenGL_3::flush_sort_by_z_order(const glm::mat4& view_projection_matrix) {
        if (shapes.empty()) {
            return;
        }

        // use unordered_map for better performance with many textures
        std::unordered_map<GLuint, TextureBatch> textureBatches;
        textureBatches.reserve(8); // Reasonable default

        for (auto& s: shapes) {
            TextureBatch& batch = textureBatches[s.texture_id];
            batch.textureID     = s.texture_id;

            if (s.transparent) {
                batch.transparent_shapes.push_back(&s);
            } else {
                batch.opaque_shapes.push_back(&s);
            }
        }

        // Compute depth and sort transparents
        for (auto& [textureID, batch]: textureBatches) {
            for (auto* s: batch.transparent_shapes) {
                const glm::vec4 center_world_space = s->model * glm::vec4(s->center_object_space, 1.0f);
                const glm::vec4 center_view_space  = view_projection_matrix * center_world_space;
                s->depth                           = center_view_space.z / center_view_space.w; // Proper NDC depth
            }
            std::sort(batch.transparent_shapes.begin(), batch.transparent_shapes.end(),
                      [](const Shape* A, const Shape* B) { return A->depth > B->depth; }); // Back to front
        }

        glBindVertexArray(vao);

        // Opaque pass
        if (graphics != nullptr) {
            if (graphics->hint_enable_depth_test) {
                glEnable(GL_DEPTH_TEST);
            } else {
                glDisable(GL_DEPTH_TEST);
            }
        } else {
            glEnable(GL_DEPTH_TEST);
        }
        glDepthFunc(GL_LEQUAL); // Allow equal depths to pass ( `GL_LESS` is default )
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        for (auto& [textureID, batch]: textureBatches) {
            renderBatch(batch.opaque_shapes, view_projection_matrix, textureID);
        }

        // Transparent pass
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        for (auto& [textureID, batch]: textureBatches) {
            renderBatch(batch.transparent_shapes, view_projection_matrix, textureID);
        }

        glDepthMask(GL_TRUE);
        glBindVertexArray(0);
        shapes.clear();
    }

    void ShapeRendererOpenGL_3::flush_submission_order(const glm::mat4& view_projection_matrix) {
        if (shapes.empty()) {
            return;
        }

        glBindVertexArray(vao);

        // Enable depth test for all shapes
        if (graphics != nullptr) {
            if (graphics->hint_enable_depth_test) {
                glEnable(GL_DEPTH_TEST);
            } else {
                glDisable(GL_DEPTH_TEST);
            }
        } else {
            glEnable(GL_DEPTH_TEST);
        }
        glDepthFunc(GL_LEQUAL);

        GLuint currentTexture = UINT32_MAX; // Force initial texture bind
        bool   blendEnabled   = false;

        // Render each shape individually in submission order
        for (auto& shape: shapes) {
            // Handle transparency state changes
            if (shape.transparent && !blendEnabled) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE); // Disable depth writes for transparency
                blendEnabled = true;
            } else if (!shape.transparent && blendEnabled) {
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE); // Enable depth writes for opaque
                blendEnabled = false;
            }

            // Handle texture changes
            if (shape.texture_id != currentTexture) {
                currentTexture = shape.texture_id;
                if (currentTexture == TEXTURE_NONE) {
                    // Switch to untextured shader
                    glUseProgram(untexturedShaderProgram);
                    glUniformMatrix4fv(untexturedUniforms.uViewProj, 1, GL_FALSE, &view_projection_matrix[0][0]);
                } else {
                    // Switch to textured shader
                    glUseProgram(texturedShaderProgram);
                    glUniformMatrix4fv(texturedUniforms.uViewProj, 1, GL_FALSE, &view_projection_matrix[0][0]);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, currentTexture);
                    glUniform1i(texturedUniforms.uTex, 0);
                }
            }

            // Render single shape (create a vector with just this shape)
            std::vector<Shape*> singleShape = {&shape};
            renderBatch(singleShape, view_projection_matrix, shape.texture_id);
        }

        // Restore default state
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glBindVertexArray(0);
        shapes.clear();
    }

    void ShapeRendererOpenGL_3::submitShape(Shape& s) {
        if (s.transparent) { // NOTE only copmute center for transparent shapes
            computeShapeCenter(s);
        }

        if (graphics != nullptr) {
            /* convert stroke shape to line strips + triangulate stroke */
            if (!s.filled) {
                std::vector<Shape> converted_shapes;
                PGraphics::convert_stroke_shape_to_line_strip(s, converted_shapes);

                if (!converted_shapes.empty()) {
                    for (auto& cs: converted_shapes) {
                        std::vector<Vertex> triangulated_vertices;
                        graphics->triangulate_line_strip_vertex(cs.vertices,
                                                                cs.closed,
                                                                triangulated_vertices);
                        cs.vertices = std::move(triangulated_vertices);
                        cs.filled   = true;
                        cs.mode     = TRIANGLES;
                        shapes.push_back(std::move(cs));
                    }
                }
            }
            /* convert filled shapes to triangles */
            if (s.filled) {
                graphics->convert_fill_shape_to_triangles(s);
                s.mode = TRIANGLES;
                shapes.push_back(std::move(s));
            }
        }
    }

    void ShapeRendererOpenGL_3::computeShapeCenter(Shape& s) const {
        if (s.vertices.empty()) {
            s.center_object_space = glm::vec3(0, 0, 0);
            return;
        }

        switch (shape_center_compute_strategy) {
            case AXIS_ALIGNED_BOUNDING_BOX: {
                glm::vec3 minP(FLT_MAX), maxP(-FLT_MAX);
                for (const auto& v: s.vertices) {
                    minP = glm::min(minP, glm::vec3(v.position));
                    maxP = glm::max(maxP, glm::vec3(v.position));
                }
                s.center_object_space = (minP + maxP) * 0.5f;
                break;
            }
            case CENTER_OF_MASS: {
                glm::vec4 center(0, 0, 0, 1);
                for (const auto& v: s.vertices) {
                    center += v.position;
                }
                center /= static_cast<float>(s.vertices.size());
                s.center_object_space = center;
                break;
            }
            case ZERO_CENTER:
            default:
                s.center_object_space = glm::vec3(0, 0, 0);
                break;
        }
    }

    void ShapeRendererOpenGL_3::setupUniformBlocks(const GLuint program) {
        const GLuint blockIndex = glGetUniformBlockIndex(program, "Transforms");
        glUniformBlockBinding(program, blockIndex, 0);
    }

    void ShapeRendererOpenGL_3::initShaders(const std::vector<int>& shader_programm_id) {
        // NOTE for OpenGL ES 3.0 create shader source with dynamic array size
        //      ```c
        //      std::string transformsDefine = "#define MAX_TRANSFORMS " + std::to_string(MAX_TRANSFORMS) + "\n";
        //      const auto texturedVS = transformsDefine + R"(#version 330 core
        //      ```

        texturedShaderProgram   = shader_programm_id[SHADER_PROGRAM_TEXTURED];
        untexturedShaderProgram = shader_programm_id[SHADER_PROGRAM_UNTEXTURED];
        // TODO implement
        //      untexturedLightingShaderProgram = shader_programm_id[SHADER_PROGRAM_UNTEXTURED_LIGHT];
        //      texturedLightingShaderProgram   = shader_programm_id[SHADER_PROGRAM_TEXTURED_LIGHT];
        //      pointShaderProgram              = shader_programm_id[SHADER_PROGRAM_POINT];
        //      lineShaderProgram               = shader_programm_id[SHADER_PROGRAM_LINE];

        setupUniformBlocks(texturedShaderProgram);
        setupUniformBlocks(untexturedShaderProgram);

        // Cache uniform locations
        texturedUniforms.uViewProj   = glGetUniformLocation(texturedShaderProgram, "uViewProj");
        texturedUniforms.uTex        = glGetUniformLocation(texturedShaderProgram, "uTex");
        untexturedUniforms.uViewProj = glGetUniformLocation(untexturedShaderProgram, "uViewProj");
    }

    void ShapeRendererOpenGL_3::initBuffers() {
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

    size_t ShapeRendererOpenGL_3::estimateTriangleCount(const Shape& s) {
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

    void ShapeRendererOpenGL_3::tessellateToTriangles(const Shape& s, std::vector<Vertex>& out, const uint16_t transformID) {
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

    void ShapeRendererOpenGL_3::renderBatch(const std::vector<Shape*>& shapesToRender, const glm::mat4& viewProj, const GLuint textureID) {
        if (shapesToRender.empty()) {
            return;
        }

        const GLuint          shader   = enable_lighting ? ((textureID == TEXTURE_NONE) ? untexturedLightingShaderProgram : texturedLightingShaderProgram) : ((textureID == TEXTURE_NONE) ? untexturedShaderProgram : texturedShaderProgram);
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
            // TODO tesselation is not necessary if shape is already TRIANGLES
            for (size_t i = 0; i < chunkSize; ++i) {
                const auto* s = shapesToRender[offset + i];
                tessellateToTriangles(*s, frameVertices, static_cast<uint16_t>(i));
            }
            // for (size_t i = chunkSize; i-- > 0;) {
            //     const auto* s = shapesToRender[offset + i];
            //     tessellateToTriangles(*s, frameVertices, static_cast<uint16_t>(i));
            // }

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