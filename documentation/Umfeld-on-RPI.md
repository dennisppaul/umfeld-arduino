# Umfeld on Raspberry Pi (RPI)

RPI currently uses X11 as the rendering system when in desktop environment. however, there is also an allegedly much faster KMS ( or KMSDRM ) rendering system which can start fullscreen windows without(!) a GUI i.e from the command-line ( and even from a remote machine via SSH ). i have tested this already, it does work but requires some extra development. stay tuned.

PS: RPI does not support antialiasing i.e make sure to set the value `antialiasing` to `0` in `settings()`:

```cpp
void settings(){
    antialiasing = 0;
}
```

this step by step guide has been tested on a *Raspberry Pi 4 Model B* and *Raspberry Pi 400* with Raspberry Pi OS (64-bit), *Debian Bookworm* ( Released: 2025-05-13, Size: 1152MB ) installed.

however, it has not been tested carefully. there might be glitches …

## Quickstart

an `.img` image with *Umfeld* and all dependencies can be download from: http://dm-hb.de/umfeld-rpi and installed with tools like [Raspberry Pi Imager](https://www.raspberrypi.com/software/).

the image has been tested on a *Raspberry Pi 4 Model B* and *Raspberry Pi 400* with Raspberry Pi OS (64-bit), *Debian Bookworm* ( Released: 2025-05-13, Size: 1152MB ) installed.

the credentials are:

- name : `umfeld.local` 
- user: `umfeld` 
- password: `umfeld123`

## Preparing the Build Environment on Raspberry Pi OS (with desktop)

This guide helps set up a Raspberry Pi (running *Raspberry Pi OS with desktop*) for compiling and running SDL3-based applications with direct rendering (KMSDRM).

### 1. System Update and Base Development Packages

update package list and upgrade installed packages:

```sh
sudo apt update -y
sudo apt upgrade -y
```

### 2. Dependencies

these packages are needed for media, audio, and graphics support in *Umfeld* applications:

```sh
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
```

**Note:** `gcc`, `git`, and `pkg-config` are typically pre-installed on Raspberry Pi OS with desktop, but are included above for completeness.

SDL3 is not yet available in the official Raspberry Pi OS repositories  ( e.g via `apt install libsdl3-dev` ). It must be built from source.

### 3. Building and Installing SDL3 from Source

these additional packages are required for building SDL3 with KMSDRM (direct rendering) support:

```sh
sudo apt install -y \
  libdrm-dev \
  libgbm-dev \
  libudev-dev \
  libegl-dev \
  libgl-dev
```

now build and install SDL3 from source:

```sh
git clone https://github.com/libsdl-org/SDL.git
cd SDL
cmake -S . -B build \
  -DSDL_KMSDRM=ON \
  -DSDL_OPENGL=ON \
  -DSDL_OPENGLES=ON \
  -DVIDEO_KMSDRM=ON \
  -DVIDEO_X11=ON \
  -DVIDEO_WAYLAND=ON \
  -DSDL_ALSA=ON \
  -DSDL_PULSEAUDIO=ON \
  -DSDL_PIPEWIRE=ON \
  -DSDL_JACK=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build   # Avoid using -j$(nproc) or `--parallel` for now, it may overwhelm the RPi
sudo cmake --install build --prefix /usr/local
```

once installed, confirm SDL3 is available to projects:

```sh
pkg-config --libs --cflags sdl3
```

this should return the necessary compiler and linker flags for SDL3.

now the system is fully set up to build SDL3-based applications with KMSDRM support.

## Setting up Umfeld

first clone [umfeld](https://github.com/dennisppaul/umfeld) and the [umfeld-examples](https://github.com/dennisppaul/umfeld-examples) repositories with submodules from GitHub:

```sh
git clone https://github.com/dennisppaul/umfeld.git
git clone https://github.com/dennisppaul/umfeld-examples.git
```

now enter the example directory, to build and run an example e.g a basic minimal example:

```sh
cd umfeld-examples/Basics/minimal
cmake -B build
cmake --build build # `--parallel` might be too much for the RPI
./build/minimal
```

note, the first time might take a bit on a small machine. note, examples can not be run from `ssh` sessions ( without *X11 forwarding* ).

## X11 Forwarding

to run GUI applications from another machine via `ssh` the following steps must be taken:

- check that remote RPI has *X11 forwarding* enabled. in `/etc/ssh/sshd_config` confirm that `X11Forwarding yes`. if not change it an restart `ssh` with `sudo systemctl restart ssh`
- make sure local machine has an *X server* running ( e.g for macOS install XQuartz with `brew install xquartz` )
- start `ssh` session from local machine with `-Y` option e.g `ssh -Y umfeld@umfeld.local`
- run application with `DISPLAY` set to remote screen e.g `DISPLAY=:0 ./umgegbung-application`
