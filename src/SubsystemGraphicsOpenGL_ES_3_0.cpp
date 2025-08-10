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
#include "PGraphicsOpenGL_3.h"

namespace umfeld {
    static SDL_Window*   window                                  = nullptr;
    static SDL_GLContext gl_context                              = nullptr;
    static bool          blit_framebuffer_object_to_screenbuffer = true; // NOTE FBO is BLITted directly into the color buffer instead of rendered with a textured quad

    static void draw_pre();
    static void draw_post();

    static bool init() {
        return OGL_init(window, gl_context, 3, 0, SDL_GL_CONTEXT_PROFILE_ES);
    }

    static void setup_pre() {
        checkOpenGLError("SUBSYSTEM_GRAPHICS_OPENGL_ES_3_0::setup_pre(begin)");
        OGL_setup_pre(window);
        checkOpenGLError("SUBSYSTEM_GRAPHICS_OPENGL_ES_3_0::setup_pre(end)");
    }

    static void setup_post() {
        checkOpenGLError("SUBSYSTEM_GRAPHICS_OPENGL_ES_3_0::setup_post(begin)");
        OGL_setup_post();
        OGL_draw_post(window, blit_framebuffer_object_to_screenbuffer); // TODO maybe move this to shared methods once it is fully integrated
        checkOpenGLError("SUBSYSTEM_GRAPHICS_OPENGL_ES_3_0::setup_post(end)");
    }

    static void draw_pre() {
        checkOpenGLError("SUBSYSTEM_GRAPHICS_OPENGL_ES_3_0::draw_pre(begin)");
        OGL_draw_pre();
        checkOpenGLError("SUBSYSTEM_GRAPHICS_OPENGL_ES_3_0::draw_pre(end)");
    }

    static void draw_post() {
        checkOpenGLError("SUBSYSTEM_GRAPHICS_OPENGL_ES_3_0::draw_post(begin)");
        OGL_draw_post(window, blit_framebuffer_object_to_screenbuffer);
        checkOpenGLError("SUBSYSTEM_GRAPHICS_OPENGL_ES_3_0::draw_post(end)");
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
            warning("TODO implement resize in OGLves30");
        }
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    static void event_in_update_loop(SDL_Event* event) {
        if (event->type == SDL_EVENT_WINDOW_RESIZED) {
            warning("TODO implement resize in OGLves30");
        }
    }

    static PGraphics* create_native_graphics(const bool render_to_offscreen) {
#ifdef OPENGL_ES_3_0
        return new PGraphicsOpenGL_3(render_to_offscreen);
#else
        error("RENDERER_OPENGL_ES_3_0 requires `OPENGL_ES_3_0` to be defined. e.g `-DOPENGL_ES_3_0` in CLI or `set(UMFELD_OPENGL_VERSION \"OPENGL_ES_3_0\")` in `CMakeLists.txt`");
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
        return RENDERER_OPENGL_ES_3_0;
    }

    static const char* name() {
        return "OpenGL ES 3.0";
    }
} // namespace umfeld

umfeld::SubsystemGraphics* umfeld_create_subsystem_graphics_openglves30() {
    auto* graphics                   = new umfeld::SubsystemGraphics{};
    graphics->set_flags              = umfeld::set_flags;
    graphics->init                   = umfeld::init;
    graphics->setup_pre              = umfeld::setup_pre;
    graphics->setup_post             = umfeld::setup_post;
    graphics->draw_pre               = umfeld::draw_pre;
    graphics->draw_post              = umfeld::draw_post;
    graphics->shutdown               = umfeld::shutdown;
    graphics->event                  = umfeld::event;
    graphics->event_in_update_loop   = umfeld::event_in_update_loop;
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
