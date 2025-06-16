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

#include "Umfeld.h"

namespace umfeld {

    /* --- Subsystem --- */

    static void set_flags(uint32_t& subsystem_flags) {}

    static bool init() { return true; }

    static void setup_pre() {
        if (g == nullptr) { return; }
        g->init(nullptr, width, height, false);
    }

    static void setup_post() {}

    static void update_loop() {}

    static void draw_pre() {}

    static void draw_post() {}

    static void shutdown() {}

    static void event(SDL_Event* event) {}

    static void event_in_update_loop(SDL_Event* event) {}

    static const char* name() { return "TEMPLATE"; }

    /* --- SubsystemGraphics --- */

    class PGraphicsStub final : public PGraphics {
    public:
        void IMPL_background(float a, float b, float c, float d) override {}
        void IMPL_bind_texture(int bind_texture_id) override {}
        void IMPL_set_texture(PImage* img) override {}
        void IMPL_emit_shape_fill_triangles(std::vector<Vertex>& triangle_vertices) override {}
        void IMPL_emit_shape_stroke_points(std::vector<Vertex>& point_vertices, float point_size) override {}
        void IMPL_emit_shape_stroke_line_strip(std::vector<Vertex>& line_strip_vertices, bool line_strip_closed) override {}
    };

    static PGraphics* create_native_graphics(const bool render_to_offscreen) {
        return new PGraphicsStub();
    }

    static void post() {}

    static void set_title(std::string& title) {}

    static std::string get_title() { return ""; }

    static void set_window_position(const int x, const int y) {}

    static void get_window_position(int& x, int& y) {}

    static void set_window_size(const int width, const int height) {}

    static void get_window_size(int& width, int& height) {}

    static SDL_Window* get_sdl_window() { return nullptr; }

    static void* get_renderer() { return nullptr; }

    static int get_renderer_type() { return RENDERER_TEMPLATE; }

} // namespace umfeld

umfeld::SubsystemGraphics* umfeld_create_subsystem_graphics_template() {
    auto* graphics                   = new umfeld::SubsystemGraphics{};
    graphics->set_flags              = umfeld::set_flags;
    graphics->init                   = umfeld::init;
    graphics->setup_pre              = umfeld::setup_pre;
    graphics->setup_post             = umfeld::setup_post;
    graphics->update_loop            = umfeld::update_loop;
    graphics->draw_pre               = umfeld::draw_pre;
    graphics->draw_post              = umfeld::draw_post;
    graphics->shutdown               = umfeld::shutdown;
    graphics->event                  = umfeld::event;
    graphics->event_in_update_loop   = umfeld::event_in_update_loop;
    graphics->name                   = umfeld::name;
    graphics->create_native_graphics = umfeld::create_native_graphics;
    graphics->post                   = umfeld::post; // TODO maybe remove this, as there is also a `draw_post()` method
    graphics->set_title              = umfeld::set_title;
    graphics->get_title              = umfeld::get_title;
    graphics->set_window_size        = umfeld::set_window_size;
    graphics->get_window_size        = umfeld::get_window_size;
    graphics->set_window_position    = umfeld::set_window_position;
    graphics->get_window_position    = umfeld::get_window_position;
    graphics->get_sdl_window         = umfeld::get_sdl_window;
    graphics->get_renderer           = umfeld::get_renderer;
    graphics->get_renderer_type      = umfeld::get_renderer_type;
    return graphics;
}
