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
#include "ShapeRendererOpenGL_3.h"
#include "Geometry.h"

namespace umfeld {
    void ShapeRendererOpenGL_3::init(PGraphics* g, const std::vector<int> shader_programs) {
        graphics = g;
        initShaders(shader_programs);
        initBuffers();
    }

    void ShapeRendererOpenGL_3::submitShape(Shape& s) {
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
        shapes.push_back(std::move(s));
    }

    int ShapeRendererOpenGL_3::set_texture(PImage* img) {
        // NOTE not un-/binding any textures here.
        //      this also means that custom shapes ( e.g VertexBuffer/Mesh )
        //      would need to manually bind textures before rendering
        if (img == nullptr) {
            PGraphicsOpenGL::OGL_bind_texture(TEXTURE_NONE);
            return TEXTURE_NONE;
        }

        if (img->texture_id == TEXTURE_NOT_GENERATED) {
            const bool success = PGraphicsOpenGL::OGL_generate_and_upload_image_as_texture(img);
            if (!success || img->texture_id == TEXTURE_NOT_GENERATED) {
                error_in_function("cannot create texture from image.");
                PGraphicsOpenGL::OGL_bind_texture(TEXTURE_NONE);
                return TEXTURE_NONE;
            }
        }

        PGraphicsOpenGL::OGL_bind_texture(img->texture_id);
        return img->texture_id;
    }

    void ShapeRendererOpenGL_3::reset_flush_frame() {
        const size_t current_size = shapes.size();
        shapes.clear();
        shapes.reserve(current_size);
        initialized_vbo_buffer         = false;
        max_vertices_per_batch         = 0;
        frame_light_shapes_count       = 0;
        frame_transparent_shapes_count = 0;
        frame_opaque_shapes_count      = 0;
    }

    void ShapeRendererOpenGL_3::print_frame_info(const std::vector<Shape>& processed_point_shapes, const std::vector<Shape>& processed_line_shapes, const std::vector<Shape>& processed_triangle_shapes) const {
        // console("\n\t",
        //         "-----------------------", "\n\t",
        //         "FRAME_INFO", "\n\t",
        //         "-----------------------", "\n\t",
        //         "SHAPES SUBMITTED     ", "\n\t",
        //         "opaque_shapes      : ", frame_opaque_shapes_count, "\n\t",
        //         "light_shapes       : ", frame_light_shapes_count, "\n\t",
        //         "transparent_shapes : ", frame_transparent_shapes_count, "\n\t",
        //         "-----------------------", "\n\t",
        //         "SHAPES PROCESSED     ", "\n\t",
        //         "point_shapes       : ", processed_point_shapes.size(), "\n\t",
        //         "line_shapes        : ", processed_line_shapes.size(), "\n\t",
        //         "triangle_shapes    : ", processed_triangle_shapes.size(), "\n\t",
        //         "-----------------------");
        static constexpr int format_gap = 22;
        console("----------------------------");
        console("FRAME_INFO");
        console("----------------------------");
        console("SHAPES SUBMITTED");
        console(format_label("opaque_shapes", format_gap), frame_opaque_shapes_count);
        console(format_label("light_shapes", format_gap), frame_light_shapes_count);
        console(format_label("transparent_shapes", format_gap), frame_transparent_shapes_count);
        console("----------------------------");
        console("SHAPES PROCESSED");
        console(format_label("point_shapes", format_gap), processed_point_shapes.size());
        console(format_label("line_shapes", format_gap), processed_line_shapes.size());
        console(format_label("triangle_shapes", format_gap), processed_triangle_shapes.size());
        console("----------------------------");
    }
    void ShapeRendererOpenGL_3::flush(const glm::mat4& view_matrix, const glm::mat4& projection_matrix) {
        if (shapes.empty() || graphics == nullptr) {
            reset_flush_frame();
            return;
        }

        /*
         * render_pipeline ( render modes: sort_by_z_order, submission_order, immediately )
         *
         * ├── opaque shapes
         * │   └── batched by texture IDs ( including solid color )
         * ├── lighting shapes ( NOTE opaque + transparent z-order or submission )
         * │   └── batched by texture IDs ( including solid color )
         * ├── point-shader shapes ( TODO implement )
         * ├── line-shader shapes ( TODO implement )
         * └── transparent shapes ( sorted by z-order or submission  )
         *     └── batched by texture IDs ( including solid color )
         */

        std::vector<Shape> processed_point_shapes;
        processed_point_shapes.reserve(shapes.size());
        std::vector<Shape> processed_line_shapes;
        processed_line_shapes.reserve(shapes.size());
        std::vector<Shape> processed_triangle_shapes;
        processed_triangle_shapes.reserve(shapes.size());
        process_shapes(processed_point_shapes, processed_line_shapes, processed_triangle_shapes);
        flush_processed_shapes(processed_point_shapes,
                               processed_line_shapes,
                               processed_triangle_shapes,
                               view_matrix,
                               projection_matrix);

        run_once({ print_frame_info(processed_point_shapes, processed_line_shapes, processed_triangle_shapes); });

        // reserve capacity for next frame based on current usage
        reset_flush_frame();
    }

    static bool has_transparent_vertices(const std::vector<Vertex>& vertices) {
        for (auto& v: vertices) {
            if (v.color.a < 1.0f) {
                return true;
            }
        }
        return false;
    }

    // NOTE this is required by `process_shapes`
    void ShapeRendererOpenGL_3::handle_point_shape(std::vector<Shape>& processed_triangle_shapes, std::vector<Shape>& processed_point_shapes, Shape& point_shape) const {
        if (graphics->get_point_render_mode() == POINT_RENDER_MODE_TRIANGULATE) {
            std::vector<Vertex> triangulated_vertices = convertPointsToTriangles(point_shape.vertices, graphics->get_point_size());
            point_shape.vertices                      = std::move(triangulated_vertices);
            point_shape.filled                        = true;
            point_shape.mode                          = TRIANGLES;
            point_shape.transparent                   = has_transparent_vertices(point_shape.vertices) ? true : point_shape.texture_id != TEXTURE_NONE;
            processed_triangle_shapes.push_back(std::move(point_shape));
        } else if (graphics->get_point_render_mode() == POINT_RENDER_MODE_NATIVE) {
            // TODO handle this in extra path
            warning_in_function_once("TODO unsupported point render mode 'POINT_RENDER_MODE_NATIVE'");
            processed_point_shapes.push_back(std::move(point_shape));
        } else if (graphics->get_point_render_mode() == POINT_RENDER_MODE_SHADER) {
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
            warning_in_function_once("TODO unsupported point render mode 'POINT_RENDER_MODE_SHADER'");
            processed_point_shapes.push_back(std::move(point_shape));
        }
    }

    // NOTE this is required by `process_shapes`
    void ShapeRendererOpenGL_3::handle_stroke_shape(std::vector<Shape>& processed_triangle_shapes,
                                                    std::vector<Shape>& processed_line_shapes,
                                                    Shape&              stroke_shape) const {
        /* convert stroke shape to line strips */
        const bool         shape_has_transparent_vertices = has_transparent_vertices(stroke_shape.vertices);
        std::vector<Shape> converted_shapes;
        converted_shapes.reserve(stroke_shape.vertices.size());
        PGraphics::convert_stroke_shape_to_line_strip(stroke_shape, converted_shapes);
        /* triangulate line strips */
        if (!converted_shapes.empty()) {
            if (graphics->get_stroke_render_mode() == STROKE_RENDER_MODE_TRIANGULATE_2D) {
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
            } else if (graphics->get_stroke_render_mode() == STROKE_RENDER_MODE_TUBE_3D) {
                for (auto& cs: converted_shapes) {
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
            } else if (graphics->get_stroke_render_mode() == STROKE_RENDER_MODE_NATIVE) {
                for (auto& cs: converted_shapes) {
                    // shader_fill_texture->use();
                    // OGL3_render_vertex_buffer(vertex_buffer, GL_LINE_STRIP, line_strip_vertices);
                    // TODO close each shape by appending the last first vertex to last
                    processed_line_shapes.push_back(std::move(cs));
                }
                warning_in_function_once("unsupported stroke render mode 'STROKE_RENDER_MODE_NATIVE'");
            } else if (graphics->get_stroke_render_mode() == STROKE_RENDER_MODE_LINE_SHADER) {
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
            } else if (graphics->get_stroke_render_mode() == STROKE_RENDER_MODE_BARYCENTRIC_SHADER) {
                for (auto& cs: converted_shapes) {
                    processed_line_shapes.push_back(std::move(cs));
                }
                warning_in_function_once("unsupported stroke render mode 'STROKE_RENDER_MODE_BARYCENTRIC_SHADER'");
            } else if (graphics->get_stroke_render_mode() == STROKE_RENDER_MODE_GEOMETRY_SHADER) {
                for (auto& cs: converted_shapes) {
                    processed_line_shapes.push_back(std::move(cs));
                }
                warning_in_function_once("unsupported stroke render mode 'STROKE_RENDER_MODE_GEOMETRY_SHADER'");
            }
        }
    }

    void ShapeRendererOpenGL_3::setup_uniform_blocks(const std::string& shader_name, const GLuint program) {
        // NOTE uniform blocks are only setup for built-in shaders
        //      custom shaders must setup uniform blocks manually
        const GLuint blockIndex = glGetUniformBlockIndex(program, "Transforms");
        if (blockIndex == GL_INVALID_INDEX) {
            warning(shader_name, ": block uniform 'Transforms' not found");
        } else {
            glUniformBlockBinding(program, blockIndex, 0);
        }
    }

    bool ShapeRendererOpenGL_3::evaluate_shader_uniforms(const std::string& shader_name, const ShaderUniforms& uniforms) {
        bool valid = true;

        if (uniforms.uViewProj == ShaderUniforms::NOT_FOUND) {
            warning(shader_name, ": uniform 'uViewProj' not found");
            valid = false;
        }

        // only check uTexture for texture shaders
        if (shader_name.find("texture") != std::string::npos) {
            if (uniforms.uTexture == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'uTexture' not found");
                valid = false;
            }
        }

        // only check lighting uniforms for lighting shaders
        if (shader_name.find("lights") != std::string::npos) {
            if (uniforms.uView == ShaderUniforms::NOT_FOUND) {
                warning(shader_name, ": uniform 'uView' not found");
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

    void ShapeRendererOpenGL_3::initShaders(const std::vector<int>& shader_programm_id) {
        // NOTE for OpenGL ES 3.0 create shader source with dynamic array size
        //      ```c
        //      std::string transformsDefine = "#define MAX_TRANSFORMS " + std::to_string(MAX_TRANSFORMS) + "\n";
        //      const auto texturedVS = transformsDefine + R"(#version 330 core
        //      ```

        shader_programm_color          = shader_programm_id[SHADER_PROGRAM_COLOR];
        shader_programm_texture        = shader_programm_id[SHADER_PROGRAM_TEXTURE];
        shader_programm_color_lights   = shader_programm_id[SHADER_PROGRAM_COLOR_LIGHTS];
        shader_programm_texture_lights = shader_programm_id[SHADER_PROGRAM_TEXTURE_LIGHTS];
        // TODO implement
        //      point_shader_program              = shader_programm_id[SHADER_PROGRAM_POINT];
        //      line_shader_program               = shader_programm_id[SHADER_PROGRAM_LINE];

        setup_uniform_blocks("color", shader_programm_color);
        setup_uniform_blocks("texture", shader_programm_texture);
        setup_uniform_blocks("color_lights", shader_programm_color_lights);
        setup_uniform_blocks("texture_lights", shader_programm_texture_lights);

        // cache uniform locations
        shader_uniforms_color.uViewProj = glGetUniformLocation(shader_programm_color, "uViewProj");
        evaluate_shader_uniforms("color", shader_uniforms_color);

        shader_uniforms_texture.uViewProj = glGetUniformLocation(shader_programm_texture, "uViewProj");
        shader_uniforms_texture.uTexture  = glGetUniformLocation(shader_programm_texture, "uTexture");
        evaluate_shader_uniforms("texture", shader_uniforms_texture);

        // cache lighting shader uniform locations
        // shader_uniforms_color_lights.normalMatrix  = glGetUniformLocation(shader_programm_color_lights, "normalMatrix"); // TODO "normalMatrix as Transform"
        shader_uniforms_color_lights.uViewProj     = glGetUniformLocation(shader_programm_color_lights, "uViewProj");
        shader_uniforms_color_lights.uView         = glGetUniformLocation(shader_programm_color_lights, "uView");
        shader_uniforms_color_lights.ambient       = glGetUniformLocation(shader_programm_color_lights, "ambient");
        shader_uniforms_color_lights.specular      = glGetUniformLocation(shader_programm_color_lights, "specular");
        shader_uniforms_color_lights.emissive      = glGetUniformLocation(shader_programm_color_lights, "emissive");
        shader_uniforms_color_lights.shininess     = glGetUniformLocation(shader_programm_color_lights, "shininess");
        shader_uniforms_color_lights.lightCount    = glGetUniformLocation(shader_programm_color_lights, "lightCount");
        shader_uniforms_color_lights.lightPosition = glGetUniformLocation(shader_programm_color_lights, "lightPosition");
        shader_uniforms_color_lights.lightNormal   = glGetUniformLocation(shader_programm_color_lights, "lightNormal");
        shader_uniforms_color_lights.lightAmbient  = glGetUniformLocation(shader_programm_color_lights, "lightAmbient");
        shader_uniforms_color_lights.lightDiffuse  = glGetUniformLocation(shader_programm_color_lights, "lightDiffuse");
        shader_uniforms_color_lights.lightSpecular = glGetUniformLocation(shader_programm_color_lights, "lightSpecular");
        shader_uniforms_color_lights.lightFalloff  = glGetUniformLocation(shader_programm_color_lights, "lightFalloff");
        shader_uniforms_color_lights.lightSpot     = glGetUniformLocation(shader_programm_color_lights, "lightSpot");
        evaluate_shader_uniforms("color_lights", shader_uniforms_color_lights);

        // shader_uniforms_texture_lights.normalMatrix  = glGetUniformLocation(shader_programm_texture_lights, "normalMatrix"); // TODO "normalMatrix as Transform"
        shader_uniforms_texture_lights.uTexture      = glGetUniformLocation(shader_programm_texture_lights, "uTexture");
        shader_uniforms_texture_lights.uViewProj     = glGetUniformLocation(shader_programm_texture_lights, "uViewProj");
        shader_uniforms_texture_lights.uView         = glGetUniformLocation(shader_programm_texture_lights, "uView");
        shader_uniforms_texture_lights.ambient       = glGetUniformLocation(shader_programm_texture_lights, "ambient");
        shader_uniforms_texture_lights.specular      = glGetUniformLocation(shader_programm_texture_lights, "specular");
        shader_uniforms_texture_lights.emissive      = glGetUniformLocation(shader_programm_texture_lights, "emissive");
        shader_uniforms_texture_lights.shininess     = glGetUniformLocation(shader_programm_texture_lights, "shininess");
        shader_uniforms_texture_lights.lightCount    = glGetUniformLocation(shader_programm_texture_lights, "lightCount");
        shader_uniforms_texture_lights.lightPosition = glGetUniformLocation(shader_programm_texture_lights, "lightPosition");
        shader_uniforms_texture_lights.lightNormal   = glGetUniformLocation(shader_programm_texture_lights, "lightNormal");
        shader_uniforms_texture_lights.lightAmbient  = glGetUniformLocation(shader_programm_texture_lights, "lightAmbient");
        shader_uniforms_texture_lights.lightDiffuse  = glGetUniformLocation(shader_programm_texture_lights, "lightDiffuse");
        shader_uniforms_texture_lights.lightSpecular = glGetUniformLocation(shader_programm_texture_lights, "lightSpecular");
        shader_uniforms_texture_lights.lightFalloff  = glGetUniformLocation(shader_programm_texture_lights, "lightFalloff");
        shader_uniforms_texture_lights.lightSpot     = glGetUniformLocation(shader_programm_texture_lights, "lightSpot");
        evaluate_shader_uniforms("texture_lights", shader_uniforms_texture_lights);
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

        // pre-allocate frame buffers
        flush_frame_matrices.reserve(MAX_TRANSFORMS);
    }

    size_t ShapeRendererOpenGL_3::estimate_triangle_count(const Shape& s) {
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

    void ShapeRendererOpenGL_3::convert_shapes_to_triangles(const Shape& s, std::vector<Vertex>& out, const uint16_t transformID) {
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

    void ShapeRendererOpenGL_3::render_batch(const std::vector<Shape*>& shapes_to_render) {
        // NOTE assumes that shader is already in use and texture is already bound

        if (shapes_to_render.empty()) {
            return;
        }

        // process in chunks to respect MAX_TRANSFORMS limit
        for (size_t offset = 0; offset < shapes_to_render.size(); offset += MAX_TRANSFORMS) {
            const size_t chunkSize = std::min(static_cast<size_t>(MAX_TRANSFORMS), shapes_to_render.size() - offset);

            // upload transforms for this chunk
            flush_frame_matrices.clear();
            flush_frame_matrices.reserve(chunkSize);
            for (size_t i = 0; i < chunkSize; ++i) {
                flush_frame_matrices.push_back(shapes_to_render[offset + i]->model);
            }

            glBindBuffer(GL_UNIFORM_BUFFER, ubo);
            glBufferSubData(GL_UNIFORM_BUFFER, 0,
                            static_cast<GLsizeiptr>(flush_frame_matrices.size() * sizeof(glm::mat4)),
                            flush_frame_matrices.data());

            // estimate and reserve vertex space
            flush_frame_vertices.clear();
            size_t totalEstimate = 0;
            for (size_t i = 0; i < chunkSize; ++i) {
                totalEstimate += estimate_triangle_count(*shapes_to_render[offset + i]);
            }
            flush_frame_vertices.reserve(totalEstimate);

            for (size_t i = 0; i < chunkSize; ++i) {
                const auto* s = shapes_to_render[offset + i];
                if (s->light_enabled) {
                    if (s->texture_id == TEXTURE_NONE) {
                        set_light_uniforms(shader_uniforms_color_lights, s->lighting);
                    } else {
                        set_light_uniforms(shader_uniforms_texture_lights, s->lighting);
                    }
                }
            }
            // tessellate shapes in this chunk
            for (size_t i = 0; i < chunkSize; ++i) {
                const auto* s = shapes_to_render[offset + i];
                convert_shapes_to_triangles(*s, flush_frame_vertices, static_cast<uint16_t>(i));
            }
            if (flush_frame_vertices.size() > max_vertices_per_batch) {
                // TODO ... ok an now what? add some *coping* strategy
                error("number of batch vertices exceeded buffer");
            } else if (!flush_frame_vertices.empty()) {
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                if (!initialized_vbo_buffer) {
                    initialized_vbo_buffer = true;
                    glBufferData(GL_ARRAY_BUFFER,
                                 static_cast<GLsizeiptr>(max_vertices_per_batch * sizeof(Vertex)),
                                 nullptr,
                                 GL_DYNAMIC_DRAW);
                }
                glBufferSubData(GL_ARRAY_BUFFER,
                                0, flush_frame_vertices.size() * sizeof(Vertex),
                                flush_frame_vertices.data());
                glDrawArrays(GL_TRIANGLES,
                             // GL_LINE_STRIP,
                             0,
                             static_cast<GLsizei>(flush_frame_vertices.size()));
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

    void ShapeRendererOpenGL_3::enable_depth_testing() const {
        if (graphics != nullptr) {
            if (graphics->hint_enable_depth_test) {
                glEnable(GL_DEPTH_TEST);
            } else {
                glDisable(GL_DEPTH_TEST);
            }
        } else {
            glEnable(GL_DEPTH_TEST);
        }
        glDepthFunc(GL_LEQUAL); // allow equal depths to pass ( `GL_LESS` is default )
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    void ShapeRendererOpenGL_3::disable_depth_testing() {
        glEnable(GL_BLEND);
        // TODO enable proper blend function
        //      also figure out if blending needs to happen for non transparent shapes e.g via forced transparency
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    }

    void ShapeRendererOpenGL_3::set_per_frame_shader_uniforms(const glm::mat4& view_projection_matrix,
                                                              const int        frame_has_light_shapes,
                                                              const int        frame_has_transparent_shapes,
                                                              const int        frame_has_opaque_shapes) const {
        // NOTE might set VP matrix or texture unit in all necessary shaders
        if (frame_has_opaque_shapes > 0 || frame_has_transparent_shapes > 0) {
            glUseProgram(shader_programm_color);
            glUniformMatrix4fv(shader_uniforms_color.uViewProj, 1, GL_FALSE, &view_projection_matrix[0][0]);

            glUseProgram(shader_programm_texture);
            glUniformMatrix4fv(shader_uniforms_texture.uViewProj, 1, GL_FALSE, &view_projection_matrix[0][0]);
            glUniform1i(shader_uniforms_texture.uTexture, 0);
        }
        if (frame_has_light_shapes > 0) {
            glUseProgram(shader_programm_color_lights);
            glUniformMatrix4fv(shader_uniforms_color_lights.uViewProj, 1, GL_FALSE, &view_projection_matrix[0][0]);

            glUseProgram(shader_programm_texture_lights);
            glUniformMatrix4fv(shader_uniforms_texture_lights.uViewProj, 1, GL_FALSE, &view_projection_matrix[0][0]);
            glUniform1i(shader_uniforms_texture_lights.uTexture, 0);
        }
    }

    void ShapeRendererOpenGL_3::enable_default_shader_and_bind_texture(const unsigned texture_id) const {
        if (texture_id == TEXTURE_NONE) {
            glUseProgram(shader_programm_color);
        } else {
            glUseProgram(shader_programm_texture);
            PGraphicsOpenGL::OGL_bind_texture(texture_id);
        }
    }

    void ShapeRendererOpenGL_3::enable_light_shader_and_bind_texture(const unsigned texture_id) const {
        if (texture_id == TEXTURE_NONE) {
            glUseProgram(shader_programm_color_lights);
        } else {
            glUseProgram(shader_programm_texture_lights);
            PGraphicsOpenGL::OGL_bind_texture(texture_id);
        }
    }

    /**
     * render shapes in batches (preprocess).
     *
     * - preprocess shapes into texture batches for transparen, opaque, and transparent shapes.
     * - sort transparent shape by z-order.
     * render light shapes without transparency similar to opaque shapes.
     *
     * @param shapes
     * @param view_matrix
     * @param projection_matrix
     */
    void ShapeRendererOpenGL_3::flush_sort_by_z_order(std::vector<Shape>& shapes,
                                                      const glm::mat4&    view_matrix,
                                                      const glm::mat4&    projection_matrix) {
        if (shapes.empty()) { return; }

        const glm::mat4 view_projection_matrix = projection_matrix * view_matrix;

        // use unordered_map for better performance with many textures
        std::unordered_map<GLuint, TextureBatch> texture_batches;
        // TODO make default number of textures dynamic depending on last frame
        texture_batches.reserve(DEFAULT_NUM_TEXTURES);

        for (auto& s: shapes) {
            TextureBatch& batch = texture_batches[s.texture_id];
            batch.texture_id    = s.texture_id;
            if (s.light_enabled) {
                batch.light_shapes.push_back(&s);
            } else {
                if (s.transparent) {
                    batch.transparent_shapes.push_back(&s);
                } else {
                    batch.opaque_shapes.push_back(&s);
                }
            }
            batch.max_vertices += s.vertices.size();
        }

        max_vertices_per_batch = 0;
        initialized_vbo_buffer = false;
        for (auto& [textureID, batch]: texture_batches) {
            if (batch.max_vertices > max_vertices_per_batch) {
                max_vertices_per_batch = batch.max_vertices;
            }
            // compute depth and sort transparents
            for (auto* s: batch.transparent_shapes) {
                const glm::vec4 center_world_space = s->model * glm::vec4(s->center_object_space, 1.0f);
                const glm::vec4 center_view_space  = view_projection_matrix * center_world_space;
                s->depth                           = center_view_space.z / center_view_space.w; // Proper NDC depth
            }
            std::sort(batch.transparent_shapes.begin(), batch.transparent_shapes.end(),
                      [](const Shape* A, const Shape* B) { return A->depth > B->depth; }); // Back to front
        }

        glBindVertexArray(vao);

        if (custom_shader == nullptr) {
            // NOTE set shader program uniforms ... only once
            set_per_frame_shader_uniforms(view_projection_matrix, frame_light_shapes_count, frame_transparent_shapes_count, frame_opaque_shapes_count);
        } else {
            warning_in_function_once("custom_shader: preset uniforms?");
        }
        // opaque pass
        if (frame_opaque_shapes_count > 0) {
            enable_depth_testing();
            warning_in_function_once("frame_opaque_shapes_count: ", frame_opaque_shapes_count);
            for (auto& [texture_id, batch]: texture_batches) {
                enable_default_shader_and_bind_texture(texture_id);
                render_batch(batch.opaque_shapes);
            }
        }
        // light pass
        if (frame_light_shapes_count > 0) {
            if (frame_opaque_shapes_count > 0) {
                enable_depth_testing();
            }
            warning_in_function_once("frame_light_shapes_count: ", frame_light_shapes_count);
            for (auto& [texture_id, batch]: texture_batches) {
                enable_light_shader_and_bind_texture(texture_id);
                render_batch(batch.light_shapes);
            }
        }
        // transparent pass
        if (frame_transparent_shapes_count > 0) {
            disable_depth_testing();
            warning_in_function_once("frame_transparent_shapes_count: ", frame_transparent_shapes_count);
            for (auto& [texture_id, batch]: texture_batches) {
                enable_default_shader_and_bind_texture(texture_id);
                render_batch(batch.transparent_shapes);
            }
        }

        glDepthMask(GL_TRUE);
        glBindVertexArray(0);
    }

    void ShapeRendererOpenGL_3::set_light_uniforms(const ShaderUniforms& uniforms, const LightingState& lighting) {
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

    /**
     * render shapes directly (no preprocess).
     *
     * - render shapes directly from submission order
     *
     * @param shapes
     * @param view_matrix
     * @param projection_matrix
     */
    void ShapeRendererOpenGL_3::flush_submission_order(std::vector<Shape>& shapes,
                                                       const glm::mat4&    view_matrix,
                                                       const glm::mat4&    projection_matrix) {
        const glm::mat4 view_projection_matrix = projection_matrix * view_matrix;

        glBindVertexArray(vao);

        // enable depth test for all shapes
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

        GLuint currentTexture = UINT32_MAX; // force initial texture bind
        bool   blendEnabled   = false;

        if (custom_shader == nullptr) {
            // NOTE set shader program uniforms ... only once
            set_per_frame_shader_uniforms(view_projection_matrix, frame_light_shapes_count, frame_transparent_shapes_count, frame_opaque_shapes_count);
        } else {
            warning_in_function_once("custom_shader: set shader uniforms once?");
            // TODO implement an option that TRIES to set the *known* uniforms:
            //      - texture unit
            //      - model, view, projection matrices
            //      - light uniforms
        }

        max_vertices_per_batch          = 0;
        initialized_vbo_buffer          = false;
        GLuint cached_shader_program_id = NO_SHADER_PROGRAM;

        // render each shape individually in submission order
        for (auto& shape: shapes) {
            // handle transparency state changes
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
            // switch program ... if necessary
            if (custom_shader == nullptr) {
                const GLuint required_shader_program_id = shape.light_enabled ? (shape.texture_id == TEXTURE_NONE ? shader_programm_color_lights : shader_programm_texture_lights) : (shape.texture_id == TEXTURE_NONE ? shader_programm_color : shader_programm_texture);
                if (required_shader_program_id != cached_shader_program_id) {
                    glUseProgram(required_shader_program_id);
                    cached_shader_program_id = required_shader_program_id;
                }
            } else {
                warning_in_function_once("custom_shader: set shader uniforms per shape");
                glUseProgram(custom_shader->get_program_id());
                // custom_shader->update_uniforms(shape);
                // TODO implement an option that TRIES to set the *known* uniforms:
                //      - model, view, projection matrices
                //      - light uniforms
            }
            // handle texture changes
            if (shape.texture_id != currentTexture) {
                currentTexture = shape.texture_id;
                if (currentTexture != TEXTURE_NONE) {
                    PGraphicsOpenGL::OGL_bind_texture(currentTexture);
                }
            }
            // adapt buffer size if necessary
            if (shape.vertices.size() > max_vertices_per_batch) {
                max_vertices_per_batch = shape.vertices.size();
                initialized_vbo_buffer = false;
            }
            // render single shape (create a vector with just this shape)
            std::vector single_shape = {&shape};
            TRACE_SCOPE_N("RENDER_BATCH_SUBMISION_ORDER");
            {
                render_batch(single_shape);
            }
        }

        // restore default state
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glBindVertexArray(0);
    }

    void ShapeRendererOpenGL_3::flush_immediately(std::vector<Shape>& shapes,
                                                  const glm::mat4&    view_matrix,
                                                  const glm::mat4&    projection_matrix) {
        flush_submission_order(shapes, view_matrix, projection_matrix);
    }

    void ShapeRendererOpenGL_3::flush_processed_shapes(const std::vector<Shape>& processed_point_shapes,
                                                       const std::vector<Shape>& processed_line_shapes,
                                                       std::vector<Shape>&       processed_triangle_shapes,
                                                       const glm::mat4&          view_matrix,
                                                       const glm::mat4&          projection_matrix) {
        if (!processed_point_shapes.empty() || !processed_line_shapes.empty()) {
            // TODO alternative render paths for points and lines are currently not implemented
            warning_in_function_once("TODO alternative render paths for points and lines are currently not implemented");
        }
        // NOTE the paths below ONLY render filled triangle shapes.
        if (graphics->get_render_mode() == RENDER_MODE_SORTED_BY_Z_ORDER) {
            console_once(format_label("render_mode"), "RENDER_MODE_SORTED_BY_Z_ORDER ( rendering shapes in z-order and in batches )");
            flush_sort_by_z_order(processed_triangle_shapes, view_matrix, projection_matrix);
        } else if (graphics->get_render_mode() == RENDER_MODE_SORTED_BY_SUBMISSION_ORDER) {
            console_once(format_label("render_mode"), "RENDER_MODE_SORTED_BY_SUBMISSION_ORDER ( rendering shapes in submission order )");
            flush_submission_order(processed_triangle_shapes, view_matrix, projection_matrix);
        } else if (graphics->get_render_mode() == RENDER_MODE_IMMEDIATELY) {
            console_once(format_label("render_mode"), "RENDER_MODE_IMMEDIATELY ( rendering shapes immediately )");
            flush_immediately(processed_triangle_shapes, view_matrix, projection_matrix);
        }
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
    void ShapeRendererOpenGL_3::process_shapes(std::vector<Shape>& processed_point_shapes,
                                               std::vector<Shape>& processed_line_shapes,
                                               std::vector<Shape>& processed_triangle_shapes) {
        // ReSharper disable once CppDFAConstantConditions
        if (shapes.empty() || graphics == nullptr) { return; }

        for (auto& s: shapes) {
            /* stroke shapes */
            if (!s.filled) {
                if (s.mode == POINTS) {
                    /* point shapes */
                    handle_point_shape(processed_triangle_shapes, processed_point_shapes, s);
                } else {
                    /* all other shapes */
                    handle_stroke_shape(processed_triangle_shapes, processed_line_shapes, s);
                }
                // NOTE `continue` prevents shapes that were converted to filled triangles
                //       from being added again as a filled shape.
                continue;
            }
            /* fill shapes */
            if (s.filled) {
                /* convert filled shapes to triangles */
                graphics->convert_fill_shape_to_triangles(s);
                s.mode = TRIANGLES;
                processed_triangle_shapes.push_back(std::move(s));
            }
        }
    }
} // namespace umfeld