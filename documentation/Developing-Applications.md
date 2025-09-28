# Umfeld / Developing Applications

## Quickstart

the easiest way to start a new application is to:

1. copy the folder `umfeld/template-application` to a desired location  
2. rename the folder to a desired name e.g `my-application`  
3. in the folder, find and open `CMakeLists.txt`  
    1. change `project(template)` to a desired application name e.g `project(my-application)`  
    2. set `UMFELD_PATH` ( `set(UMFELD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")` ) to the location of the `umfeld` folder. the location can be relative ( e.g `../` ), relative to the application ( e.g `${CMAKE_CURRENT_SOURCE_DIR}/..` ), or absolute ( e.g `/usr/home/myusername/umfeld` )  
    3. save changes to `CMakeLists.txt`  
4. open a terminal and navigate to the application folder ( e.g `cd my-application` )  
5. configure build folder `cmake -B build`  
6. compile application `cmake --build build`  
7. run application `./build/my-application`

> tip: if compilation fails, make sure the `umfeld` path is correct and all dependencies are installed ( see [DOCUMENTATION](documentation/DOCUMENTATION.md) ).

## Default Application Structure

```
my-application
├── application.cpp
├── CMakeLists.txt
└── data
    └── umfeld.png
```

the directory `my-application` should contain the `CMakeLists.txt`, all source and header files as well as all resources ( e.g images, fonts and movies ) in a `data` folder.

in this example `application.cpp` is the main source file containing the *entry points* used by *Umfeld*:

```c
#include "Umfeld.h"

void settings() {}

void setup() {}

void draw() {}
```

in order to compile the application a CMake script `CMakeLists.txt` must be supplied. the following is a script that may be used to compile the above example:

```cmake
cmake_minimum_required(VERSION 3.12)

project(my-application)                           # set application name
set(UMFELD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..") # set path to umfeld library

# --------- no need to change anything below this line ---------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(".")
file(GLOB SOURCE_FILES "*.cpp")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

add_subdirectory(${UMFELD_PATH} ${CMAKE_BINARY_DIR}/umfeld-lib-${PROJECT_NAME})
add_umfeld_libs()
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
