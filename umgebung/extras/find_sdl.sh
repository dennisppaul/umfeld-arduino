#!/bin/zsh

BREW_PREFIX=$(brew --prefix 2>/dev/null || echo "/usr/local")

# Check for SDL2 installation
if [ -d "$BREW_PREFIX/opt/sdl2" ]; then
    SDL2_INCLUDE_PATH="$BREW_PREFIX/include/SDL2"
    SDL2_LIB_PATH="$BREW_PREFIX/lib"
else
    echo "Error: SDL2 not found in Homebrew installation!" >&2
    exit 1
fi

# Output paths to a file
echo "-I$SDL2_INCLUDE_PATH" > "$1/sdl_include.txt"
echo "-L$SDL2_LIB_PATH -lSDL2" > "$1/sdl_libs.txt"
