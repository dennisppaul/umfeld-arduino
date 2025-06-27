# Umfeld / CMake Options

⚠️ @TODO this needs to be reworked ⚠️


## OpenGL Version

```
set(UMFELD_OPENGL_VERSION "OPENGL_3_3_CORE") # set OpenGL version. currently available: "OPENGL_2_0", "OPENGL_3_3_CORE" or "OPENGL_ES_3_0"
```

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
