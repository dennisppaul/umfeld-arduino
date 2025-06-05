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
#include "PGraphicsDefault2D.h"

namespace umfeld {
    // TODO Ref https://github.com/libsdl-org/SDL/blob/main/docs/hello.c

    static SDL_Window*   window       = nullptr;
    static SDL_Renderer* sdl_renderer = nullptr;

    static void setup_pre() {
        int w = 0, h = 0;
        SDL_GetRenderOutputSize(sdl_renderer, &w, &h);
        umfeld::g->init(nullptr, w, h, 4, false);
    }

    static void setup_post() { printf("Setup Post\n"); }

    static void shutdown() { printf("Shutdown / TODO clean up window and renderer\n"); }

    static bool init() {
        SDL_WindowFlags flags = 0;
        if (!SDL_CreateWindowAndRenderer(DEFAULT_WINDOW_TITLE,
                                         static_cast<int>(umfeld::width),
                                         static_cast<int>(umfeld::height),
                                         get_SDL_WindowFlags(flags),
                                         &window,
                                         &sdl_renderer)) {
            return false;
        }

        console(format_label("renderer Name"), SDL_GetRendererName(sdl_renderer));
        console(format_label("renderer property"), SDL_GetRendererProperties(sdl_renderer));

        SDL_ShowWindow(window);
        return true;
    }

    static void draw_pre() {
        SDL_SetRenderScale(sdl_renderer, 1, 1);
    }

    static void draw_post() {
        SDL_RenderPresent(sdl_renderer);
    }

    static void set_flags(uint32_t& subsystem_flags) {
        subsystem_flags |= SDL_INIT_VIDEO;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    static void event(SDL_Event* event) {
        if (event->type == SDL_EVENT_WINDOW_RESIZED) {
            warning("TODO implement resize in RENDERER_SDL_2D");
        }
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    static void event_in_update_loop(SDL_Event* event) {
        if (event->type == SDL_EVENT_WINDOW_RESIZED) {
            warning("TODO implement resize in RENDERER_SDL_2D");
        }
    }

    static PGraphics* create_native_graphics(const bool render_to_offscreen) {
        (void) render_to_offscreen;
        return new PGraphicsDefault2D(sdl_renderer);
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    static void set_title(std::string& title) {
        if (window) {
            SDL_SetWindowTitle(window, title.c_str());
        }
    }

    static std::string get_title() {
        if (window) {
            std::string title = SDL_GetWindowTitle(window);
            return title;
        }
        return "";
    }

    static void set_window_position(const int x, const int y) {
        if (window == nullptr) {
            return;
        }
        SDL_SetWindowPosition(window, x, y);
    }

    static void get_window_position(int& x, int& y) {
        if (window == nullptr) {
            return;
        }
        SDL_GetWindowPosition(window, &x, &y);
    }

    static void set_window_size(const int width, const int height) {
        if (window == nullptr) {
            return;
        }
        SDL_SetWindowSize(window, width, height);
    }

    static void get_window_size(int& width, int& height) {
        if (window == nullptr) {
            return;
        }
        SDL_GetWindowSize(window, &width, &height);
    }

    static SDL_Window* get_sdl_window() {
        return window;
    }

    static void* get_renderer() {
        return sdl_renderer;
    }

    static int get_renderer_type() {
        return RENDERER_SDL_2D;
    }

    static const char* name() {
        return "SDL 2D";
    }
} // namespace umfeld

umfeld::SubsystemGraphics* umfeld_create_subsystem_graphics_sdl2d() {
    auto* graphics                   = new umfeld::SubsystemGraphics{};
    graphics->set_flags              = umfeld::set_flags;
    graphics->init                   = umfeld::init;
    graphics->setup_pre              = umfeld::setup_pre;
    graphics->setup_post             = umfeld::setup_post;
    graphics->draw_pre               = umfeld::draw_pre;
    graphics->draw_post              = umfeld::draw_post;
    graphics->shutdown               = umfeld::shutdown;
    graphics->create_native_graphics = umfeld::create_native_graphics;
    graphics->set_title              = umfeld::set_title;
    graphics->get_title              = umfeld::get_title;
    graphics->set_window_size        = umfeld::set_window_size;
    graphics->get_window_size        = umfeld::get_window_size;
    graphics->set_window_position    = umfeld::set_window_position;
    graphics->get_window_position    = umfeld::get_window_position;
    graphics->get_sdl_window         = umfeld::get_sdl_window;
    graphics->get_renderer           = umfeld::get_renderer;
    graphics->get_renderer_type      = umfeld::get_renderer_type;
    graphics->name                   = umfeld::name;
    return graphics;
}
