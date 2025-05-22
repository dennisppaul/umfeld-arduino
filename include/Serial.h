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

#include <string>
#include <vector>
#include <deque>
#include <chrono>

#if defined(SYSTEM_WINDOWS)
#include <windows.h>
#elif (defined(SYSTEM_MACOS) || defined(SYSTEM_UNIX))
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

class Serial {
public:
    Serial(const std::string& portName, int baudrate, bool flush_buffer = true);
    Serial(const std::string& portName, int baudrate, char parity, int dataBits, int stopBits, bool flush_buffer = true);
    ~Serial();

    void poll(); // read data from the serial port

    int                             available() const;
    void                            buffer(int n);
    void                            bufferUntil(uint8_t b);
    void                            clear();
    int                             last() const;
    char                            lastChar() const;
    static std::vector<std::string> list(const std::vector<std::string>& filters);
    static std::vector<std::string> list() { return list({"tty."}); } // TODO find a good default pattern e.g "tty."
    int                             read();
    std::vector<uint8_t>            readBytes();
    std::vector<uint8_t>            readBytesUntil(uint8_t delimiter);
    char                            readChar();
    std::string                     readString();
    std::string                     readStringUntil(char delimiter);
    void                            stop();
    void                            write(uint8_t b);
    void                            write(const std::vector<uint8_t>& data);
    void                            write(const std::string& str);

private:
#ifdef SYSTEM_WINDOWS
    HANDLE handle;
#else
    int handle;
#endif
    std::deque<uint8_t>                   rxBuffer;
    int                                   lastByte        = -1;
    bool                                  isOpen          = false;
    int                                   bufferSize      = 1;
    int                                   bufferDelimiter = -1;
    std::chrono::steady_clock::time_point lastPoll        = std::chrono::steady_clock::now();

    void configure(bool flush_buffer, int baudrate, char parity, int dataBits, int stopBits);
};
