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

#include "Serial.h"

#include <iostream>
#include <stdexcept>
#include <cstring>
#ifdef SYSTEM_WINDOWS
#include <setupapi.h>
#include <tchar.h>
#pragma comment(lib, "setupapi.lib")
#else
#include <fcntl.h>
#include <dirent.h>
#include <map>
#endif

static speed_t getBaudConstant(int baudrate) {
#ifdef SYSTEM_WINDOWS
    return baudrate; // not used
#else
    static std::map<int, speed_t> baudMap = {
        {0, B0},       {50, B50},     {75, B75},     {110, B110},   {134, B134},
        {150, B150},   {200, B200},   {300, B300},   {600, B600},   {1200, B1200},
        {1800, B1800}, {2400, B2400}, {4800, B4800}, {9600, B9600}, {19200, B19200},
        {38400, B38400}, {57600, B57600}, {115200, B115200}, {230400, B230400}
    };
    auto it = baudMap.find(baudrate);
    return (it != baudMap.end()) ? it->second : B9600;
#endif
}

Serial::Serial(const std::string& portName, int baudrate, bool flush_buffer) : Serial(portName, baudrate, 'N', 8, 1, flush_buffer) {}

Serial::Serial(const std::string& portName, int baudrate, char parity, int dataBits, int stopBits, bool flush_buffer) {
#ifdef SYSTEM_WINDOWS
    std::wstring wport(portName.begin(), portName.end());
    handle = CreateFileW((L"\\.\\" + wport).c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Cannot open port");
    }
#else
    handle = open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (handle < 0) {
        throw std::runtime_error("Cannot open port");
    }
#endif
    configure(baudrate, parity, dataBits, stopBits, flush_buffer);
    isOpen = true;
}

void Serial::configure(bool flush_buffer, int baudrate, char parity, int dataBits, int stopBits) {
#ifdef SYSTEM_WINDOWS
    DCB dcb       = {0};
    dcb.DCBlength = sizeof(dcb);
    GetCommState(handle, &dcb);
    dcb.BaudRate = baudrate;
    dcb.ByteSize = static_cast<BYTE>(dataBits);
    dcb.Parity   = (parity == 'E') ? EVENPARITY : (parity == 'O') ? ODDPARITY : NOPARITY;
    dcb.StopBits = (stopBits == 2) ? TWOSTOPBITS : ONESTOPBIT;
    SetCommState(handle, &dcb);

    COMMTIMEOUTS timeouts             = {0};
    timeouts.ReadIntervalTimeout      = 1;
    timeouts.ReadTotalTimeoutConstant = 1;
    SetCommTimeouts(handle, &timeouts);
#else
    termios tty = {};
    tcgetattr(handle, &tty);
    speed_t baud = getBaudConstant(baudrate);
    cfsetospeed(&tty, baud);
    cfsetispeed(&tty, baud);
    tty.c_cflag = (tty.c_cflag & ~CSIZE);
    tty.c_cflag |= (dataBits == 7) ? CS7 : CS8;
    tty.c_cflag |= (stopBits == 2) ? CSTOPB : 0;

    if (parity == 'E') {
        tty.c_cflag |= PARENB;
        tty.c_cflag &= ~PARODD;
    } else if (parity == 'O') {
        tty.c_cflag |= PARENB;
        tty.c_cflag |= PARODD;
    } else {
        tty.c_cflag &= ~PARENB;
    }

    tty.c_iflag     = 0;
    tty.c_oflag     = 0;
    tty.c_lflag     = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 1;
    tcsetattr(handle, TCSANOW, &tty);
#endif

    if (flush_buffer) {
#ifdef SYSTEM_WINDOWS
        PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
#else
        tcflush(handle, TCIFLUSH);
#endif
    }
}

Serial::~Serial() { stop(); }

void Serial::stop() {
    if (!isOpen) {
        return;
    }
#ifdef SYSTEM_WINDOWS
    CloseHandle(handle);
#else
    close(handle);
#endif
    isOpen = false;
}

// void Serial::poll() {
//     char buf[256];
// #ifdef SYSTEM_WINDOWS
//     DWORD n = 0;
//     if (ReadFile(handle, buf, sizeof(buf), &n, nullptr) && n > 0) {
//         for (DWORD i = 0; i < n; ++i) {
//             rxBuffer.push_back(buf[i]);
//             lastByte = (uint8_t) buf[i];
//         }
//     }
// #else
//     int n = ::read(handle, buf, sizeof(buf));
//     if (n > 0) {
//         for (int i = 0; i < n; ++i) {
//             rxBuffer.push_back(buf[i]);
//             lastByte = static_cast<uint8_t>(buf[i]);
//         }
//     }
// #endif
// }

void Serial::poll() {
    constexpr int pollIntervalMs = 2;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPoll).count();
    if (elapsed < pollIntervalMs) return;
    lastPoll = now;

#ifdef SYSTEM_WINDOWS
    DWORD errors;
    COMSTAT status;
    if (!ClearCommError(handle, &errors, &status)) return;
    if (status.cbInQue == 0) return;
#else
    int bytesAvailable = 0;
    ioctl(handle, FIONREAD, &bytesAvailable);
    if (bytesAvailable == 0) return;
#endif

    char buf[256];
#ifdef SYSTEM_WINDOWS
    DWORD n = 0;
    if (ReadFile(handle, buf, sizeof(buf), &n, nullptr) && n > 0) {
        for (DWORD i = 0; i < n; ++i) {
            rxBuffer.push_back(buf[i]);
            lastByte = (uint8_t)buf[i];
        }
    }
#else
    int n = ::read(handle, buf, sizeof(buf));
    if (n > 0) {
        for (int i = 0; i < n; ++i) {
            rxBuffer.push_back(buf[i]);
            lastByte = (uint8_t)buf[i];
        }
    }
#endif
}

int Serial::available() const { return rxBuffer.size(); }

void Serial::clear() { rxBuffer.clear(); }

int Serial::last() const { return lastByte; }

char Serial::lastChar() const { return lastByte >= 0 ? static_cast<char>(lastByte) : -1; }

int Serial::read() {
    if (rxBuffer.empty()) {
        return -1;
    }
    uint8_t b = rxBuffer.front();
    rxBuffer.pop_front();
    return b;
}

char Serial::readChar() {
    int b = read();
    return b >= 0 ? static_cast<char>(b) : -1;
}

std::vector<uint8_t> Serial::readBytes() {
    std::vector<uint8_t> result(rxBuffer.begin(), rxBuffer.end());
    rxBuffer.clear();
    return result;
}

std::vector<uint8_t> Serial::readBytesUntil(uint8_t delimiter) {
    std::vector<uint8_t> result;
    while (!rxBuffer.empty()) {
        uint8_t b = rxBuffer.front();
        rxBuffer.pop_front();
        result.push_back(b);
        if (b == delimiter) {
            break;
        }
    }
    return result;
}

std::string Serial::readString() {
    std::string s(rxBuffer.begin(), rxBuffer.end());
    rxBuffer.clear();
    return s;
}

std::string Serial::readStringUntil(char delimiter) {
    std::string result;
    while (!rxBuffer.empty()) {
        char c = rxBuffer.front();
        rxBuffer.pop_front();
        result += c;
        if (c == delimiter) {
            break;
        }
    }
    return result;
}

void Serial::buffer(int n) {
    bufferSize = n;
}

void Serial::bufferUntil(uint8_t b) {
    bufferDelimiter = b;
}

void Serial::write(uint8_t b) {
#ifdef SYSTEM_WINDOWS
    DWORD n;
    WriteFile(handle, &b, 1, &n, nullptr);
#else
    ::write(handle, &b, 1);
#endif
}
void Serial::write(const std::vector<uint8_t>& data) {
#ifdef SYSTEM_WINDOWS
    DWORD n;
    WriteFile(handle, data.data(), data.size(), &n, nullptr);
#else
    ::write(handle, data.data(), data.size());
#endif
}
void Serial::write(const std::string& str) {
    write(std::vector<uint8_t>(str.begin(), str.end()));
}

std::vector<std::string> Serial::list(const std::vector<std::string>& filters) {
    std::vector<std::string> ports;
#ifdef SYSTEM_WINDOWS
    for (int i = 1; i <= 256; ++i) {
        std::string  name  = "COM" + std::to_string(i);
        std::wstring wname = L"\\\\.\\" + std::wstring(name.begin(), name.end());
        HANDLE       h     = CreateFileW(wname.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
            ports.push_back(name);
        }
    }
#else
    DIR* dev = opendir("/dev");
    if (!dev) {
        return ports;
    }
    while (dirent* e = readdir(dev)) {
        std::string name(e->d_name);
        if (filters.empty()) {
            ports.push_back("/dev/" + name);
        } else {
            for (const auto& pattern: filters) {
                if (name.find(pattern) != std::string::npos) {
                    ports.push_back("/dev/" + name);
                    break;
                }
            }
        }
    }
    closedir(dev);
#endif
    return ports;
}
