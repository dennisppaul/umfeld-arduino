#!/bin/bash

set -e

echo "+++ updating package manager  +++"

sudo apt-get update -y
sudo apt-get upgrade -y

echo "+++ installing packages       +++"

sudo apt install -y \
  build-essential \
  cmake \
  git \
  pkg-config \
  ffmpeg \
  libharfbuzz-dev \
  libfreetype6-dev \
  libavcodec-dev \
  libavformat-dev \
  libavutil-dev \
  libswscale-dev \
  libavdevice-dev \
  librtmidi-dev \
  libglm-dev \
  portaudio19-dev \
  libegl1-mesa-dev \
  libgles2-mesa-dev
#sudo apt install libsdl3-dev # currently (2025-05-22) not available

echo "+++ building SDL3 from source +++"

sudo apt install -y \
  libdrm-dev \
  libgbm-dev \
  libudev-dev \
  libegl-dev \
  libgl-dev

# detect if running headless (no X11/Wayland)
echo "+++ detecting display system  +++"
USE_CONSOLE_BUILD=OFF
if [[ -z "$DISPLAY" && -z "$WAYLAND_DISPLAY" ]]; then
  echo "+++ no X11 or Wayland detected, enabling SDL_UNIX_CONSOLE_BUILD"
  USE_CONSOLE_BUILD=ON
else
  echo "+++ graphical environment detected"
fi

git clone https://github.com/libsdl-org/SDL.git
cd SDL

cmake -S . -B build \
  -DSDL_KMSDRM=ON \
  -DSDL_OPENGL=ON \
  -DSDL_OPENGLES=ON \
  -DSDL_ALSA=ON \
  -DSDL_PULSEAUDIO=ON \
  -DSDL_PIPEWIRE=ON \
  -DSDL_JACK=ON \
  -DSDL_UNIX_CONSOLE_BUILD=$USE_CONSOLE_BUILD \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build
sudo cmake --install build --prefix /usr/local

echo "+++ testing SDL3 pkg-config:"
pkg-config --libs --cflags sdl3

echo "+++"
echo
