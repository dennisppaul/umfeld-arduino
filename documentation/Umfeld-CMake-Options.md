# Umfeld / CMake Options

⚠️ @TODO this needs to be reworked ⚠️

## OpenGL Version

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

## Build Options

option(ENABLE_PORTAUDIO "Enable PortAudio output" ON)
option(ENABLE_ANGLE "Enable ANGLE OpenGL ES emulation on macOS ( apple silicon )" OFF)
option(DISABLE_PDF "Disable PDF export ( cairo )" OFF)
option(DISABLE_NCURSES "Disable ncurses support" OFF)
option(DISABLE_VIDEO "Disable video ( ffmpeg ) support" OFF)
option(DISABLE_AUDIO "Disable audio ( SDL3 + portaudio ) support" OFF)
option(DISABLE_OPENGL "Disable OpenGL support" OFF)
option(DISABLE_GRAPHICS "Disable graphics support" OFF) # same as DISABLE_OPENGL?
option(DISABLE_FONT "Disable font rendering ( harfbuzz + freetype ) support" OFF)
option(DISABLE_MAIN "Disable main function" OFF)
