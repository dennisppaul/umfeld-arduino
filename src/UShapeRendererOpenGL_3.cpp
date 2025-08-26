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

#include <cfloat>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>
#include <iostream>
#include <map>

#include "UmfeldConstants.h"
#include "UmfeldFunctionsAdditional.h"
#include "PGraphicsOpenGL.h"
#include "UShapeRendererOpenGL_3.h"
#include "Geometry.h"
#include "VertexBuffer.h"

#ifndef USHAPE_RENDERER_OPENGL_3_DO_NOT_CHECK_ERRORS
#define GL_CALL(func) \
    func;             \
    PGraphicsOpenGL::OGL_check_error(#func)
#else
#define GL_CALL(func) func;
#endif

namespace umfeld {
    void UShapeRendererOpenGL_3::init(PGraphics* g, const std::vector<PShader*> shader_programs) {
        graphics                = g;
        default_shader_programs = shader_programs;
        init_shaders(shader_programs);
        init_buffers();
    }

    void UShapeRendererOpenGL_3::submit_shape(UShape& s) {
        // NOTE only compute center for transparent shapes
        if (s.light_enabled) {
            frame_light_shapes_count++;
        } else {
            if (s.transparent) {
                computeShapeCenter(s);
                frame_transparent_shapes_count++;
            } else {
                frame_opaque_shapes_count++;
            }
        }
        if (s.texture_id != TEXTURE_NONE) {
            frame_textured_shapes_count++;
        }
        shapes.push_back(std::move(s));
    }

    void UShapeRendererOpenGL_3::flush(const glm::mat4& view_matrix, const glm::mat4& projection_matrix) {
        if (shapes.empty() || graphics == nullptr) {
            prepare_next_flush_frame();
            return;
        }

        // NOTE render mode paths
        //      ├── 1. z_order
        //      │   ├── 1.1 process_shapes_z_order
        //      │   │   ├── processed_point_shapes
        //      │   │   ├── processed_line_shapes
        //      │   │   └── processed_shapes
        //      │   └── 1.2 flush_sort_by_z_order ( TODO what about custom shader and custom vertex buffer shapes ... `render_shape()` handles them already but what about `render_batch()`? )
        //      │       ├── separate shapes into opaque and transparent shapes
        //      │       ├── sort opaque shapes into flat, light and custom shape bins
        //      │       ├── ggf resize default vertex buffer ( depending on batch size ) ( TODO what about transparent shapes? )
        //      │       ├── compute depth and sort transparent shapes
        //      │       ├── set per frame shader uniforms ( OPTIMIZE this can be handle more efficent e.g with caching states )
        //      │       ├── draw opaque pass ( with `render_batch()` )
        //      │       │   ├── disable transpareny
        //      │       │   ├── use shader + ggf bind texture
        //      │       │   └── draw with `render_batch()`
        //      │       ├── draw (native) point pass ( with `render_shape()` )
        //      │       ├── draw (native) line pass ( with `render_shape()` )
        //      │       ├── draw light (opaque) pass ( with `render_batch()` )
        //      │       │   ├── disable transpareny
        //      │       │   ├── use shader + ggf bind texture
        //      │       │   └── draw with `render_batch()`
        //      │       └── draw transparent pass ( with `render_shape()` ) ( OPTIMIZE might be improved with dedicates `render_shape()` )
        //      ├── 2. submission_order
        //      │   ├── 2.1 process_shapes_submission_order
        //      │   │   ├── stroke shapes
        //      │   │   │   ├── point ( depending on point render mode, converts points to triangles or point primitive )
        //      │   │   │   └── line ( depending on line render mode, converts lines to triangles or native line primitives LINES, LINE_STRIP or LINE_LOOP )
        //      │   │   └── filled shapes ( converts all filled shapes to TRIANGLES ) ( OPTIMIZE include primitves TRIANGLE_FAN and TRIANGLE_STRIP )
        //      │   └── 2.2 flush_submission_order
        //      │       └── render_shape
        //      │           ├── evaluate shape mode ( TODO what about shapes with custom vertex buffers? )
        //      │           ├── handle transparency
        //      │           ├── handle shader program ( incl custom shader )
        //      │           │   ├── default shader
        //      │           │   │   └── ggf set/update model matrix uniforms
        //      │           │   └── custom shader
        //      │           │       └── ggf set/update model matrix uniforms
        //      │           ├── handle lighting
        //      │           │   └── ggf set/update light uniforms
        //      │           ├── handle texture ( NOTE use caching to minimze API calls )
        //      │           ├── handle vertex buffer
        //      │           │   ├── default vertex buffer
        //      │           │   │   └── ggf set/update model matrix uniforms
        //      │           │   └── custom vertex buffer
        //      │           │       ├── unbind default vertex buffer
        //      │           │       ├── draw custom vertex buffer
        //      │           │       └── (re)bind default vertex buffer
        //      │           └── draw vertex buffer ( default and custom )
        //      └── 3. immediately ( same as 'submission order' only with single shape )

        frame_state_cache.reset();

        // if (!processed_point_shapes.empty() || !processed_line_shapes.empty()) {
        //     // TODO alternative render paths for points and lines are currently not implemented
        //     warning_in_function_once("TODO alternative render paths for points and lines are currently not implemented");
        // }
        // NOTE the paths below ONLY render filled triangle shapes.
        if (graphics->get_render_mode() == RENDER_MODE_SORTED_BY_Z_ORDER) {

            // NOTE Z-ORDER RENDER MODE PATH
            //      ├── OPAQUE
            //      │   ├── ( BATCH_ ) textured+flat shapes
            //      │   ├── ( BATCH_ ) light shapes
            //      │   ├── ( SINGLE ) point shapes
            //      │   ├── ( SINGLE ) line shapes
            //      │   └── ( SINGLE ) custom shapes ( shader or vertex buffer )
            //      └─── TRANSPARENT
            //           ├── ( SINGLE ) all shapes ( textured+flat+light+custom ) sorted by z-order
            //           └── ( BATCH_ ) OR textured+flat+light shapes ( if no other transparent custom shapes are present )

            console_once(format_label("render_mode"), "RENDER_MODE_SORTED_BY_Z_ORDER ( rendering shapes in z-order and in batches )");
            // TRACE_SCOPE_N("RENDER_MODE_SORTED_BY_Z_ORDER");
            {
                std::vector<UShape> processed_point_shapes;
                std::vector<UShape> processed_line_shapes;
                std::vector<UShape> processed_triangle_shapes;
                // TODO maybe add `std::vector<UShape> processed_custom_shapes;`
                processed_point_shapes.reserve(shapes.size());
                processed_line_shapes.reserve(shapes.size());
                processed_triangle_shapes.reserve(shapes.size());
                // NOTE `process_shapes_z_order` converts all shapes to TRIANGLES with the exception of
                //      POINTS and LINE* shapes that may be deferred to separate render passes
                //      ( i.e `processed_point_shapes` and `processed_line_shapes` ) where they
                //      may be handle differently ( e.g rendered with point shader or in natively )
                process_shapes_z_order(processed_point_shapes, processed_line_shapes, processed_triangle_shapes);
                // NOTE `flush_processed_shapes` renders shapes according to the current render mode.
                flush_sort_by_z_order(processed_point_shapes,
                                      processed_line_shapes,
                                      processed_triangle_shapes,
                                      view_matrix,
                                      projection_matrix);
                run_once({ print_frame_info(processed_point_shapes, processed_line_shapes, processed_triangle_shapes); });
            }
        } else if (graphics->get_render_mode() == RENDER_MODE_SORTED_BY_SUBMISSION_ORDER || graphics->get_render_mode() == RENDER_MODE_IMMEDIATELY) {
            if (graphics->get_render_mode() == RENDER_MODE_SORTED_BY_SUBMISSION_ORDER) {
                console_once(format_label("render_mode"), "RENDER_MODE_SORTED_BY_SUBMISSION_ORDER ( rendering shapes in submission order )");
            } else {
                console_once(format_label("render_mode"), "RENDER_MODE_IMMEDIATELY ( rendering shapes immediately )");
            }
            TRACE_SCOPE_N("RENDER_MODE_SORTED_BY_SUBMISSION_ORDER/RENDER_MODE_IMMEDIATELY");
            {
                std::vector<UShape> processed_shapes;
                processed_shapes.reserve(shapes.size());
                // NOTE `process_shapes_z_order` converts all shapes to TRIANGLES with the exception of
                //      POINTS and LINE* shapes that may be deferred to separate render passes
                //      ( i.e `processed_point_shapes` and `processed_line_shapes` ) where they
                //      may be handle differently ( e.g rendered with point shader or in natively )
                process_shapes_submission_order(processed_shapes);
                // NOTE `flush_processed_shapes` renders shapes according to the current render mode.
                flush_submission_order(processed_shapes, view_matrix, projection_matrix);
                run_once({ print_frame_info({}, {}, processed_shapes); });
            }
        }

        // reserve capacity for next frame based on current usage
        prepare_next_flush_frame();
    }

    void UShapeRendererOpenGL_3::prepare_next_flush_frame() {
        const size_t current_size = shapes.size();
        shapes.clear();
        shapes.reserve(current_size);
        frame_light_shapes_count       = 0;
        frame_transparent_shapes_count = 0;
        frame_opaque_shapes_count      = 0;
        frame_textured_shapes_count    = 0;
    }

    void UShapeRendererOpenGL_3::print_frame_info(const std::vector<UShape>& processed_point_shapes, const std::vector<UShape>& processed_line_shapes, const std::vector<UShape>& processed_triangle_shapes) const {
        static constexpr int format_gap = 22;
        console("----------------------------");
        console("FRAME_INFO");
        console("----------------------------");
        console("SHAPES SUBMITTED");
        console(format_label("shapes", format_gap), frame_opaque_shapes_count);
        console(format_label("light_shapes", format_gap), frame_light_shapes_count);
        console(format_label("transparent_shapes", format_gap), frame_transparent_shapes_count);
        console(format_label("(textured_shapes)", format_gap), frame_textured_shapes_count);
        console("----------------------------");
        console("SHAPES PROCESSED");
        console(format_label("point_shapes", format_gap), processed_point_shapes.size());
        console(format_label("line_shapes", format_gap), processed_line_shapes.size());
        console(format_label("triangle_shapes", format_gap), processed_triangle_shapes.size());
        console("----------------------------");
    }

    static bool has_transparent_vertices(const std::vector<Vertex>& vertices) {
        for (auto& v: vertices) {
            if (v.color.a < 1.0f) {
                return true;
            }
        }
        return false;
    }

    void UShapeRendererOpenGL_3::init_shaders(const std::vector<PShader*>& shader_programms) {
        // NOTE for OpenGL ES 3.0 create shader source with dynamic array size
        //      ```c
        //      std::string transformsDefine = "#define MAX_TRANSFORMS " + std::to_string(MAX_TRANSFORMS) + "\n";
        //      const auto texturedVS = transformsDefine + R"(#version 330 core
        //      ```

        shader_color.id          = shader_programms[SHADER_PROGRAM_COLOR]->get_program_id();
        shader_texture.id        = shader_programms[SHADER_PROGRAM_TEXTURE]->get_program_id();
        shader_color_lights.id   = shader_programms[SHADER_PROGRAM_COLOR_LIGHTS]->get_program_id();
        shader_texture_lights.id = shader_programms[SHADER_PROGRAM_TEXTURE_LIGHTS]->get_program_id();
        // TODO implement
        //      shader_point.id              = shader_programms[SHADER_PROGRAM_POINT];
        //      shader_line.id               = shader_programms[SHADER_PROGRAM_LINE];

        setup_uniform_blocks("color", shader_color.id);
        setup_uniform_blocks("texture", shader_texture.id);
        setup_uniform_blocks("color_lights", shader_color_lights.id);
        setup_uniform_blocks("texture_lights", shader_texture_lights.id);
        // TODO shader_point.id
        // TODO shader_line.id

        /* cache uniform locations */
        shader_color.uniforms.uModelMatrixFallback  = glGetUniformLocation(shader_color.id, "uModelMatrixFallback");
        shader_color.uniforms.uViewProjectionMatrix = glGetUniformLocation(shader_color.id, "uViewProjectionMatrix");
        evaluate_shader_uniforms("color", shader_color.uniforms);

        shader_texture.uniforms.uModelMatrixFallback  = glGetUniformLocation(shader_texture.id, "uModelMatrixFallback");
        shader_texture.uniforms.uViewProjectionMatrix = glGetUniformLocation(shader_texture.id, "uViewProjectionMatrix");
        shader_texture.uniforms.uTextureUnit          = glGetUniformLocation(shader_texture.id, "uTextureUnit");
        evaluate_shader_uniforms("texture", shader_texture.uniforms);

        /* cache lighting shader uniform locations */
        // shader_color_lights.uniforms.normalMatrix  = glGetUniformLocation(shader_color_lights.id, "normalMatrix"); // TODO "normalMatrix as Transform"
        shader_color_lights.uniforms.uModelMatrixFallback  = glGetUniformLocation(shader_color_lights.id, "uModelMatrixFallback");
        shader_color_lights.uniforms.uViewProjectionMatrix = glGetUniformLocation(shader_color_lights.id, "uViewProjectionMatrix");
        shader_color_lights.uniforms.uViewMatrix           = glGetUniformLocation(shader_color_lights.id, "uViewMatrix");
        shader_color_lights.uniforms.ambient               = glGetUniformLocation(shader_color_lights.id, "ambient");
        shader_color_lights.uniforms.specular              = glGetUniformLocation(shader_color_lights.id, "specular");
        shader_color_lights.uniforms.emissive              = glGetUniformLocation(shader_color_lights.id, "emissive");
        shader_color_lights.uniforms.shininess             = glGetUniformLocation(shader_color_lights.id, "shininess");
        shader_color_lights.uniforms.lightCount            = glGetUniformLocation(shader_color_lights.id, "lightCount");
        shader_color_lights.uniforms.lightPosition         = glGetUniformLocation(shader_color_lights.id, "lightPosition");
        shader_color_lights.uniforms.lightNormal           = glGetUniformLocation(shader_color_lights.id, "lightNormal");
        shader_color_lights.uniforms.lightAmbient          = glGetUniformLocation(shader_color_lights.id, "lightAmbient");
        shader_color_lights.uniforms.lightDiffuse          = glGetUniformLocation(shader_color_lights.id, "lightDiffuse");
        shader_color_lights.uniforms.lightSpecular         = glGetUniformLocation(shader_color_lights.id, "lightSpecular");
        shader_color_lights.uniforms.lightFalloff          = glGetUniformLocation(shader_color_lights.id, "lightFalloff");
        shader_color_lights.uniforms.lightSpot             = glGetUniformLocation(shader_color_lights.id, "lightSpot");
        evaluate_shader_uniforms("color_lights", shader_color_lights.uniforms);

        // shader_texture_lights.uniforms.normalMatrix  = glGetUniformLocation(shader_texture_lights.id, "normalMatrix"); // TODO "normalMatrix as Transform"
        shader_texture_lights.uniforms.uModelMatrixFallback  = glGetUniformLocation(shader_texture_lights.id, "uModelMatrixFallback");
        shader_texture_lights.uniforms.uTextureUnit          = glGetUniformLocation(shader_texture_lights.id, "uTextureUnit");
        shader_texture_lights.uniforms.uViewProjectionMatrix = glGetUniformLocation(shader_texture_lights.id, "uViewProjectionMatrix");
        shader_texture_lights.uniforms.uViewMatrix           = glGetUniformLocation(shader_texture_lights.id, "uViewMatrix");
        shader_texture_lights.uniforms.ambient               = glGetUniformLocation(shader_texture_lights.id, "ambient");
        shader_texture_lights.uniforms.specular              = glGetUniformLocation(shader_texture_lights.id, "specular");
        shader_texture_lights.uniforms.emissive              = glGetUniformLocation(shader_texture_lights.id, "emissive");
        shader_texture_lights.uniforms.shininess             = glGetUniformLocation(shader_texture_lights.id, "shininess");
        shader_texture_lights.uniforms.lightCount            = glGetUniformLocation(shader_texture_lights.id, "lightCount");
        shader_texture_lights.uniforms.lightPosition         = glGetUniformLocation(shader_texture_lights.id, "lightPosition");
        shader_texture_lights.uniforms.lightNormal           = glGetUniformLocation(shader_texture_lights.id, "lightNormal");
        shader_texture_lights.uniforms.lightAmbient          = glGetUniformLocation(shader_texture_lights.id, "lightAmbient");
        shader_texture_lights.uniforms.lightDiffuse          = glGetUniformLocation(shader_texture_lights.id, "lightDiffuse");
        shader_texture_lights.uniforms.lightSpecular         = glGetUniformLocation(shader_texture_lights.id, "lightSpecular");
        shader_texture_lights.uniforms.lightFalloff          = glGetUniformLocation(shader_texture_lights.id, "lightFalloff");
        shader_texture_lights.uniforms.lightSpot             = glGetUniformLocation(shader_texture_lights.id, "lightSpot");
        evaluate_shader_uniforms("texture_lights", shader_texture_lights.uniforms);

        // TODO add point + line shader program
    }

    void UShapeRendererOpenGL_3::init_buffers() {
        glGenVertexArrays(1, &default_vao);
        bind_default_vertex_buffer();

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

        unbind_default_vertex_buffer();
    }

    void UShapeRendererOpenGL_3::computeShapeCenter(UShape& s) const {
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

    void UShapeRendererOpenGL_3::enable_depth_testing() {
        // TODO figure out if and how we might handle this hint: if (graphics != nullptr && graphics->hint_enable_depth_test) {}
        PGraphicsOpenGL::OGL_enable_depth_testing();
    }

    void UShapeRendererOpenGL_3::enable_blending() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // TODO blendfunction should or could be configurable
    }

    void UShapeRendererOpenGL_3::disable_blending() {
        glDisable(GL_BLEND);
    }

    void UShapeRendererOpenGL_3::enable_depth_buffer_writing() {
        PGraphicsOpenGL::OGL_enable_depth_buffer_writing();
    }

    void UShapeRendererOpenGL_3::disable_depth_buffer_writing() {
        PGraphicsOpenGL::OGL_disable_depth_buffer_writing();
    }

    void UShapeRendererOpenGL_3::disable_depth_testing() {
        PGraphicsOpenGL::OGL_disable_depth_testing();
    }

    void UShapeRendererOpenGL_3::bind_default_vertex_buffer() const {
        // TODO optimize by caching currently bound VBO
        glBindVertexArray(default_vao); // NOTE VAOs are only guaranteed works for OpenGL ≥ 3
    }

    void UShapeRendererOpenGL_3::unbind_default_vertex_buffer() {
        glBindVertexArray(0); // NOTE VAOs are only guaranteed works for OpenGL ≥ 3
    }

    void UShapeRendererOpenGL_3::enable_flat_shaders_and_bind_texture(GLuint& current_shader_program_id, const unsigned texture_id) const {
        if (texture_id == TEXTURE_NONE) {
            if (current_shader_program_id != shader_color.id) {
                current_shader_program_id = shader_color.id;
                glUseProgram(current_shader_program_id);
            }
        } else {
            if (current_shader_program_id != shader_texture.id) {
                current_shader_program_id = shader_texture.id;
                glUseProgram(current_shader_program_id);
            }
            PGraphicsOpenGL::OGL_bind_texture(texture_id);
        }
    }

    void UShapeRendererOpenGL_3::enable_light_shaders_and_bind_texture(GLuint& current_shader_program_id, const unsigned texture_id) const {
        if (texture_id == TEXTURE_NONE) {
            if (current_shader_program_id != shader_color_lights.id) {
                current_shader_program_id = shader_color_lights.id;
                glUseProgram(current_shader_program_id);
            }
        } else {
            if (current_shader_program_id != shader_texture_lights.id) {
                current_shader_program_id = shader_texture_lights.id;
                glUseProgram(current_shader_program_id);
            }
            PGraphicsOpenGL::OGL_bind_texture(texture_id);
        }
    }

    void UShapeRendererOpenGL_3::setup_uniform_blocks(const std::string& shader_name, const GLuint program) {
        // NOTE uniform blocks are only setup for built-in shaders
        //      custom shaders must setup uniform blocks manually
        const GLuint blockIndex = glGetUniformBlockIndex(program, "Transforms");
        if (blockIndex == GL_INVALID_INDEX) {
            warning(shader_name, ": block uniform 'Transforms' not found");
        } else {
            glUniformBlockBinding(program, blockIndex, 0);
        }
    }

    bool UShapeRendererOpenGL_3::evaluate_shader_uniforms(const std::string& shader_name, const ShaderUniforms& uniforms) {
        bool valid = true;
        if (uniforms.uViewProjectionMatrix == ShaderUniforms::NOT_FOUND) {
            warning(shader_name, ": uniform 'uViewProjectionMatrix' not found");
            valid = false;
        }
        if (uniforms.uModelMatrixFallback == ShaderUniforms::NOT_FOUND) {
            warning(shader_name, ": uniform 'uModelMatrixFallback' not found");
            valid = false;
        }
        /* only check uTextureUnit for texture shaders */
        if (shader_name.find("texture") != std::string::npos) {
            if (uniforms.uTextureUnit == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'uTextureUnit' not found");
                valid = false;
            }
        }
        /* only check lighting uniforms for lighting shaders */
        if (shader_name.find("lights") != std::string::npos) {
            if (uniforms.uViewMatrix == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'uViewMatrix' not found");
                valid = false;
            }
            // TODO consider "normalMatrix as Transform" ... see model_matrix
            // if (uniforms.normalMatrix == ShaderUniforms::NOT_FOUND) {
            //     warning(shader_name, ": uniform 'normalMatrix' not found");
            //     valid = false;
            // }
            if (uniforms.ambient == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'ambient' not found");
                valid = false;
            }
            if (uniforms.specular == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'specular' not found");
                valid = false;
            }
            if (uniforms.emissive == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'emissive' not found");
                valid = false;
            }
            if (uniforms.shininess == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'shininess' not found");
                valid = false;
            }
            if (uniforms.lightCount == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'lightCount' not found");
                valid = false;
            }
            if (uniforms.lightPosition == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'lightPosition' not found");
                valid = false;
            }
            if (uniforms.lightNormal == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'lightNormal' not found");
                valid = false;
            }
            if (uniforms.lightAmbient == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'lightAmbient' not found");
                valid = false;
            }
            if (uniforms.lightDiffuse == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'lightDiffuse' not found");
                valid = false;
            }
            if (uniforms.lightSpecular == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'lightSpecular' not found");
                valid = false;
            }
            if (uniforms.lightFalloff == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'lightFalloff' not found");
                valid = false;
            }
            if (uniforms.lightSpot == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'lightSpot' not found");
                valid = false;
            }
        }
        return valid;
    }

    void UShapeRendererOpenGL_3::set_per_frame_default_shader_uniforms(const glm::mat4& view_projection_matrix,
                                                                       const int        frame_has_light_shapes,
                                                                       const int        frame_has_transparent_shapes,
                                                                       const int        frame_has_opaque_shapes) const {
        // NOTE set view_projection_matrix and texture unit in all default shaders
        if (frame_has_light_shapes > 0) {
            glUseProgram(shader_color_lights.id);
            glUniformMatrix4fv(shader_color_lights.uniforms.uViewProjectionMatrix, 1, GL_FALSE, &view_projection_matrix[0][0]);

            glUseProgram(shader_texture_lights.id);
            glUniformMatrix4fv(shader_texture_lights.uniforms.uViewProjectionMatrix, 1, GL_FALSE, &view_projection_matrix[0][0]);
            glUniform1i(shader_texture_lights.uniforms.uTextureUnit, 0);
        }
        if (frame_has_opaque_shapes > 0 || frame_has_transparent_shapes > 0) {
            glUseProgram(shader_color.id);
            glUniformMatrix4fv(shader_color.uniforms.uViewProjectionMatrix, 1, GL_FALSE, &view_projection_matrix[0][0]);

            glUseProgram(shader_texture.id);
            glUniformMatrix4fv(shader_texture.uniforms.uViewProjectionMatrix, 1, GL_FALSE, &view_projection_matrix[0][0]);
            glUniform1i(shader_texture.uniforms.uTextureUnit, 0);
        }
        // TODO implement point and line shaders
    }

    void UShapeRendererOpenGL_3::set_light_uniforms(const ShaderUniforms& uniforms, const LightingState& lighting) {
        // OPTIMIZE only update uniforms that are dirty
        if (uniform_exists(uniforms.ambient)) {
            glUniform4fv(uniforms.ambient, 1, &lighting.ambient[0]);
        }
        if (uniform_exists(uniforms.specular)) {
            glUniform4fv(uniforms.specular, 1, &lighting.specular[0]);
        }
        if (uniform_exists(uniforms.emissive)) {
            glUniform4fv(uniforms.emissive, 1, &lighting.emissive[0]);
        }
        if (uniform_exists(uniforms.shininess)) {
            glUniform1f(uniforms.shininess, lighting.shininess);
        }

        const int count = std::min(lighting.lightCount, LightingState::MAX_LIGHTS);
        if (uniform_exists(uniforms.lightCount)) {
            glUniform1i(uniforms.lightCount, count);
        }
        if (count <= 0) {
            return;
        }

        if (uniform_exists(uniforms.lightPosition)) {
            glUniform4fv(uniforms.lightPosition, count, reinterpret_cast<const float*>(lighting.lightPositions));
        }
        if (uniform_exists(uniforms.lightNormal)) {
            glUniform3fv(uniforms.lightNormal, count, reinterpret_cast<const float*>(lighting.lightNormals));
        }
        if (uniform_exists(uniforms.lightAmbient)) {
            glUniform3fv(uniforms.lightAmbient, count, reinterpret_cast<const float*>(lighting.lightAmbientColors));
        }
        if (uniform_exists(uniforms.lightDiffuse)) {
            glUniform3fv(uniforms.lightDiffuse, count, reinterpret_cast<const float*>(lighting.lightDiffuseColors));
        }
        if (uniform_exists(uniforms.lightSpecular)) {
            glUniform3fv(uniforms.lightSpecular, count, reinterpret_cast<const float*>(lighting.lightSpecularColors));
        }
        if (uniform_exists(uniforms.lightFalloff)) {
            glUniform3fv(uniforms.lightFalloff, count, reinterpret_cast<const float*>(lighting.lightFalloffCoeffs));
        }
        if (uniform_exists(uniforms.lightSpot)) {
            glUniform2fv(uniforms.lightSpot, count, reinterpret_cast<const float*>(lighting.lightSpotParams));
        }
    }

    const UShapeRendererOpenGL_3::ShaderProgram& UShapeRendererOpenGL_3::get_shader_program_cached() const {
        return frame_state_cache.cached_shader_program;
    }

    bool UShapeRendererOpenGL_3::use_shader_program_cached(const ShaderProgram& required_shader_program) {
        // TODO use this for all program changes i.e make `cached_shader_program` a global property
        if (required_shader_program.id != frame_state_cache.cached_shader_program.id) {
            frame_state_cache.cached_shader_program = required_shader_program;
            glUseProgram(frame_state_cache.cached_shader_program.id);
            return true;
        }
        return false;
    }

    bool UShapeRendererOpenGL_3::set_uniform_model_matrix(const UShape&        shape,
                                                          const ShaderProgram& shader_program) {
        if (uniform_available(shader_program.uniforms.uModelMatrixFallback)) {
            glUniformMatrix4fv(shader_program.uniforms.uModelMatrixFallback, 1, GL_FALSE, &shape.model[0][0]);
            return true;
        }
        return false;
    }

    void UShapeRendererOpenGL_3::set_point_size_and_line_width(const UShape& shape) const {
        // TODO this does not work so well … better move to shader soon
        // TODO this is a bit hacky … is there a better place?
        /* NOTE draw stroke shapes */
        if (shape.mode == LINES ||
            shape.mode == LINE_STRIP ||
            shape.mode == LINE_LOOP) {
            if (graphics->get_stroke_render_mode() == STROKE_RENDER_MODE_NATIVE) {
#ifndef OPENGL_ES_3_0
                static bool    query_once = false;
                static GLfloat line_width_range[2];
                if (!query_once) {
                    query_once = true;
                    glGetFloatv(GL_LINE_WIDTH_RANGE, line_width_range);
                    console(format_label("native line width range"), nf(line_width_range[0], 1), "px — ", nf(line_width_range[1], 1), "px");
                }

                // Clamp the line width to supported range
                const float clamped_width = std::clamp(shape.stroke.stroke_weight,
                                                       line_width_range[0],
                                                       line_width_range[1]);
#else
                constexpr float clamped_width = 1;
#endif
                glLineWidth(clamped_width);
            }
        }
#ifndef OPENGL_ES_3_0
        else if (shape.mode == POINTS) {
            static bool    query_once = false;
            static GLfloat point_size_range[2];
            if (!query_once) {
                query_once = true;
                glGetFloatv(GL_POINT_SIZE_RANGE, point_size_range);
                console(format_label("native point size range"), nf(point_size_range[0], 1), "px — ", nf(point_size_range[1], 1), "px");
            }
            if (graphics->get_point_render_mode() == POINT_RENDER_MODE_NATIVE) {
                const float clamped_size = std::clamp(shape.stroke.point_weight,
                                                      point_size_range[0],
                                                      point_size_range[1]);
                glPointSize(clamped_size);
            }
        }
#endif
    }

    /**
     * render shapes in batches (preprocess).
     *
     * - preprocess shapes into texture batches for transparen, opaque, and transparent shapes.
     * - sort transparent shape by z-order.
     * render light shapes without transparency similar to opaque shapes.
     *
     * @param point_shapes
     * @param line_shapes
     * @param triangulated_shapes
     * @param view_matrix
     * @param projection_matrix
     */
    void UShapeRendererOpenGL_3::flush_sort_by_z_order(const std::vector<UShape>& point_shapes,
                                                       const std::vector<UShape>& line_shapes,
                                                       std::vector<UShape>&       triangulated_shapes,
                                                       const glm::mat4&           view_matrix,
                                                       const glm::mat4&           projection_matrix) {
        if (point_shapes.empty() && line_shapes.empty() && triangulated_shapes.empty()) { return; }

        std::vector<UShape> opaque_flat_shapes;
        std::vector<UShape> opaque_light_shapes;
        std::vector<UShape> opaque_custom_shapes;
        std::vector<UShape> transparent_shapes;

        /* pre allocate shape bins */
        // OPTIMIZE save this iteration by counting shapes at submission time ( and processing time )
        size_t transparent_count = 0, light_count = 0, custom_count = 0, flat_count = 0;
        for (const auto& s: triangulated_shapes) {
            if (s.transparent) {
                transparent_count++;
            } else if (s.light_enabled) {
                light_count++;
            } else if (s.shader != nullptr || s.vertex_buffer != nullptr) {
                custom_count++;
            } else {
                flat_count++;
            }
        }
        transparent_shapes.reserve(transparent_count);
        opaque_light_shapes.reserve(light_count);
        opaque_custom_shapes.reserve(custom_count);
        opaque_flat_shapes.reserve(flat_count);
        /* sort opaque shapes into bins */
        bool has_custom_transparent_shapes = false;
        for (auto& s: triangulated_shapes) {
            if (s.transparent) {
                transparent_shapes.emplace_back(std::move(s));
                if (s.shader != nullptr || s.vertex_buffer != nullptr) {
                    has_custom_transparent_shapes = true;
                }
            } else if (s.light_enabled) {
                opaque_light_shapes.emplace_back(std::move(s));
            } else if (s.shader != nullptr || s.vertex_buffer != nullptr) {
                opaque_custom_shapes.emplace_back(std::move(s));
            } else {
                opaque_flat_shapes.emplace_back(std::move(s));
            }
        }
        console_once("----------------------------");
        console_once("FLUSH SORT_BY_Z_ORDER STATS");
        console_once("----------------------------");
        console_once(format_label("point shapes"), point_shapes.size());
        console_once(format_label("line shapes"), line_shapes.size());
        console_once(format_label("opaque flat shapes"), opaque_flat_shapes.size());
        console_once(format_label("opaque light shapes"), opaque_light_shapes.size());
        console_once(format_label("opaque custom shapes"), opaque_custom_shapes.size());
        console_once(format_label("transparent shapes"), transparent_shapes.size());
        console_once(format_label("has custom transparent shape"), has_custom_transparent_shapes ? "YES" : "NO");

        /* compute view_projection_matrix once perframe */
        const glm::mat4 view_projection_matrix = projection_matrix * view_matrix;

        /* sort flat shapes into texture batches */
        std::unordered_map<GLuint, TextureBatch> flat_shape_batches;
        flat_shape_batches.reserve(frame_textured_shapes_count + 1); // TODO find better way to estimate size
        // OPTIMIZE maybe not sort into texture batches if there are not a lot of different textures see `frame_textured_shapes_count`
        for (auto& s: opaque_flat_shapes) {
            TextureBatch& batch = flat_shape_batches[s.texture_id]; // use unordered_map for better performance with many textures
            batch.texture_id    = s.texture_id;
            if (s.transparent) {
                error("why are there transparent shapes … this should never happen");
            }
            batch.shapes.push_back(&s);
            batch.max_vertices += s.vertices.size();
        }

        /* sort light shapes into texture batches */
        std::unordered_map<GLuint, TextureBatch> light_shape_batches;
        light_shape_batches.reserve(frame_light_shapes_count + 1); // TODO find better way to estimate size
        // OPTIMIZE maybe not sort into texture batches if there are not a lot of different textures see `frame_textured_shapes_count`
        for (auto& s: opaque_light_shapes) {
            TextureBatch& batch = light_shape_batches[s.texture_id]; // use unordered_map for better performance with many textures
            batch.texture_id    = s.texture_id;
            if (s.transparent) {
                error("why are there transparent shapes … this should never happen");
            }
            batch.shapes.push_back(&s);
            batch.max_vertices += s.vertices.size();
        }
        /* determine if VBO needs to be resized */
        for (auto& [textureID, batch]: flat_shape_batches) {
            if (batch.max_vertices > frame_state_cache.cached_max_vertices_per_batch) {
                frame_state_cache.cached_max_vertices_per_batch = batch.max_vertices;
                frame_state_cache.cached_require_buffer_resize  = true;
            }
        }
        for (auto& [textureID, batch]: light_shape_batches) {
            if (batch.max_vertices > frame_state_cache.cached_max_vertices_per_batch) {
                frame_state_cache.cached_max_vertices_per_batch = batch.max_vertices;
                frame_state_cache.cached_require_buffer_resize  = true;
            }
        }
        /* reserve vertex buffer once per flush frame */
        if (frame_state_cache.cached_require_buffer_resize) {
            current_vertex_buffer.reserve(frame_state_cache.cached_max_vertices_per_batch);
        }
        /* compute depth and sort transparent shapes */
        if (frame_transparent_shapes_count > 0) {
            for (UShape& s: transparent_shapes) {
                const glm::vec4 center_world_space = s.model * glm::vec4(s.center_object_space, 1.0f);
                const glm::vec4 center_view_space  = view_projection_matrix * center_world_space;
                s.depth                            = center_view_space.z / center_view_space.w; // Proper NDC depth
            }
            std::sort(transparent_shapes.begin(), transparent_shapes.end(),
                      [](const UShape& A, const UShape& B) {
                          return A.depth > B.depth;
                      }); // back to front
        }

        bind_default_vertex_buffer();

        // NOTE some uniforms only need to be set once per (flush) frame
        set_per_frame_default_shader_uniforms(view_projection_matrix,
                                              frame_light_shapes_count,
                                              frame_transparent_shapes_count,
                                              frame_opaque_shapes_count);

        /* render pass: opaque point + line shapes */
        // TODO point and line shapes ( that are not triangulated  )
        //      are always treated as opaque ... maybe there is
        //      a smarter way to handle this?
        for (auto& shape: point_shapes) {
            render_shape(shape);
        }
        for (auto& shape: line_shapes) {
            render_shape(shape);
        }
        /* render pass: opaque flat shapes */
        if (frame_opaque_shapes_count > 0) {
            enable_depth_testing();
            enable_depth_buffer_writing();
            disable_blending();
            for (auto& [texture_id, batch]: flat_shape_batches) {
                enable_flat_shaders_and_bind_texture(frame_state_cache.cached_shader_program.id, texture_id);
                render_batch(batch.shapes);
            }
        }
        /* render pass: opaque light shapes */
        if (frame_light_shapes_count > 0) {
            enable_depth_testing();
            enable_depth_buffer_writing();
            disable_blending();
            for (auto& [texture_id, batch]: light_shape_batches) {
                enable_light_shaders_and_bind_texture(frame_state_cache.cached_shader_program.id, texture_id);
                render_batch(batch.shapes);
            }
        }
        /* render pass: opaque custom shapes */
        for (auto& shape: opaque_custom_shapes) {
            render_shape(shape);
        }
        /* render pass: transparent shapes */
        if (frame_transparent_shapes_count > 0) {
            for (auto& shape: transparent_shapes) {
                render_shape(shape);
            }
        }

        unbind_default_vertex_buffer();
    }

    /**
     * render shapes directly (no preprocess).
     *
     * - render shapes directly ordered by their submission
     *
     * @param shapes
     * @param view_matrix
     * @param projection_matrix
     */
    void UShapeRendererOpenGL_3::flush_submission_order(const std::vector<UShape>& shapes,
                                                        const glm::mat4&           view_matrix,
                                                        const glm::mat4&           projection_matrix) {
        if (shapes.empty()) { return; }
        const glm::mat4 view_projection_matrix = projection_matrix * view_matrix;

        // NOTE some uniforms only need to be set once per (flush) frame
        // OPTIMIZE only set uniforms if they have actually changed in the (default) shaders
        //          maybe this should then be moved to `render_shape`
        set_per_frame_default_shader_uniforms(view_projection_matrix,
                                              frame_light_shapes_count,
                                              frame_transparent_shapes_count,
                                              frame_opaque_shapes_count);
        // TODO maybe remove the above and handle it with cacxxxxxxhing flags entirely in loop below … don t forget `glUniform1i(shader_xxx.uniforms.uTextureUnit, 0);`

        bind_default_vertex_buffer();

        /* render each shape individually in submission order */
        for (auto& shape: shapes) {
            render_shape(shape);
        }

        /* restore default state */
        unbind_default_vertex_buffer();
    }

    void UShapeRendererOpenGL_3::flush_immediately(const std::vector<UShape>& shapes,
                                                   const glm::mat4&           view_matrix,
                                                   const glm::mat4&           projection_matrix) {
        flush_submission_order(shapes, view_matrix, projection_matrix);
    }

    /**
     * convert shapes to primitive types e.g:
     *
     * - filled shapes become triangles
     * - stroke shapes are converted to triangles, line strips or are moved to shader-based collections ( depending on point and line render modes )
     *
     * note, that this method might create additional shapes
     *
     * @param processed_point_shapes
     * @param processed_line_shapes
     * @param processed_triangle_shapes
     */
    void UShapeRendererOpenGL_3::process_shapes_z_order(std::vector<UShape>& processed_point_shapes,
                                                        std::vector<UShape>& processed_line_shapes,
                                                        std::vector<UShape>& processed_triangle_shapes) {
        // ReSharper disable once CppDFAConstantConditions
        if (shapes.empty() || graphics == nullptr) { return; }

        // NOTE depending on render mode and point and line and render modes
        //      shapes are either converted to triangles or stored in dedicated bins

        for (auto& s: shapes) {
            /* stroke shapes */
            if (!s.filled) {
                if (s.mode == POINTS) {
                    /* point shapes */
                    process_point_shape_z_order(processed_triangle_shapes, processed_point_shapes, s);
                } else {
                    /* all other shapes */
                    process_stroke_shapes_z_order(processed_triangle_shapes, processed_line_shapes, s);
                }
                // NOTE `continue` prevents shapes that were converted to filled triangles
                //       from being added again as a filled shape.
                continue;
            }
            /* fill shapes */
            if (s.filled) {
                // TODO @maybe move this to PGraphics
                // OPTIMIZE also make use of other native modes like TRIANGLE_FAN and TRIANGLE_STRIP
                /* convert filled shapes to triangles */
                graphics->convert_fill_shape_to_triangles(s);
                s.mode = TRIANGLES;
                processed_triangle_shapes.push_back(std::move(s));
            }
        }
    }

    void UShapeRendererOpenGL_3::process_shapes_submission_order(std::vector<UShape>& processed_shapes) {
        // ReSharper disable once CppDFAConstantConditions
        if (shapes.empty() || graphics == nullptr) { return; }

        for (auto& s: shapes) {
            /* stroke shapes */
            // NOTE 'stroke shapes' are required to either be converted to TRIANGLES
            //      and handled as filled shapes ( e.g triangulated outlines or point sprite point )
            //      or to convert non-stroke shape modes ( e.g QUADS or TRIANGLES ) to either POINTS, LINES, LINE_STRIP, LINE_LOOP modes.
            if (!s.filled) {
                if (s.mode == POINTS) {
                    /* point shapes */
                    process_point_shape_submission_order(processed_shapes, s);
                } else {
                    /* all other unfilled shapes */
                    process_stroke_shapes_submission_order(processed_shapes, s);
                }
                // NOTE `continue` prevents shapes that were converted to filled triangles
                //       from being added again as a filled shape.
                continue;
            }
            /* fill shapes */
            // NOTE 'fill shapes' are required to be converted to either TRIANGLES, TRIANGLE_STRIP, or TRIANGLE_FAN shape modes.
            if (s.filled) {
                // TODO @maybe move this to PGraphics
                // OPTIMIZE also make use of other native modes like TRIANGLE_FAN and TRIANGLE_STRIP
                /* convert all filled shapes to triangles */
                graphics->convert_fill_shape_to_triangles(s);
                if (s.vertex_buffer == nullptr) {
                    // assert(s.mode == TRIANGLES);
                    if (s.mode != TRIANGLES) {
                        warning_in_function_once("shape mode should be of type TRIANGLES. this should never happen ...");
                    }
                }
                processed_shapes.push_back(std::move(s));
            }
        }
    }

    void UShapeRendererOpenGL_3::convert_point_shape_to_triangles(std::vector<UShape>& processed_triangle_shapes, UShape& point_shape) {
        std::vector<Vertex> triangulated_vertices = convertPointsToTriangles(point_shape.vertices, point_shape.stroke.point_weight);
        point_shape.vertices                      = std::move(triangulated_vertices);
        point_shape.filled                        = true;
        point_shape.mode                          = TRIANGLES;
        point_shape.transparent                   = has_transparent_vertices(point_shape.vertices) ? true : point_shape.texture_id != TEXTURE_NONE;
        processed_triangle_shapes.push_back(std::move(point_shape));
    }

    void UShapeRendererOpenGL_3::convert_point_shape_for_shader(std::vector<UShape>& processed_point_shapes, UShape& point_shape) {
        // TODO handle this in extra path
        // emit_shape_stroke_points(shape_stroke_vertex_buffer, point_size);
        // IMPL_emit_shape_stroke_points
        // shader_point->use();
        // update_shader_matrices(shader_point);
        // // TODO this MUST be optimized! it is not efficient to update all uniforms every time
        // shader_point->set_uniform("viewport", glm::vec4(0, 0, width, height));
        // shader_point->set_uniform("perspective", 1);
        // std::vector<Vertex> point_vertices_expanded;
        //
        // for (size_t i = 0; i < point_vertices.size(); i++) {
        //     Vertex v0, v1, v2, v3;
        //
        //     v0 = point_vertices[i];
        //     v1 = point_vertices[i];
        //     v2 = point_vertices[i];
        //     v3 = point_vertices[i];
        //
        //     v0.normal.x = 0;
        //     v0.normal.y = 0;
        //
        //     v1.normal.x = point_size;
        //     v1.normal.y = 0;
        //
        //     v2.normal.x = point_size;
        //     v2.normal.y = point_size;
        //
        //     v3.normal.x = 0;
        //     v3.normal.y = point_size;
        //
        //     point_vertices_expanded.push_back(v0);
        //     point_vertices_expanded.push_back(v1);
        //     point_vertices_expanded.push_back(v2);
        //
        //     point_vertices_expanded.push_back(v0);
        //     point_vertices_expanded.push_back(v2);
        //     point_vertices_expanded.push_back(v3);
        // }
        // OGL3_render_vertex_buffer(vertex_buffer, GL_TRIANGLES, point_vertices_expanded);
        warning_in_function_once("TODO unsupported point render mode 'POINT_RENDER_MODE_POINT_SHADER'");
        processed_point_shapes.push_back(std::move(point_shape));
    }

    void UShapeRendererOpenGL_3::process_point_shape_z_order(std::vector<UShape>& processed_triangle_shapes, std::vector<UShape>& processed_point_shapes, UShape& point_shape) const {
        // NOTE this is required by `process_shapes_z_order`
        if (graphics->get_point_render_mode() == POINT_RENDER_MODE_TRIANGULATE) {
            convert_point_shape_to_triangles(processed_triangle_shapes, point_shape);
        } else if (graphics->get_point_render_mode() == POINT_RENDER_MODE_NATIVE) {
#ifdef OPENGL_ES_3_0
            warning_in_function_once("in OpenGL ES 3.0 native points behave weird … be warned! you might want to selecte a different point render mode.");
#endif
            if (point_shape.texture_id != TEXTURE_NONE) {
                // TODO how to handle textures here? HACK just remove texture for native render mode ... for now
                point_shape.texture_id = TEXTURE_NONE;
                warning_in_function_once("removing texture for points in native render mode");
            }
            processed_point_shapes.push_back(std::move(point_shape));
        } else if (graphics->get_point_render_mode() == POINT_RENDER_MODE_POINT_SHADER) {
            warning_in_function_once("TODO pointer shader is not implemented yet");
            convert_point_shape_for_shader(processed_point_shapes, point_shape);
        }
    }

    void UShapeRendererOpenGL_3::process_point_shape_submission_order(std::vector<UShape>& processed_shape_batch, UShape& point_shape) const {
        // NOTE this is required by `process_shapes_z_order`
        if (graphics->get_point_render_mode() == POINT_RENDER_MODE_TRIANGULATE) {
            convert_point_shape_to_triangles(processed_shape_batch, point_shape);
        } else if (graphics->get_point_render_mode() == POINT_RENDER_MODE_NATIVE) {
#ifdef OPENGL_ES_3_0
            warning_in_function_once("in OpenGL ES 3.0 native points behave weird … be warned! you might want to selecte a different point render mode.");
#endif
            if (point_shape.texture_id != TEXTURE_NONE) {
                // TODO how to handle textures here? HACK just remove texture for native render mode ... for now
                point_shape.texture_id = TEXTURE_NONE;
                warning_in_function_once("removing texture for points in native render mode");
            }
            processed_shape_batch.push_back(std::move(point_shape));
        } else if (graphics->get_point_render_mode() == POINT_RENDER_MODE_POINT_SHADER) {
            warning_in_function_once("TODO pointer shader is not implemented yet");
            convert_point_shape_for_shader(processed_shape_batch, point_shape);
        }
    }

    void UShapeRendererOpenGL_3::convert_stroke_shape_to_triangles_2D(std::vector<UShape>& processed_triangle_shapes, UShape& stroke_shape) const {
        std::vector<UShape> converted_shapes;
        converted_shapes.reserve(stroke_shape.vertices.size());
        PGraphics::convert_stroke_shape_to_line_strip(stroke_shape, converted_shapes);
        const bool shape_has_transparent_vertices = has_transparent_vertices(stroke_shape.vertices);
        if (!converted_shapes.empty()) {
            // TODO @maybe move this to PGraphics
            for (auto& cs: converted_shapes) {
                std::vector<Vertex> triangulated_vertices;
                graphics->triangulate_line_strip_vertex(cs.vertices,
                                                        cs.stroke,
                                                        cs.closed,
                                                        triangulated_vertices);
                cs.vertices    = std::move(triangulated_vertices);
                cs.filled      = true;
                cs.mode        = TRIANGLES;
                cs.transparent = shape_has_transparent_vertices;
                processed_triangle_shapes.push_back(std::move(cs));
            }
        }
    }

    void UShapeRendererOpenGL_3::convert_stroke_shape_to_triangles_3D_tube(std::vector<UShape>& processed_triangle_shapes, UShape& stroke_shape) {
        std::vector<UShape> converted_shapes;
        converted_shapes.reserve(stroke_shape.vertices.size());
        PGraphics::convert_stroke_shape_to_line_strip(stroke_shape, converted_shapes);
        const bool shape_has_transparent_vertices = has_transparent_vertices(stroke_shape.vertices);
        for (auto& cs: converted_shapes) {
            // TODO @maybe move this to PGraphics
            const std::vector<Vertex> triangulated_vertices = generate_tube_mesh(cs.vertices,
                                                                                 cs.stroke.stroke_weight / 2.0f,
                                                                                 cs.closed);
            cs.vertices                                     = std::move(triangulated_vertices);
            cs.filled                                       = true;
            cs.mode                                         = TRIANGLES;
            cs.transparent                                  = shape_has_transparent_vertices;
            processed_triangle_shapes.push_back(std::move(cs));
        }
        warning_in_function_once("untested stroke render mode 'STROKE_RENDER_MODE_TUBE_3D'");
    }

    void UShapeRendererOpenGL_3::convert_stroke_shape_for_native(UShape& stroke_shape) {
        // NOTE convert all shapes here that have no native OpenGL mode to:
        //      - LINES           -> GL_LINES
        //      - LINE_STRIP      -> GL_LINE_STRIPS
        //      - LINE_LOOP       -> GL_LINE_LOOP
        //      convert the following shapes to conform with one of the 3 native modes ( RESTART is for future implementation of indexed mode ):
        //      - LINES           -> GL_LINES
        //      - LINE_STRIP      -> GL_LINE_STRIPS
        //      - LINE_LOOP       -> GL_LINE_LOOP
        //      - TRIANGLES       -> GL_LINE_LOOP + RESTART
        //      - TRIANGLE_STRIP  -> GL_LINE_STRIPS + RESTART
        //      - TRIANGLE_FAN    -> GL_LINE_LOOP + RESTART
        //      - QUADS           -> GL_LINE_LOOP + RESTART
        //      - QUAD_STRIP      -> GL_LINE_STRIPS + RESTART
        //      - POLYGON(OPEN)   -> GL_LINE_STRIPS
        //      - POLYGON(CLOSED) -> GL_LINE_LOOP

        if (stroke_shape.mode == LINES ||
            stroke_shape.mode == LINE_STRIP ||
            stroke_shape.mode == LINE_LOOP) {
            return;
        }

        const auto&  v = stroke_shape.vertices;
        const size_t n = v.size();

        // helper to append one line segment (two vertices).
        std::vector<Vertex> out;
        out.reserve(n * 2); // rough upper bound for typical cases
        auto add_segment = [&](const size_t i, const size_t j) {
            if (i < n && j < n) {
                out.push_back(v[i]);
                out.push_back(v[j]);
            }
        };

        switch (stroke_shape.mode) {
            case TRIANGLES: {
                const size_t m = (n / 3) * 3;
                for (size_t i = 0; i + 2 < m; i += 3) {
                    const size_t a = i, b = i + 1, c = i + 2;
                    add_segment(a, b);
                    add_segment(b, c);
                    add_segment(c, a);
                }
                stroke_shape.vertices = std::move(out);
                stroke_shape.mode     = LINES;
                stroke_shape.closed   = false;
                stroke_shape.filled   = false;
                break;
            }
            case TRIANGLE_STRIP: {
                if (n >= 3) {
                    for (size_t k = 2; k < n; ++k) {
                        const size_t a = k - 2, b = k - 1, c = k;
                        add_segment(a, b);
                        add_segment(b, c);
                        add_segment(c, a);
                    }
                }
                stroke_shape.vertices = std::move(out);
                stroke_shape.mode     = LINES;
                stroke_shape.closed   = false;
                stroke_shape.filled   = false;
                break;
            }
            case TRIANGLE_FAN: {
                if (n >= 3) {
                    constexpr size_t center = 0;
                    for (size_t i = 1; i + 1 < n; ++i) {
                        const size_t a = center, b = i, c = i + 1;
                        add_segment(a, b);
                        add_segment(b, c);
                        add_segment(c, a);
                    }
                }
                stroke_shape.vertices = std::move(out);
                stroke_shape.mode     = LINES;
                stroke_shape.closed   = false;
                stroke_shape.filled   = false;
                break;
            }
            case QUADS: {
                const size_t q = (n / 4) * 4;
                for (size_t i = 0; i + 3 < q; i += 4) {
                    const size_t a = i, b = i + 1, c = i + 2, d = i + 3;
                    add_segment(a, b);
                    add_segment(b, c);
                    add_segment(c, d);
                    add_segment(d, a);
                }
                stroke_shape.vertices = std::move(out);
                stroke_shape.mode     = LINES;
                stroke_shape.closed   = false;
                stroke_shape.filled   = false;
                break;
            }
            case QUAD_STRIP: {
                // Each quad is (i,i+1,i+2,i+3) for i += 2
                for (size_t i = 0; i + 3 < n; i += 2) {
                    const size_t a = i, b = i + 1, c = i + 2, d = i + 3;
                    add_segment(a, b);
                    add_segment(b, d);
                    add_segment(d, c);
                    add_segment(c, a);
                }
                stroke_shape.vertices = std::move(out);
                stroke_shape.mode     = LINES;
                stroke_shape.closed   = false;
                stroke_shape.filled   = false;
                break;
            }
            case POLYGON:
            default: {
                // Map directly to native line topology based on `closed`.
                stroke_shape.mode   = stroke_shape.closed ? LINE_LOOP : LINE_STRIP;
                stroke_shape.filled = false;
                // vertices stay as-is
                break;
            }
        }
    }

    void UShapeRendererOpenGL_3::convert_stroke_shape_for_line_shader(std::vector<UShape>& processed_line_shapes, UShape& stroke_shape) {
        std::vector<UShape> converted_shapes;
        converted_shapes.reserve(stroke_shape.vertices.size());
        PGraphics::convert_stroke_shape_to_line_strip(stroke_shape, converted_shapes);
        for (auto& cs: converted_shapes) {
            // TODO this MUST be optimized! it is not efficient to update all uniforms every time
            // TODO move this to render pass
            // TODO emit warning when running OpenGL ES 3.0
            // shader_stroke->use();
            // update_shader_matrices(shader_stroke);
            // shader_stroke->set_uniform("viewport", glm::vec4(0, 0, width, height));
            // shader_stroke->set_uniform("perspective", 1);
            // constexpr float scale_factor = 0.99f;
            // shader_stroke->set_uniform("scale", glm::vec3(scale_factor, scale_factor, scale_factor));
            //
            // const float         stroke_weight_half = current_stroke_state.stroke_weight / 2.0f;
            // std::vector<Vertex> line_strip_vertices_expanded;
            // for (size_t i = 0; i + 1 < line_strip_vertices.size(); i++) {
            //     Vertex p0 = line_strip_vertices[i];
            //     Vertex p1 = line_strip_vertices[i + 1];
            //     add_line_quad(p0, p1, stroke_weight_half, line_strip_vertices_expanded);
            // }
            // if (line_strip_closed) {
            //     Vertex p0 = line_strip_vertices.back();
            //     Vertex p1 = line_strip_vertices.front();
            //     add_line_quad(p0, p1, stroke_weight_half, line_strip_vertices_expanded);
            // }
            // OGL3_render_vertex_buffer(vertex_buffer, GL_TRIANGLES, line_strip_vertices_expanded);
            processed_line_shapes.push_back(std::move(cs));
        }
        warning_in_function_once("unsupported stroke render mode 'STROKE_RENDER_MODE_LINE_SHADER'");
    }

    void UShapeRendererOpenGL_3::convert_stroke_shape_for_barycentric_shader(std::vector<UShape>& processed_line_shapes, UShape& stroke_shape) {
        std::vector<UShape> converted_shapes;
        converted_shapes.reserve(stroke_shape.vertices.size());
        PGraphics::convert_stroke_shape_to_line_strip(stroke_shape, converted_shapes);
        for (auto& cs: converted_shapes) {
            processed_line_shapes.push_back(std::move(cs));
        }
        warning_in_function_once("unsupported stroke render mode 'STROKE_RENDER_MODE_BARYCENTRIC_SHADER'");
    }

    void UShapeRendererOpenGL_3::convert_stroke_shape_for_geometry_shader(std::vector<UShape>& processed_line_shapes, UShape& stroke_shape) {
        std::vector<UShape> converted_shapes;
        converted_shapes.reserve(stroke_shape.vertices.size());
        PGraphics::convert_stroke_shape_to_line_strip(stroke_shape, converted_shapes);
        for (auto& cs: converted_shapes) {
            processed_line_shapes.push_back(std::move(cs));
        }
        warning_in_function_once("unsupported stroke render mode 'STROKE_RENDER_MODE_GEOMETRY_SHADER'");
    }

    // NOTE this is required by `process_shapes_z_order`
    void UShapeRendererOpenGL_3::process_stroke_shapes_z_order(std::vector<UShape>& processed_triangle_shapes,
                                                               std::vector<UShape>& processed_line_shapes,
                                                               UShape&              stroke_shape) const {
        if (graphics->get_stroke_render_mode() == STROKE_RENDER_MODE_TRIANGULATE_2D) {
            convert_stroke_shape_to_triangles_2D(processed_triangle_shapes, stroke_shape);
        } else if (graphics->get_stroke_render_mode() == STROKE_RENDER_MODE_TUBE_3D) {
            convert_stroke_shape_to_triangles_3D_tube(processed_triangle_shapes, stroke_shape);
        } else if (graphics->get_stroke_render_mode() == STROKE_RENDER_MODE_NATIVE) {
            convert_stroke_shape_for_native(stroke_shape);
            processed_line_shapes.push_back(std::move(stroke_shape));
        } else if (graphics->get_stroke_render_mode() == STROKE_RENDER_MODE_LINE_SHADER) {
            convert_stroke_shape_for_line_shader(processed_line_shapes, stroke_shape);
        } else if (graphics->get_stroke_render_mode() == STROKE_RENDER_MODE_BARYCENTRIC_SHADER) {
            convert_stroke_shape_for_barycentric_shader(processed_line_shapes, stroke_shape);
        } else if (graphics->get_stroke_render_mode() == STROKE_RENDER_MODE_GEOMETRY_SHADER) {
            convert_stroke_shape_for_geometry_shader(processed_line_shapes, stroke_shape);
        }
    }

    void UShapeRendererOpenGL_3::process_stroke_shapes_submission_order(std::vector<UShape>& processed_shape_batch, UShape& stroke_shape) const {
        const int stroke_render_mode = graphics->get_stroke_render_mode();
        switch (stroke_render_mode) {
            case STROKE_RENDER_MODE_TUBE_3D: {
                convert_stroke_shape_to_triangles_3D_tube(processed_shape_batch, stroke_shape);
            } break;
            case STROKE_RENDER_MODE_NATIVE: {
                if (stroke_shape.mode == LINES ||
                    stroke_shape.mode == LINE_STRIP ||
                    stroke_shape.mode == LINE_LOOP) {
                } else if (stroke_shape.mode == POINTS) {
                    // NOTE that POINTS are handled separately
                    warning_in_function_once("POINTS should not to be handled here");
                } else {
                    switch (stroke_shape.mode) {
                        case LINES:
                        case LINE_STRIP:
                        case LINE_LOOP:
                            break;
                        case TRIANGLES:
                        case TRIANGLE_STRIP:
                        case TRIANGLE_FAN:
                        case QUADS:
                        case QUAD_STRIP:
                            convert_stroke_shape_for_native(stroke_shape); // NOTE this converts one of the above shapes to a *renderable* native shape
                        case POLYGON:
                        default:
                            stroke_shape.mode = stroke_shape.closed ? LINE_LOOP : LINE_STRIP;
                    }
                }
                processed_shape_batch.push_back(std::move(stroke_shape));
            } break;
            case STROKE_RENDER_MODE_LINE_SHADER: {
                convert_stroke_shape_for_line_shader(processed_shape_batch, stroke_shape); // TODO
            } break;
            case STROKE_RENDER_MODE_BARYCENTRIC_SHADER: {
                convert_stroke_shape_for_barycentric_shader(processed_shape_batch, stroke_shape); // TODO
            } break;
            case STROKE_RENDER_MODE_GEOMETRY_SHADER: {
                convert_stroke_shape_for_geometry_shader(processed_shape_batch, stroke_shape); // TODO
            } break;
            case STROKE_RENDER_MODE_TRIANGULATE_2D:
            default: {
                convert_stroke_shape_to_triangles_2D(processed_shape_batch, stroke_shape);
            } break;
        }
    }

    size_t UShapeRendererOpenGL_3::estimate_triangle_count(const UShape& s) {
        const size_t n = s.vertices.size();
        if (n < 3 || !s.filled) {
            return 0;
        }

        switch (s.mode) {
            case TRIANGLES: return n / 3 * 3;
            case TRIANGLE_STRIP:
            case TRIANGLE_FAN:
            case POLYGON: return (n - 2) * 3;
            case QUADS: return n / 4 * 6;
            case QUAD_STRIP: return n >= 4 ? (n / 2 - 1) * 6 : 0;
            default: return 0;
        }
    }

    void UShapeRendererOpenGL_3::convert_shapes_to_triangles_and_set_transform_id(const UShape& s, std::vector<Vertex>& out, const uint16_t transformID) {
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
        // OPTIMIZE this is supposedly faster … profile!
        // auto pushTri = [&](size_t i0, size_t i1, size_t i2) {
        //     out.emplace_back(v[i0]);
        //     out.back().transform_id = transformID;
        //     out.emplace_back(v[i1]);
        //     out.back().transform_id = transformID;
        //     out.emplace_back(v[i2]);
        //     out.back().transform_id = transformID;
        // };

        switch (s.mode) {
            case TRIANGLES: {
                const size_t m = n / 3 * 3;
                for (size_t i = 0; i < m; ++i) {
                    Vertex vv       = v[i];
                    vv.transform_id = transformID;
                    out.push_back(vv);
                    // OPTIMIZE this is supposedly faster … profile!
                    // out.emplace_back(v[i]);
                    // out.back().transform_id = transformID;
                }
                // NOTE on MacBook Pro this is approx 6 µsec faster than
                //      ```
                //      const size_t m = n / 3 * 3;
                //      for (size_t i = 0; i < m; ++i) {
                //          Vertex vv       = v[i];
                //          vv.transform_id = transformID;
                //          out.push_back(vv);
                //      }
                //      ```
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
                const size_t q = n / 4 * 4;
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

    void UShapeRendererOpenGL_3::render_batch(const std::vector<UShape*>& shapes_to_render) {
        // NOTE assumes that ...
        //      - shader is in use
        //      - texture is bound
        //      - VBO is bound

        // TODO `render_batch` does not support custom shaders and custom vertex buffers
        //      or shape modes other than TRIANGLES

        if (shapes_to_render.empty()) {
            return;
        }

        for (size_t i = 0; i < shapes_to_render.size(); ++i) {
            const auto* s = shapes_to_render[i];
            if (s->shader != nullptr) {
                warning_in_function_once("custom shaders are currently not supported in this function");
            }
            if (s->vertex_buffer != nullptr) {
                warning_in_function_once("custom vertex buffers are currently not supported in this function");
            }
            if (s->mode != TRIANGLES) {
                warning_in_function_once("only shapes in TRIANGLES mode are supported in this function");
            }
        }

        /* process in chunks to respect MAX_TRANSFORMS limit */
        std::vector<glm::mat4> flush_frame_matrices;
        for (size_t offset = 0; offset < shapes_to_render.size(); offset += MAX_TRANSFORMS) {
            const size_t chunkSize = std::min(static_cast<size_t>(MAX_TRANSFORMS), shapes_to_render.size() - offset);
            /* upload transforms for this chunk */
            flush_frame_matrices.clear();
            flush_frame_matrices.reserve(chunkSize);
            for (size_t i = 0; i < chunkSize; ++i) {
                const auto* s = shapes_to_render[offset + i];
                flush_frame_matrices.push_back(s->model);
            }
            // OPTIMIZE this only needs to happen once per frame
            // TODO maybe move this outside of loop
            glBindBuffer(GL_UNIFORM_BUFFER, ubo);
            glBufferSubData(GL_UNIFORM_BUFFER, 0,
                            static_cast<GLsizeiptr>(flush_frame_matrices.size() * sizeof(glm::mat4)),
                            flush_frame_matrices.data());


            // TODO this needs to be OPTIMIZE e.g by caching last used lighting state
            for (size_t i = 0; i < chunkSize; ++i) {
                const auto* s = shapes_to_render[offset + i];
                if (s->light_enabled) {
                    if (s->texture_id == TEXTURE_NONE) {
                        set_light_uniforms(shader_color_lights.uniforms, s->lighting);
                    } else {
                        set_light_uniforms(shader_texture_lights.uniforms, s->lighting);
                    }
                }
            }
            /* tessellate shapes in this chunk */
            current_vertex_buffer.clear();
// #define OPTIMIZE_TRIANGLE_BATCHING
#ifdef OPTIMIZE_TRIANGLE_BATCHING
            TRACE_SCOPE_N("OPTIMIZE_TRIANGLE_BATCHING__EMPLACE_BACK");
            for (size_t i = 0; i < chunkSize; ++i) {
                const auto*    s           = shapes_to_render[offset + i];
                const auto&    v           = s->vertices;
                const size_t   m           = (v.size() / 3) * 3; // NOTE combine size calculation and triangle alignment
                const uint16_t transformID = static_cast<uint16_t>(i + PER_VERTEX_TRANSFORM_ID_START);

                for (size_t j = 0; j < m; ++j) {
                    current_vertex_buffer.emplace_back(v[j]);
                    current_vertex_buffer.back().transform_id = transformID;
                }
            }
#else
            TRACE_SCOPE_N("OPTIMIZE_TRIANGLE_BATCHING__PUSH_BACK");
            for (size_t i = 0; i < chunkSize; ++i) {
                const auto*    s           = shapes_to_render[offset + i];
                const auto&    v           = s->vertices;
                const size_t   n           = v.size();
                const size_t   m           = n / 3 * 3;
                const uint16_t transformID = static_cast<uint16_t>(i + PER_VERTEX_TRANSFORM_ID_START);
                for (size_t j = 0; j < m; ++j) {
                    Vertex vv       = v[j];
                    vv.transform_id = transformID;
                    current_vertex_buffer.push_back(vv);
                }
            }
#endif
        }
        if (!current_vertex_buffer.empty()) {
            GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo)); // TODO maybe move this outside of loop … but only if there a no shapes with custom vertex buffer
            if (frame_state_cache.cached_require_buffer_resize) {
                frame_state_cache.cached_require_buffer_resize = false;
                GL_CALL(glBufferData(GL_ARRAY_BUFFER,
                                     static_cast<GLsizeiptr>(frame_state_cache.cached_max_vertices_per_batch * sizeof(Vertex)),
                                     nullptr,
                                     GL_DYNAMIC_DRAW));
            }
            GL_CALL(glBufferSubData(GL_ARRAY_BUFFER,
                                    0, current_vertex_buffer.size() * sizeof(Vertex),
                                    current_vertex_buffer.data()));
            GL_CALL(glDrawArrays(GL_TRIANGLES,
                                 0,
                                 static_cast<GLsizei>(current_vertex_buffer.size())));
        }
    }
}

void UShapeRendererOpenGL_3::render_shape(const UShape& shape) {
    // NOTE 'render_shape' handles:
    //      - transparency + depth testing (writing?)
    //      - shader program usage
    //      - shader uniforms update
    //      - texture binding
    //      - vertex buffer binding and drawing

    const bool has_custom_shader        = shape.shader != nullptr;
    const bool has_custom_vertex_buffer = shape.vertex_buffer != nullptr;

    // NOTE 'render_shape' asssumes that a shape is either 'fill' or 'stroke':
    //       - 'fill shapes' are expected to render in shape mode TRIANGLES, TRIANGLE_STRIP or TRIANGLE_FAN
    //       - 'stroke shapes' are expected to render in shape mode POINTS, LINES, LINE_STRIP or LINE_LOOP
    if (shape.mode == TRIANGLES ||
        shape.mode == TRIANGLE_STRIP ||
        shape.mode == TRIANGLE_FAN) {
        /* NOTE draw filled shapes */
        // opengl_shape_mode = PGraphicsOpenGL::OGL_get_draw_mode(shape.mode);
        // tessellate shape into triangles and write to vertex buffer
        // current_vertex_buffer.clear();
        // current_vertex_buffer.reserve(vertex_count_estimate);
        // // OPTIMIZE check if this is the fastest way to prepare shapes for this case … maybe just use shape vertex data directly
        // convert_shapes_to_triangles_and_set_transform_id(shape, current_vertex_buffer, FALLBACK_MODEL_MATRIX_ID);
        // convert_shapes_to_triangles_and_set_transform_id(shape, shape.vertices, FALLBACK_MODEL_MATRIX_ID);
    } else if (shape.mode == POINTS ||
               shape.mode == LINES ||
               shape.mode == LINE_STRIP ||
               shape.mode == LINE_LOOP) {
        set_point_size_and_line_width(shape);
    } else {
        if (!has_custom_vertex_buffer) {
            // NOTE only emit warning von default vertex buffer ... this should never happen
            warning_in_function_once("shape mode not supported at this point ... this should never happen ... undefined behavior");
        }
    }
    /* handle transparency state changes */
    const bool desired_transparent_state = has_custom_vertex_buffer ? shape.vertex_buffer->get_transparent() : shape.transparent;
    if (desired_transparent_state) {
        if (!frame_state_cache.cached_transparent_shape_enabled) {
            frame_state_cache.cached_transparent_shape_enabled = true;
            enable_depth_testing();
            disable_depth_buffer_writing();
            enable_blending();
        }
    } else {
        if (frame_state_cache.cached_transparent_shape_enabled) {
            enable_depth_testing();
            enable_depth_buffer_writing();
            disable_blending();
            frame_state_cache.cached_transparent_shape_enabled = false;
        }
    }
    /* switch shader program ( if necessary ) */
    if (!has_custom_shader) {
        /* default shaders */
        const ShaderProgram required_shader_program = shape.light_enabled ? (shape.texture_id == TEXTURE_NONE ? shader_color_lights : shader_texture_lights) : (shape.texture_id == TEXTURE_NONE ? shader_color : shader_texture);
        const bool          changed_shader_program  = use_shader_program_cached(required_shader_program);
        if (changed_shader_program) {
            // TODO warning_in_function_once("default shader: do we need to update uniforms here?");
            // OPTIMIZE maybe use this to set a shader uniform ( e.g view matrix ) once per flush frame once it is reuqested for the first time
        }
        // NOTE always use fallback model matrix instead of UBO
        //      i.e vertex attribute 'aTransformID' needs to be 0
        set_uniform_model_matrix(shape, required_shader_program);
    } else {
        /* custom shader */
        warning_in_function_once("custom_shader: set shader uniforms per SHAPE ( e.g 'view_projection_matrix' )");
        const auto required_shader_program = ShaderProgram{.id = shape.shader->get_program_id()};
        // TODO what about the uniforms … should we maybe add the `ShaderProgram` struct to PShader?
        // TODO question: how to handle default shader compliant uniforms for custom shader?
        const bool changed_shader_program = use_shader_program_cached(required_shader_program);
        if (changed_shader_program) {
            // NOTE update uniforms per shape
            warning_in_function_once("custom_shader: update uniforms once per frame");
            // custom_shader->update_uniforms(shape);
            // TODO implement an option that TRIES to set the *known* uniforms:
            //      - model, view, projection matrices
            //      - light uniforms
            // if (!shape.shader->has_transform_block() && shape.shader->has_model_matrix) {
            //     shape.shader->set_uniform(SHADER_UNIFORM_MODEL_MATRIX, shape.model); // glUniformMatrix4fv
            // }
        }
        const bool result = set_uniform_model_matrix(shape, required_shader_program);
        if (!result) {
            warning_in_function_once("custom shader has no model matrix ( might be intended )");
        }
        /* upload transform (single mat4) if shader expects the transform block */
        if (shape.shader->has_transform_block()) {
            // TODO do we need or want this?!?
            warning_in_function_once("custom_shader: TODO set custom shader model uniform block");
        }
        if (shape.light_enabled) {
            // TODO skipping light uniforms for now
            warning_in_function_once("custom_shader: lighting currently not supported");
        }
    }
    /* set lights for this shape ( if enabled ) */
    if (shape.light_enabled) {
        if (!has_custom_shader) {
            if (shape.texture_id == TEXTURE_NONE) {
                set_light_uniforms(shader_color_lights.uniforms, shape.lighting);
            } else {
                set_light_uniforms(shader_texture_lights.uniforms, shape.lighting);
            }
        } else {
            // TODO see above
            warning_in_function_once("custom_shader: lighting currently not supported");
        }
    }
    /* handle texture changes */
    if (shape.texture_id != frame_state_cache.cached_texture_id) {
        frame_state_cache.cached_texture_id = shape.texture_id;
        if (frame_state_cache.cached_texture_id != TEXTURE_NONE) {
            PGraphicsOpenGL::OGL_bind_texture(frame_state_cache.cached_texture_id);
        }
    }
    /* handle vertex buffer binding + drawing */
    if (has_custom_vertex_buffer) {
        unbind_default_vertex_buffer();
        shape.vertex_buffer->draw();
        bind_default_vertex_buffer(); // OPTIMIZE this could be cached as well
    } else {
        // NOTE at this point there should be only either of two shapes groups:
        //      - filled shapes :: TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN
        //      - stroke shapes :: POINTS, LINES, LINE_STRIP, LINE_LOOP
        const uint32_t opengl_shape_mode = PGraphicsOpenGL::OGL_get_draw_mode(shape.mode);
        set_point_size_and_line_width(shape);
        /* adapt buffer size if necessary */
        const uint32_t vertex_count = shape.vertices.size();
        if (vertex_count > frame_state_cache.cached_max_vertices_per_batch) {
            frame_state_cache.cached_max_vertices_per_batch = vertex_count;
            frame_state_cache.cached_require_buffer_resize  = true;
        }
        /* draw vertex buffer */
        // NOTE `bind_default_vertex_buffer()` default VBO should always be bound at this point
        GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
        if (frame_state_cache.cached_require_buffer_resize) {
            frame_state_cache.cached_require_buffer_resize = false;
            GL_CALL(glBufferData(GL_ARRAY_BUFFER,
                                 static_cast<GLsizeiptr>(frame_state_cache.cached_max_vertices_per_batch * sizeof(Vertex)),
                                 nullptr,
                                 GL_DYNAMIC_DRAW));
        }
        GL_CALL(glBufferSubData(GL_ARRAY_BUFFER,
                                0,
                                static_cast<GLsizeiptr>(vertex_count * sizeof(Vertex)),
                                shape.vertices.data()));
        GL_CALL(glDrawArrays(opengl_shape_mode, 0, static_cast<GLsizei>(vertex_count)));
        // NOTE `unbind_default_vertex_buffer()` default VBO should always be bound at this point
    }
}
} // namespace umfeld