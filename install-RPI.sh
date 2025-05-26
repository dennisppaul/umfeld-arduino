#!/bin/bash

# TODO this is not tested on RPI

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
  portaudio19-dev
#sudo apt install libsdl3-dev # currently (2025-05-22) not available

echo "+++ building SDL3 from source +++"

sudo apt install -y \
  libdrm-dev \
  libgbm-dev \
  libudev-dev \
  libegl-dev \
  libgl-dev

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
  -DCMAKE_BUILD_TYPE=Release
cmake --build build   # Avoid using -j$(nproc) or `--parallel` for now, it may overwhelm the RPi
sudo cmake --install build --prefix /usr/local

echo "+++ testing SDL3 pkg-config:"
pkg-config --libs --cflags sdl3

echo "+++"
echo
