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

#include <SDL3/SDL.h>

#include "PGraphics.h"
#include "PAudio.h"


namespace umfeld {

    namespace subsystem::graphics_terminal {
        // NOTE expose this function to allow setting debounce interval from example application in terminal renderer
        void set_debounce_interval(int interval);
    } // namespace subsystem::graphics_terminal

    struct Subsystem {
        void (*set_flags)(uint32_t& subsystem_flags);
        bool (*init)();
        void (*setup_pre)();
        void (*setup_post)();
        void (*update_loop)(); /* higher frequency update loop but on same thread as draw, called before each draw */
        void (*draw_pre)();
        void (*draw_post)();
        void (*shutdown)();
        void (*event)(SDL_Event* event);
        void (*event_in_update_loop)(SDL_Event* event);
        const char* (*name)();
    };

    struct SubsystemGraphics : Subsystem {
        PGraphics* (*create_native_graphics)(bool render_to_offscreen);
        void (*post)(); // TODO maybe remove this, as there is also a `draw_post()` method
        void (*set_title)(const std::string& title);
        std::string (*get_title)();
        void (*set_window_position)(int x, int y);
        void (*get_window_position)(int& x, int& y);
        void (*set_window_size)(int width, int height);
        void (*get_window_size)(int& width, int& height);
        SDL_Window* (*get_sdl_window)();
        void* (*get_renderer)();
        int (*get_renderer_type)();
    };

    struct SubsystemAudio : Subsystem {
        void (*start)(PAudio* device);
        void (*stop)(PAudio* device);
        PAudio* (*create_audio)(const AudioUnitInfo* device_info);
    };

    struct SubsystemLibraries : Subsystem {
        void (*register_library)(LibraryListener* listener);
        void (*unregister_library)(const LibraryListener* listener);
    };

    /**
    * this function pointer is used to create an audio subsystem.
    *
    * it can be used e.g like this:
    *
    *     create_subsystem_audio = []() -> SubsystemAudio* {
    *         return umfeld_subsystem_audio_create_sdl();
    *     };
    *
    * or like this:
    *
    *     SubsystemAudio* create_sdl_audio() {
    *         return umfeld_subsystem_audio_create_sdl(); // <<< example implementation
    *     }
    *
    *     void settings() {
    *        create_subsystem_audio = create_sdl_audio;
    *        ...
    *     }
    */
    inline SubsystemAudio* (*create_subsystem_audio)()       = nullptr;
    inline SubsystemGraphics* (*create_subsystem_graphics)() = nullptr; // TODO this is redundant with `subsystem_graphics` in `Umfeld.h`, maybe remove this?

    class LibraryListener {
    public:
        virtual ~LibraryListener() = default;
        // TODO maybe add `loop()` aka `update()`
        virtual void setup_pre()                            = 0;
        virtual void setup_post()                           = 0;
        virtual void update_loop()                          = 0;
        virtual void draw_pre()                             = 0;
        virtual void draw_post()                            = 0;
        virtual void event(SDL_Event* event)                = 0; // NOTE events maybe handled in own thread
        virtual void event_in_update_loop(SDL_Event* event) = 0; // NOTE events are handled in the main loop
        virtual void shutdown()                             = 0;
    };
} // namespace umfeld

/* implemented subsystems */

umfeld::SubsystemGraphics*  umfeld_create_subsystem_graphics_sdl2d();
umfeld::SubsystemGraphics*  umfeld_create_subsystem_graphics_openglv20();
umfeld::SubsystemGraphics*  umfeld_create_subsystem_graphics_openglves30();
umfeld::SubsystemGraphics*  umfeld_create_subsystem_graphics_openglv33();
umfeld::SubsystemAudio*     umfeld_create_subsystem_audio_sdl();
umfeld::SubsystemAudio*     umfeld_create_subsystem_audio_portaudio();
umfeld::Subsystem*          umfeld_create_subsystem_hid();
umfeld::SubsystemLibraries* umfeld_create_subsystem_libraries();
