#!/bin/bash

set -e

echo "--- updating apt"
sudo apt-get update
sudo apt-get -y upgrade

echo "--- installing dependencies (RaspberryPi)"
sudo apt-get -y install \
  build-essential \
  git \
  cmake \
  curl \
  libx11-dev \
  libegl1-mesa-dev \
  libgles2-mesa-dev
echo "--- installing umfeld dependencies"
sudo apt-get -y install \
  pkg-config \
  ffmpeg \
  libavcodec-dev \
  libavformat-dev \
  libavutil-dev \
  libswscale-dev \
  libavdevice-dev \
  libharfbuzz-dev \
  libfreetype6-dev \
  librtmidi-dev \
  libglm-dev \
  portaudio19-dev \
  libcairo2-dev \
  libcurl4-openssl-dev \
  libncurses-dev
#sudo apt-get install libsdl3-dev # currently (2025-05-22) not available

echo "--- building SDL3 from source"

sudo apt-get -y install \
  libdrm-dev \
  libgbm-dev \
  libudev-dev \
  libegl-dev \
  libgl-dev

# detect if running headless (no X11/Wayland)
echo "--- detecting display system "
USE_CONSOLE_BUILD=OFF
if [[ -z "$DISPLAY" && -z "$WAYLAND_DISPLAY" ]]; then
  echo "--- no X11 or Wayland detected, enabling SDL_UNIX_CONSOLE_BUILD"
  USE_CONSOLE_BUILD=ON
else
  echo "--- graphical environment detected"
fi

TMP_DIR=$(mktemp -d)
cd "$TMP_DIR"

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

echo "--- testing SDL3 pkg-config:"
pkg-config --libs --cflags sdl3

# clean up
cd ~
rm -rf "$TMP_DIR"
echo "--- installed SDL3 successfully"

echo "--- RaspberryPi environment set up for 'umfeld'."
echo
