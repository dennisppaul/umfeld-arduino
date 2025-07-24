---
name: Bug report
about: Create a report to help improve Umfeld
title: '[BUG] '
labels: bug
assignees: ''
---

Thank you for helping improve **Umfeld**! Please complete the checklist and provide as much detail as possible.

---

## Checklist (please confirm before submitting)

- [ ] I have searched [existing issues](https://github.com/dennisppaul/umfeld/issues) and this has not been reported yet.
- [ ] I have provided a minimal example or clear steps to reproduce the issue.
- [ ] I have included any relevant logs, screenshots, or error messages.
- [ ] I have described the environment and backend I'm using (e.g. OpenGL, renderer, OS).

---

## Environment

- OS: (e.g. macOS 15.5, Ubuntu 24.04, Windows 11 UCRT64)
- Renderer backend: (e.g. OpenGL 3.3, GLES, etc.)
- SDL version: (if known e.g. 3.2.18)
- Compiler: (e.g. clang 17, gcc 13)
- Umfeld version / commit: (e.g. v2.0.1 or `a1b2c3d`)

<details>
<summary><strong>Not sure where to find this info?</strong> (click to expand)</summary>

- **OS**:  
  - **macOS**: Run `sw_vers`  
  - **Linux**: Run `lsb_release -a` or `cat /etc/os-release`  
  - **Windows (MSYS2 UCRT64)**: Run `uname -o && uname -m && gcc --version`

- **Renderer backend**:  
  Check your code or console/log output.  
  CMake script output something like `-- OPEN_GL       : OPENGL_3_3_CORE`

- **SDL version (SDL3)**:  
  - If you built SDL3 from source: check `CMakeCache.txt` or the cloned commit  
  - If installed via package manager:  
    - macOS: `brew info sdl3`  
    - Linux: `apt show libsdl3` or `pacman -Qi sdl3`  
    - Windows (MSYS2 UCRT64): `pacman -Qi mingw-w64-ucrt-x86_64-SDL3`

- **Compiler**:  
  - macOS: `clang --version`  
  - Linux: `gcc --version` or `clang --version`  
  - Windows (MSYS2 UCRT64): `gcc --version` or `clang --version`

- **Umfeld version / commit**:  
  - Umfeld version is defined in `include/UmfeldVersion.h`  
  - If you cloned the repo: `git rev-parse --short HEAD`  
  - If using a zip or release: check the GitHub tag or folder name

</details>

## Describe the Bug

Please add a clear and concise description of what the bug is.

## Steps to Reproduce

1. …
2. …
3. …

## Expected behavior

Please provide a short description of what you expected to happen.

## Additional context

Please add screenshots, context, ideas for a fix, etcetera.
