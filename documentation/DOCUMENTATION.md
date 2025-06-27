# Umfeld / Documentation

## Installing Umfeld

### Prerequisite

#### macOS

install the required packages with [Homebrew](https://brew.sh).

either run the install script:

```sh
./installation/install-macOS.sh
```

or install manually step by step:

```sh
brew update
brew install \
  cmake \
  pkgconfig \
  sdl3 \
  harfbuzz \
  freetype \
  ffmpeg \
  rtmidi \
  glm \
  dylibbundler \
  portaudio \
  cairo
```

#### Linux

install the required packages with [APT](https://en.wikipedia.org/wiki/APT_(software)) ( for Raspberry Pi OS, see detailed instructions below ).

either run the install script:

```sh
./installation/install-linux.sh
```

or install manually step by step:

```sh
sudo apt-get update -y
sudo apt-get upgrade -y
sudo apt-get install -y \
  git \
  clang \
  cmake \
  curl \
  libx11-dev \
  libgl1-mesa-dev \
  libglu1-mesa-dev \
  mesa-utils \
  libxext-dev \
  libxrandr-dev \
  libxcursor-dev \
  libxi-dev \
  libxinerama-dev \
  libwayland-dev \
  libxkbcommon-dev \
  wayland-protocols \
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
# sudo apt-get install libsdl3-dev # currently (2025-06-27) not available
```

##### Build SDL3 from source

⚠️ WARNING ⚠️ as of (2025-06-27) SDL3 is not available via apt and must be built from source ( see `install-linux.sh` or below )

```sh
git clone --depth=1 https://github.com/libsdl-org/SDL.git
cd SDL
cmake -S . -B build \
  -DSDL_KMSDRM=ON \
  -DSDL_OPENGL=ON \
  -DSDL_OPENGLES=ON \
  -DSDL_WAYLAND=ON \
  -DSDL_X11=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build
```

#### Raspberry Pi OS (RPI)

*Umfeld* can run on Raspberry Pi ( e.g RPI 4 Model B and RPI 5 ). see [Umfeld-on-RPI](Umfeld-on-RPI.md) for detailed information.

open a terminal and either run the install script:

```sh
./installation/install-raspberry-pi.sh
```

or install manually step by step:

```sh
sudo apt-get update -y
sudo apt-get upgrade -y
sudo apt-get install -y \
  build-essential \
  git \
  cmake \
  curl \
  libx11-dev \
  libegl1-mesa-dev \
  libgles2-mesa-dev \
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
```

##### Build SDL from source

note, currently SDL3 is not available via `apt` and needs to be build from source. the following steps should work without modification:

```sh
sudo apt-get install -y \
  libdrm-dev \
  libgbm-dev \
  libudev-dev \
  libegl-dev \
  libgl-dev
git clone --depth=1 https://github.com/libsdl-org/SDL.git
cd SDL
USE_CONSOLE_BUILD=OFF # if running headless (no X11/Wayland) set to 'USE_CONSOLE_BUILD=ON'
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
```

#### Windows

- install [MSYS2](https://www.msys2.org/)
- open `MSYS2 UCRT64`

either run the install script:

```sh
./installation/install-windows-ucrt64.sh
```

or install manually step by step:

```sh
pacman -Syu --noconfirm
pacman -Syu --noconfirm # Repeat after first upgrade to ensure core tools are also updated
pacman -S --noconfirm \
  mingw-w64-ucrt-x86_64-toolchain \
  git \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-curl \
  mingw-w64-ucrt-x86_64-mesa \
  mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-ffmpeg \
  mingw-w64-ucrt-x86_64-harfbuzz \
  mingw-w64-ucrt-x86_64-freetype \
  mingw-w64-ucrt-x86_64-rtmidi \
  mingw-w64-ucrt-x86_64-glm \
  mingw-w64-ucrt-x86_64-portaudio pkgconf \
  mingw-w64-ucrt-x86_64-cairo \
  mingw-w64-ucrt-x86_64-cairomm \
  mingw-w64-ucrt-x86_64-ncurses \
  mingw-w64-ucrt-x86_64-sdl3 \
  mingw-w64-ucrt-x86_64-sdl3-image \
  mingw-w64-ucrt-x86_64-sdl3-ttf
```

the setup is exclusively for the `MSYS2 UCRT64` branch ( and not for `MSYS2 MINGW64` etcetera ). also it uses `ninja` as a build system instead of `make` ( which is the default on Linux + macOS ).

### Cloning *Umfeld* Repositories

clone the *Umfeld* repositories from GitHub into a desired folder ( e.g `~/Documents/dev` ):

either run the install script:

```sh
./installation/install-macOS.sh
```

or clone manually step by step:

```sh
git clone https://github.com/dennisppaul/umfeld
git clone https://github.com/dennisppaul/umfeld-examples
git clone --recurse-submodules https://github.com/dennisppaul/umfeld-libraries.git # clone with submodules
```

## Building *Umfeld* Example Applications

see [Developing-Applications](Developing-Applications.md) 

## Known Differences

Java and C++ are similar in some aspects, but are very different in many others. a lot of the syntactic differences between *Processing* and *Umfeld* stem from these differences, while a few others are more or less developer decisions.

- elements in `println()` must be concatenated with `,` rather than `+` e.g `println("hello ", 23, " world");` @reason(raw strings cannot be concatenated with a `+` in C++ as they can be in Java)
- the properties `mousePressed` + `keyPressed` are renamed to `isMousePressed` and `isKeyPressed` @reason(in *Processing* the property `mousePressed` and the callback method `mousePressed` carry the same name. in C++ it is not allowed to have a field or property and a method with the same name)
- `fill(float)` must be explicitly differentiated from `fill(uint32_t)` e.g `fill(1.0f)` or `fill(0xFF44FF88)` ( the same is true for `stroke(...)` ) @reason(in a different way from Java, C++ finds two methods that only differ in their parameter types i.e `float` and `uint32_t` ambiguous)

## Known Issues

see [Known-Issues](Known-Issues.md)

### Setting up Homebrew on macOS

on some *clean* homebrew installations on *macOS* the environment variable `$LIBRARY_PATH` is not set or at least does not include the homebrew libraries. if so you may need to add the line `export LIBRARY_PATH=$LIBRARY_PATH:/usr/local/lib` ( for intel-based machines ) or `export LIBRARY_PATH=$LIBRARY_PATH:/opt/homebrew/lib` ( *apple-silicon*-based machines ) to your profile e.g in `~/.zshrc` in *zsh* shell. note, that other shell environments use other profile files and mechanisms e.g *bash* uses `~/.bashrc`. find out which shell you are using by typing `echo $0`.

if you have NO idea what this all means, either run the *Quickstart* installer script or you might just try running the following lines for *zsh* in the terminal:

#### on intel-based machines

```sh
{ echo -e "\n# set library path\n"; [ -n "$LIBRARY_PATH" ] && echo "export LIBRARY_PATH=/usr/local/lib:\"\$LIBRARY_PATH\"" || echo "export LIBRARY_PATH=/usr/local/lib"; } >> "$HOME/.zshrc"
source "$HOME/.zshrc"
```

#### on *apple-silicon*-based machines

```sh
{ echo -e "\n# set library path\n"; [ -n "$LIBRARY_PATH" ] && echo "export LIBRARY_PATH=/opt/homebrew/lib:\"\$LIBRARY_PATH\"" || echo "export LIBRARY_PATH=/opt/homebrew/lib"; } >> "$HOME/.zshrc"
source "$HOME/.zshrc"
```

this will permanently set the `LIBRARY_PATH` environement variable in your *zsh* profile file.

## Resources

- Umfeld [examples](https://github.com/dennisppaul/umfeld-examples)
- Additional [libraries](https://github.com/dennisppaul/umfeld-libraries)

examples and library examples assume that all repositories are located on the same level:

```
.
├── umfeld
├── umfeld-examples
└── umfeld-libraries
```

## Default Application Structure

```
umfeld-example-app
├── application.cpp
├── CMakeLists.txt
└── data
    └── umfeld.png
```

the directory `umfeld-example-app` should contain all source, and header files as well as all resources ( e.g images, fonts and movies ) in a `data` folder.

in this example `umfeld-example-app.cpp` is the main source file containing the *entry points* used by *Umfeld*:

```c
#include "Umfeld.h"

void settings() {}

void setup() {}

void draw() {}
```

in order to compile the application a CMake script `CMakeLists.txt` must be supplied. the following is a script that may be used to compile the above example:

```cmake
cmake_minimum_required(VERSION 3.12)

project(umfeld-example-app)                                    # set application name
set(UMFELD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../umfeld") # set path to umfeld library

# --------- no need to change anything below this line ------------

set(CMAKE_CXX_STANDARD 17)                                         # set c++ standard, this needs to happen before `add_executable`
set(CMAKE_CXX_STANDARD_REQUIRED ON)                                # minimum is C++17 but 20 and 23 should also be fine

include_directories(".")                                           # add all `.h` header files from this directory
file(GLOB SOURCE_FILES "*.cpp")                                    # collect all `.cpp` source files from this directory
add_executable(${PROJECT_NAME} ${SOURCE_FILES})                    # add source files to application

add_subdirectory(${UMFELD_PATH} ${CMAKE_BINARY_DIR}/umfeld-lib-${PROJECT_NAME}) # add umfeld location
add_umfeld_libs()                                                # add umfeld library
```

each application may have an individual name defined in `project(<name-of-application>)`.

it is required to set the variable `UMFELD_PATH` ( e.g `set(UMFELD_PATH "/Users/username/Documents/dev/umfeld/git/umfeld/")` ) which must contain the absolute path to the *Umfeld* library ( i.e the folder that contains e.g this document as well as the `include` and `src` folders of *Umfeld* ). note, that in the example CMake file above the CMake variable `${CMAKE_CURRENT_SOURCE_DIR}` is used to navigate relative to the location of the CMake file ( e.g helpful in the examples ).

the command `link_directories("/usr/local/lib")` can be used to fix a linker error on macOS ( e.g `ld: library 'glfw' not found` ). this error indicates that the global library is not set or not set properly ( i.e `echo $LIBRARY_PATH` returns an empty response or points to a folder that does not contain `libglfw.dylib` in this example ).

the section `# add umfeld` must be placed last in the CMake file and includes *Umfeld* as a library.

## Build Example Application

to compile and run this example just type the following commands in the terminal:

```sh
$ cmake -B build      # prepare build system
$ cmake --build build # compile the application
$ ./build/minimal     # to run the application
```

there is also a way to concatenate all three commands in one line like this:

```sh
$ cmake -B build ; cmake --build build ; ./build/minimal
```

to start a *clean* build simply delete the build directory:

```sh
$ rm -rf build
```

if changes are made to `umfeld-example-app.cpp` ( or any other source or header file in that folder ) it is enough to just run:

```
$ cmake --build build
```

## Umfeld and OpenGL

*Umfeld* supports several different OpenGL versions:

| VERSION         | C/C++ CONSTANT             | CMAKE             | DESCRIPTION                                               |
| --------------- | -------------------------- | ----------------- | --------------------------------------------------------- |
| OpenGL core 3.3 | `RENDERER_OPENGL_3_3_CORE` | `OPENGL_3_3_CORE` | desktop platforms (macOS, Linux, Windows).                |
| OpenGL ES 2.0   | `RENDERER_OPENGL_2_0`      | `OPENGL_2_0`      | older embedded systems or where only ES 2.0 is supported. |
| OpenGL ES 3.0   | `RENDERER_OPENGL_ES_3_0`   | `OPENGL_ES_3_0`   | iOS and WebGL 2.0 compatibility.                          |

the OpenGL renderer can be explicitly selected in `size()` or left empty for default:

```C++
size(1024, 768, RENDERER_OPENGL_3_3_CORE); 
```

additionally, the OpenGL version must be defined in the applications CMake script in `UMFELD_OPENGL_VERSION` and *before* `add_umfeld_libs()`:

```cmake
set(UMFELD_OPENGL_VERSION "OPENGL_3_3_CORE")
add_umfeld_libs()
```

if left blank *Umfeld* tries to find the *best* version.