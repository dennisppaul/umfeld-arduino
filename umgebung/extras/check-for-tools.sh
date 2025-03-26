#!/bin/bash

echo "+++ checking for SDL2"

# Get SDL2 include path
SDL_CFLAGS=$(sdl2-config --cflags)
if [ $? -ne 0 ]; then
    echo "Error: SDL2 development libraries are not installed or sdl2-config is not in your PATH."
    exit 1
fi

# Get SDL2 library path
SDL_LDFLAGS=$(sdl2-config --libs)
if [ $? -ne 0 ]; then
    echo "Error: SDL2 development libraries are not installed or sdl2-config is not in your PATH."
    exit 1
fi

echo "    SDL2 Include Flags : $SDL_CFLAGS"
echo "    SDL2 Linker Flags  : $SDL_LDFLAGS"

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

echo "+++ checking for gcc"
if command_exists gcc; then
    echo "    gcc is installed   : $(which gcc)"
    echo -n "    "
    gcc --version | head -n 1
else
    echo "Error: gcc is not installed."
    exit 1
fi

echo "+++ checking for g++"
if command_exists g++; then
    echo "    g++ is installed   : $(which g++)"
    echo -n "    "
    g++ --version | head -n 1
else
    echo "Error: g++ is not installed."
    exit 1
fi

echo