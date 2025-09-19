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

#ifdef SYSTEM_WINDOWS
#include <pdcurses/curses.h>
extern MEVENT mouse_event;
#else
#include <curses.h>
#endif
#include <cmath>
#include <unistd.h>

#include "Umfeld.h"
#include "PGraphicsTerminal.h"

extern int ESCDELAY;

namespace umfeld::subsystem {

    [[maybe_unused]] static MEVENT curses_event;
    constexpr int                  NO_KEY_PRESSED    = -1;
    static int                     key_last_pressed  = NO_KEY_PRESSED;
    static int                     debounce_interval = 5;
    static int                     debounce_counter  = 0;

    // NOTE expose this function to allow setting debounce interval from example application in terminal renderer
    void graphics_terminal::set_debounce_interval(int interval) {
        debounce_interval = interval;
    }

    /* --- Subsystem --- */

    static void set_flags(uint32_t& subsystem_flags) {}

    static bool init() { return true; }

    static void setup_pre() {
        if (g == nullptr) { return; }
        g->init(nullptr, width, height);

        ESCDELAY = 25;
        initscr();
        noecho();
        cbreak();
        curs_set(0);
        keypad(stdscr, TRUE);
        timeout(50);

        // Enable mouse events
        // mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | REPORT_MOUSE_POSITION, NULL);
        mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);

        // Required in many terminals to receive mouse move events
        printf("\033[?1003h\n"); // Enable mouse tracking (Xterm)
        fflush(stdout);          // flush to apply

        if (has_colors()) { // TODO this should be handle in PGraphicsTerminal
            start_color();
            init_pair(1, COLOR_WHITE, COLOR_BLACK);
            attron(COLOR_PAIR(1));
        }

        // TODO ugly hack!!!
#ifdef SYSTEM_WINDOWS
        resize_term(0, 0);
#else
        resizeterm(0, 0);
#endif
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        width  = cols;
        height = rows;
        if (g != nullptr) {
            g->width  = width;
            g->height = height;
        }
    }

    static void setup_post() {}

    static void update_loop() {}

    static void draw_pre() {}

    static bool get_mouse_event(int ch, int& x, int& y) {
        if (ch != KEY_MOUSE) {
            return false;
        }
#ifdef SYSTEM_WINDOWS
        // TODO seems to be a bit broken
        if (getmouse() == 0) {
            return false;
        }
        x = mouse_event.x;
        y = mouse_event.y;
        return true;
#else
        MEVENT e;
        if (getmouse(&e) != OK) {
            return false;
        }
        x = e.x;
        y = e.y;
        return true;
#endif
    }

    static void draw_post() {
        if (g != nullptr) {
            g->endDraw();
        }

        // int rows, cols;
        // getmaxyx(stdscr, rows, cols);
        // mvaddstr(3, 0, to_string(rows, ", ", cols).c_str());

        const int ch = getch();

        if (ch != KEY_MOUSE) {
            int key_ch = ch;
            key_ch     = key_ch <= NO_KEY_PRESSED ? NO_KEY_PRESSED : key_ch;
            key_ch     = key_ch >= KEY_MIN ? NO_KEY_PRESSED : key_ch;
            if (key_ch != NO_KEY_PRESSED) {
                if (!isKeyPressed) {
                    key          = key_ch;
                    isKeyPressed = true;
                    run_keyPressed_callback();
                    debounce_counter = debounce_interval;
                }
                key_last_pressed = key_ch;
            } else if (isKeyPressed && key_last_pressed != NO_KEY_PRESSED) {
                if (debounce_counter <= 0) {
                    isKeyPressed = false;
                    run_keyReleased_callback();
                    key_last_pressed = NO_KEY_PRESSED;
                }
            }
        }
        if ((ch == KEY_MOUSE || ch == NO_KEY_PRESSED) && debounce_counter > 0) {
            debounce_counter--;
        }

        int mx, my;
        if (get_mouse_event(ch, mx, my)) {
            mouseX = static_cast<float>(mx);
            mouseY = static_cast<float>(my);
        }

        if (use_esc_key_to_quit) {
            if (ch == 27) {
                request_shutdown = true;
            }
        }
    }

    static void shutdown() {
        printf("\033[?1003l\n"); // Disable mouse tracking
        fflush(stdout);
#ifndef SYSTEM_WINDOWS
        endwin();
#endif
        console("terminal graphics shutdown.");
    }

    static void event(SDL_Event* event) {}

    static void event_in_update_loop(SDL_Event* event) {}

    static const char* name() { return "TERMINAL"; }

    /* --- SubsystemGraphics --- */

    static PGraphics* create_native_graphics(const bool render_to_offscreen) {
        return new PGraphicsTerminal();
    }

    static void post() {}

    static void set_title(const std::string& title) {}

    static std::string get_title() { return ""; }

    static void set_window_position(const int x, const int y) {}

    static void get_window_position(int& x, int& y) {}

    static void set_window_size(const int width, const int height) {}

    static void get_window_size(int& width, int& height) {
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        width  = cols;
        height = rows;
    }

    static SDL_Window* get_sdl_window() { return nullptr; }

    static void* get_renderer() { return nullptr; }

    static int get_renderer_type() { return RENDERER_TERMINAL; }

} // namespace umfeld::subsystem

umfeld::SubsystemGraphics* umfeld_create_subsystem_graphics_terminal() {
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
    graphics->post                   = umfeld::subsystem::post; // TODO maybe remove this, as there is also a `draw_post()` method
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
