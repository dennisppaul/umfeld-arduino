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

#include <SDL3/SDL.h>

#include "Umfeld.h"

namespace umfeld::subsystem {

    static std::vector<LibraryListener*> _listeners;

    void register_library(LibraryListener* listener) {
        if (listener != nullptr) {
            _listeners.push_back(listener);
        }
    }

    void unregister_library(const LibraryListener* listener) {
        if (listener != nullptr) {
            _listeners.erase(std::find(_listeners.begin(), _listeners.end(), listener));
        }
    }

    static void shutdown() {
        for (const auto l: _listeners) {
            if (l != nullptr) {
                l->shutdown();
            }
        }
    }

    static void set_flags(uint32_t& subsystem_flags) {
        subsystem_flags |= SDL_INIT_EVENTS;
    }

    static void setup_pre() {
        for (const auto l: _listeners) {
            if (l != nullptr) {
        warning("setup_pre");
                l->setup_pre();
            }
        }
    }

    static void setup_post() {
        for (const auto l: _listeners) {
            if (l != nullptr) {
                l->setup_post();
            }
        }
    }

    static void draw_pre() {
        for (const auto l: _listeners) {
            if (l != nullptr) {
                l->draw_pre();
            }
        }
    }

    static void draw_post() {
        for (const auto l: _listeners) {
            if (l != nullptr) {
                l->draw_post();
            }
        }
    }

    static void event(SDL_Event* event) {
        for (const auto l: _listeners) {
            if (l != nullptr) {
                l->event(event);
            }
        }
    }

    static void event_in_update_loop(SDL_Event* event) {
        for (const auto l: _listeners) {
            if (l != nullptr) {
                l->event_in_update_loop(event);
            }
        }
    }

    static const char* name() {
        return "Client Libraries";
    }
} // namespace umfeld::subsystem

umfeld::SubsystemLibraries* umfeld_create_subsystem_libraries() {
    auto* libraries                 = new umfeld::SubsystemLibraries{};
    libraries->shutdown             = umfeld::subsystem::shutdown;
    libraries->set_flags            = umfeld::subsystem::set_flags;
    libraries->setup_pre            = umfeld::subsystem::setup_pre;
    libraries->setup_post           = umfeld::subsystem::setup_post;
    libraries->draw_pre             = umfeld::subsystem::draw_pre;
    libraries->draw_post            = umfeld::subsystem::draw_post;
    libraries->event                = umfeld::subsystem::event;
    libraries->event_in_update_loop = umfeld::subsystem::event_in_update_loop;
    libraries->name                 = umfeld::subsystem::name;
    libraries->register_library     = umfeld::subsystem::register_library;
    libraries->unregister_library   = umfeld::subsystem::unregister_library;
    return libraries;
}
