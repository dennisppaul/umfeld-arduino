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

namespace umfeld::subsystem {

    /* --- Subsystem --- */

    static void set_flags(uint32_t& subsystem_flags) {}

    static bool init() { return true; }

    static void setup_pre() {
        if (g == nullptr) { return; }
        g->init(nullptr, width, height);
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

    class PGraphicsStub final : public PGraphics {};

    static PGraphics* create_native_graphics(const bool render_to_offscreen) {
        return new PGraphicsStub();
    }

    // static void post() {}

    static void set_title(const std::string& title) {}

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
    graphics->set_flags              = umfeld::subsystem::set_flags;
    graphics->init                   = umfeld::subsystem::init;
    graphics->setup_pre              = umfeld::subsystem::setup_pre;
    graphics->setup_post             = umfeld::subsystem::setup_post;
    graphics->update_loop            = umfeld::subsystem::update_loop;
    graphics->draw_pre               = umfeld::subsystem::draw_pre;
    graphics->draw_post              = umfeld::subsystem::draw_post;
    graphics->shutdown               = umfeld::subsystem::shutdown;
    graphics->event                  = umfeld::subsystem::event;
    graphics->event_in_update_loop   = umfeld::subsystem::event_in_update_loop;
    graphics->name                   = umfeld::subsystem::name;
    graphics->create_native_graphics = umfeld::subsystem::create_native_graphics;
    // graphics->post                   = umfeld::subsystem::post; // TODO maybe remove this, as there is also a `draw_post()` method
    graphics->set_title              = umfeld::subsystem::set_title;
    graphics->get_title              = umfeld::subsystem::get_title;
    graphics->set_window_size        = umfeld::subsystem::set_window_size;
    graphics->get_window_size        = umfeld::subsystem::get_window_size;
    graphics->set_window_position    = umfeld::subsystem::set_window_position;
    graphics->get_window_position    = umfeld::subsystem::get_window_position;
    graphics->get_sdl_window         = umfeld::subsystem::get_sdl_window;
    graphics->get_renderer           = umfeld::subsystem::get_renderer;
    graphics->get_renderer_type      = umfeld::subsystem::get_renderer_type;
    return graphics;
}
