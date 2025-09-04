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

#include "SubsystemGraphicsOpenGL.h"
#include "PGraphicsOpenGL_2_0.h"

namespace umfeld::subsystem {
    static SDL_Window*   window     = nullptr;
    static SDL_GLContext gl_context = nullptr;

    static void draw_pre();
    static void draw_post();

    static bool init() {
        return OGL_init(window, gl_context, 2, 0, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    }

    static void setup_pre() {
        PGraphicsOpenGL::OGL_check_error("SUBSYSTEM_GRAPHICS_OPENGL20::setup_pre(begin)");
        OGL_setup_pre(window);
        PGraphicsOpenGL::OGL_check_error("SUBSYSTEM_GRAPHICS_OPENGL20::setup_pre(end)");
    }

    static void setup_post() {
        PGraphicsOpenGL::OGL_check_error("SUBSYSTEM_GRAPHICS_OPENGL20::setup_post(begin)");
        OGL_setup_post();
        OGL_draw_post(window, false); // TODO maybe move this to shared methods once it is fully integrated
        PGraphicsOpenGL::OGL_check_error("SUBSYSTEM_GRAPHICS_OPENGL20::setup_post(end)");
    }

    static void draw_pre() {
        PGraphicsOpenGL::OGL_check_error("SUBSYSTEM_GRAPHICS_OPENGL20::draw_pre(begin)");
        OGL_draw_pre();
        PGraphicsOpenGL::OGL_check_error("SUBSYSTEM_GRAPHICS_OPENGL20::draw_pre(end)");
    }

    static void draw_post() {
        PGraphicsOpenGL::OGL_check_error("SUBSYSTEM_GRAPHICS_OPENGL20::draw_post(begin)");
        OGL_draw_post(window, false);
        PGraphicsOpenGL::OGL_check_error("SUBSYSTEM_GRAPHICS_OPENGL20::draw_post(end)");
    }

    static void shutdown() {
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
    }

    static void set_flags(uint32_t& subsystem_flags) {
        subsystem_flags |= SDL_INIT_VIDEO;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    static void event(SDL_Event* event) {
        if (event->type == SDL_EVENT_WINDOW_RESIZED) {
            warning("TODO implement resize in OGLv20");
        }
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    static void event_in_update_loop(SDL_Event* event) {
        if (event->type == SDL_EVENT_WINDOW_RESIZED) {
            warning("TODO implement resize in OGLv20");
        }
    }

    static PGraphics* create_native_graphics(const bool render_to_offscreen) {
#ifdef OPENGL_2_0
        return new PGraphicsOpenGL_2_0(render_to_offscreen);
#else
        error("RENDERER_OPENGL_2_0 requires `OPENGL_2_0` to be defined. e.g `-DOPENGL_2_0` in CLI or `set(UMFELD_OPENGL_VERSION \"OPENGL_2_0\")` in `CMakeLists.txt`");
        return nullptr;
#endif
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    static void set_title(const std::string& title) {
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
        return gl_context;
    }

    static int get_renderer_type() {
        return RENDERER_OPENGL_2_0;
    }

    static const char* name() {
        return "OpenGL 2.0";
    }
} // namespace umfeld

umfeld::SubsystemGraphics* umfeld_create_subsystem_graphics_openglv20() {
    auto* graphics                   = new umfeld::SubsystemGraphics{};
    graphics->set_flags              = umfeld::subsystem::set_flags;
    graphics->init                   = umfeld::subsystem::init;
    graphics->setup_pre              = umfeld::subsystem::setup_pre;
    graphics->setup_post             = umfeld::subsystem::setup_post;
    graphics->draw_pre               = umfeld::subsystem::draw_pre;
    graphics->draw_post              = umfeld::subsystem::draw_post;
    graphics->shutdown               = umfeld::subsystem::shutdown;
    graphics->event                  = umfeld::subsystem::event;
    graphics->event_in_update_loop   = umfeld::subsystem::event_in_update_loop;
    graphics->create_native_graphics = umfeld::subsystem::create_native_graphics;
    graphics->set_title              = umfeld::subsystem::set_title;
    graphics->get_title              = umfeld::subsystem::get_title;
    graphics->set_window_size        = umfeld::subsystem::set_window_size;
    graphics->get_window_size        = umfeld::subsystem::get_window_size;
    graphics->set_window_position    = umfeld::subsystem::set_window_position;
    graphics->get_window_position    = umfeld::subsystem::get_window_position;
    graphics->get_sdl_window         = umfeld::subsystem::get_sdl_window;
    graphics->get_renderer           = umfeld::subsystem::get_renderer;
    graphics->get_renderer_type      = umfeld::subsystem::get_renderer_type;
    graphics->name                   = umfeld::subsystem::name;
    return graphics;
}

// TODO implement set_resizable and fullscreen() (at runtime) and is_minimized()
//    SDL_SetWindowFullscreen(window?!?, fullscreen_state ? SDL_WINDOW_FULLSCREEN: SDL_WINDOW_FULLSCREEN_DESKTOP);
//    SDL_SetWindowResizable(window?!?, SDL_TRUE);
//    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {}
