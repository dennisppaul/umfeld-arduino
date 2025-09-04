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
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#ifndef DISABLE_PDF
#include <cairo/cairo-pdf.h>
#endif // DISABLE_PDF

#include "Geometry.h"
#include "Vertex.h"

namespace umfeld {
    extern PGraphics* g;

    static int                 recording_exporter = OBJ;
    static std::string         recording_filename = "output.obj";
    static std::vector<Vertex> recorded_triangles;
    static std::vector<Vertex> recorded_lines;
    static bool                is_recording = false;
    static void (*_tmp_triangle_emitter_callback)(std::vector<Vertex>&){nullptr};
    static void (*_tmp_stroke_emitter_callback)(std::vector<Vertex>&, bool){nullptr};

#ifndef DISABLE_PDF
    inline void savePDF(const std::vector<Vertex>& triangles,
                        const std::vector<Vertex>& lines,
                        const std::string&         filename,
                        cairo_line_cap_t           stroke_cap = CAIRO_LINE_CAP_BUTT) {

        cairo_surface_t* surface = cairo_pdf_surface_create(filename.c_str(), g->width, g->height);
        cairo_t*         cr      = cairo_create(surface);

        cairo_set_line_width(cr, g->get_stroke_weight());
        cairo_set_line_cap(cr, stroke_cap);

        // Draw recorded_triangles
        for (size_t i = 0; i + 2 < triangles.size(); i += 3) {
            const Vertex& v0 = triangles[i];
            const Vertex& v1 = triangles[i + 1];
            const Vertex& v2 = triangles[i + 2];
            cairo_set_source_rgb(cr, v0.color.r, v0.color.g, v0.color.b);
            cairo_move_to(cr, v0.position.x, v0.position.y);
            cairo_line_to(cr, v1.position.x, v1.position.y);
            cairo_line_to(cr, v2.position.x, v2.position.y);
            cairo_close_path(cr);
            cairo_fill_preserve(cr);
            cairo_stroke(cr);
        }

        // Draw line strips
        for (size_t i = 1; i < lines.size(); ++i) {
            const Vertex& prev = lines[i - 1];
            const Vertex& curr = lines[i];

            if (curr.userdata == 1.0f || i == 1 || prev.userdata == 1.0f) {
                // start of new strip or skip to next valid segment
                continue;
            }

            cairo_set_source_rgb(cr, curr.color.r, curr.color.g, curr.color.b);
            cairo_move_to(cr, prev.position.x, prev.position.y);
            cairo_line_to(cr, curr.position.x, curr.position.y);
            cairo_stroke(cr);
        }

        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        console("PDF export complete: ", filename);
    }
#endif // DISABLE_PDF

    inline void saveOBJ(const std::vector<Vertex>& triangles,
                        const std::vector<Vertex>& lines,
                        const std::string&         filename) {
        std::ofstream file(filename);
        if (!file) {
            std::cerr << "Error: Unable to open file " << filename << "\n";
            return;
        }

        file << "# Exported 3D geometry to OBJ with Colors & TexCoords & Normals ";
        file << "recorded_triangles: " << triangles.size();
        file << " recorded_lines: " << lines.size();
        file << "\n";

        std::vector<Vertex> all_vertices;
        std::vector<int>    vertex_indices;

        // Function to add a vertex and return its index
        auto add_vertex = [&](const Vertex& v) -> int {
            all_vertices.push_back(v);
            return static_cast<int>(all_vertices.size()); // 1-based index
        };

        // Store vertices for recorded_triangles
        for (const auto& tri: triangles) {
            vertex_indices.push_back(add_vertex(tri));
        }

        // Store vertices for recorded_lines
        for (const auto& line_vertex: lines) {
            vertex_indices.push_back(add_vertex(line_vertex));
        }

        // Write all vertices (Position & Color)
        for (const auto& v: all_vertices) {
            file << "v " << v.position.x << " " << v.position.y << " " << v.position.z
                 << " " << v.color.r << " " << v.color.g << " " << v.color.b << "\n"; // Including color
        }

        // Write all normals
        for (const auto& v: all_vertices) {
            file << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";
        }

        // Write triangle faces with texture indices
        size_t index = 0;
        for ([[maybe_unused]] const auto& tri: triangles) {
            file << "f "
                 << (index + 1) << "/" << (index + 1) << "/" << (index + 1) << " "
                 << (index + 2) << "/" << (index + 2) << "/" << (index + 2) << " "
                 << (index + 3) << "/" << (index + 3) << "/" << (index + 3) << "\n";
            index += 3;
        }

        // Write recorded_lines (every two vertices form one line)
        const size_t line_start_index = triangles.size() + 1; // Start after triangle vertices
        for (size_t i = 0; i < lines.size(); i += 2) {
            file << "l " << (line_start_index + i) << " " << (line_start_index + i + 1) << "\n";
        }

        file.close();
        console("OBJ export complete: ", filename);
    }

    static glm::vec2 projectToPDF(const glm::vec3& point3D,
                                  const glm::mat4  mvp,
                                  float            surface_width,
                                  float            surface_height) {
        // glm::mat4 mvp  = proj * view * model;
        glm::vec4 clip = mvp * glm::vec4(point3D, 1.0f);

        if (clip.w == 0.0f) {
            return glm::vec2(-1.0f); // avoid divide-by-zero
        }

        glm::vec3 ndc = glm::vec3(clip) / clip.w;

        float x = (ndc.x * 0.5f + 0.5f) * surface_width;
        float y = (1.0f - (ndc.y * 0.5f + 0.5f)) * surface_height;

        return glm::vec2(x, y);
    }

    static void triangle_emitter_listener(std::vector<Vertex>& triangle_vertices) {
        std::vector<Vertex> transformed_triangles = triangle_vertices; // NOTE make copy
        if (recording_exporter == PDF) {
            const glm::mat4 mvp = g->projection_matrix * g->view_matrix * g->model_matrix;
            for (auto& p: transformed_triangles) {
                p.position = glm::vec4(projectToPDF(p.position, mvp, g->width, g->height), 0.0f, 1.0f);
            }
        }
        if (recording_exporter == OBJ) {
            const glm::mat4 modelview = g->model_matrix;
            for (auto& p: transformed_triangles) {
                p.position = glm::vec4(modelview * p.position);
            }
        }
        recorded_triangles.insert(recorded_triangles.end(), transformed_triangles.begin(), transformed_triangles.end());
    }

    static void stroke_emitter_listener(std::vector<Vertex>& line_strip_vertices, const bool line_strip_closed) {
        std::vector<Vertex> transformed_lines = line_strip_vertices; // NOTE make copy
        if (recording_exporter == PDF) {
            const glm::mat4 mvp = g->projection_matrix * g->view_matrix * g->model_matrix;
            for (auto& p: transformed_lines) {
                p.position = glm::vec4(projectToPDF(p.position, mvp, g->width, g->height), 0.0f, 1.0f);
            }
        }
        if (recording_exporter == OBJ) {
            const glm::mat4 modelview = g->model_matrix;
            for (auto& p: transformed_lines) {
                p.position = glm::vec4(modelview * p.position);
            }
        }

        for (size_t i = 0; i < transformed_lines.size() - 1; ++i) {
            recorded_lines.push_back(transformed_lines[i]);
            recorded_lines.push_back(transformed_lines[i + 1]);
        }
        if (line_strip_closed) {
            recorded_lines.push_back(transformed_lines.back());
            recorded_lines.push_back(transformed_lines.front());
        }
    }

    inline void beginRecord(const int exporter, const std::string& filename) {
        if (is_recording) {
            warning("already recording! Please stop the current recording before starting a new one.");
            return;
        }
        recording_exporter = exporter;
        recording_filename = filename;
        recorded_triangles.clear();
        recorded_lines.clear();
        _tmp_triangle_emitter_callback = g->get_triangle_emitter_callback();
        _tmp_stroke_emitter_callback   = g->get_stroke_emitter_callback();
        g->set_triangle_emitter_callback(triangle_emitter_listener);
        g->set_stroke_emitter_callback(stroke_emitter_listener);
        is_recording = true;
    }

    inline void endRecord() {
        is_recording = false;
        if (recording_exporter == PDF) {
#ifndef DISABLE_PDF
            savePDF(recorded_triangles, recorded_lines, recording_filename);
#else
            error("PDF export is disabled. Please enable it in the build settings.");
#endif // DISABLE_PDF
        } else if (recording_exporter == OBJ) {
            saveOBJ(recorded_triangles, recorded_lines, recording_filename);
        } else {
            error("Unknown exporter type: ", recording_exporter);
        }
        recorded_triangles.clear();
        recorded_lines.clear();
        g->set_triangle_emitter_callback(_tmp_triangle_emitter_callback);
        g->set_stroke_emitter_callback(_tmp_stroke_emitter_callback);
    }
} // namespace umfeld
