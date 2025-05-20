cmake_minimum_required(VERSION 3.20)

project(umfeld-lib)

message(STATUS "----------------------------------------")
message(STATUS "UMFELD_APP    : ${PROJECT_NAME}")

option(ENABLE_PORTAUDIO "Enable PortAudio output" ON)

##########################################################################################################
### CHECK FOR HOMEBREW                                                                                 ###
##########################################################################################################
find_program(BREW_FOUND brew)
if (APPLE)
    if (BREW_FOUND)
        execute_process(COMMAND brew --prefix
                OUTPUT_VARIABLE HOMEBREW_PREFIX
                OUTPUT_STRIP_TRAILING_WHITESPACE)
        set(HOMEBREW_LIB_PATH "${HOMEBREW_PREFIX}/lib")
        message(STATUS "HOMEBREW LIBS : ${HOMEBREW_LIB_PATH}")
    else ()
        message(WARNING "HOMEBREW not found.")
    endif ()
endif (APPLE)

##########################################################################################################
### SUPPLEMENT LIBRARY SEARCH PATH                                                                     ###
##########################################################################################################

# NOTE this does not work when executed after `add_executable` or `add_library` … which is exactly what happens :(
# if (APPLE)
#     # NOTE add default search path for libraries on macOS e.g for default Homebrew installation
#     link_directories("/usr/local/lib")
#     link_directories("/opt/homebrew/lib")
#     message(STATUS "GLOBAL LIB    : adding `/usr/local/lib` + `/opt/homebrew/lib` to library search path")
# elseif (UNIX)
# elseif (WIN32)
# else ()
# endif ()

##########################################################################################################
### ADD SOURCE + HEADER FILES                                                                          ###
##########################################################################################################

# Add library target
if (APPLE)
    file(GLOB UMFELD_SOURCE_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/*.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/libtess2/*.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/clipper/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/ip/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/ip/posix/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/osc/*.cpp
    )
elseif (UNIX)
    file(GLOB UMFELD_SOURCE_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/*.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/libtess2/*.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/clipper/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/ip/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/ip/posix/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/osc/*.cpp
    )
elseif (WIN32)
    file(GLOB UMFELD_SOURCE_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/*.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/libtess2/*.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/clipper/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/ip/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/ip/win32/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/osc/*.cpp
    )
else ()
    file(GLOB UMFELD_SOURCE_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/*.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/libtess2/*.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/clipper/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/ip/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/ip/posix/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/osc/*.cpp
    )
endif ()

if (NOT TARGET umfeld-lib)
    add_library(umfeld-lib ${UMFELD_SOURCE_FILES})
endif ()

# Specify include directories for the library
target_include_directories(umfeld-lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/shaders
        ${CMAKE_CURRENT_SOURCE_DIR}/include/libraries
        ${CMAKE_CURRENT_SOURCE_DIR}/include/libraries/dr_libs
        ${CMAKE_CURRENT_SOURCE_DIR}/include/libraries/ip
        ${CMAKE_CURRENT_SOURCE_DIR}/include/libraries/libtess2
        ${CMAKE_CURRENT_SOURCE_DIR}/include/libraries/osc
)

##########################################################################################################
### EN/DISABLE LIBRARIES                                                                               ###
##########################################################################################################
if (NOT TARGET umfeld-lib-interface)
    add_library(umfeld-lib-interface INTERFACE) # this enables `glm::aligned_vec*` types
endif ()

target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_ENABLE_EXPERIMENTAL)

if (NOT DISABLE_MAIN)
    target_compile_definitions(${PROJECT_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)
else ()
    message(STATUS "DISABLE_MAIN  : no `main` function entry point. must be defined in project.")
endif (NOT DISABLE_MAIN)

if (DISABLE_GRAPHICS)
    target_compile_definitions(umfeld-lib-interface INTERFACE DISABLE_GRAPHICS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DISABLE_GRAPHICS)
    message(STATUS "DISABLING     : graphics")
endif (DISABLE_GRAPHICS)

if (DISABLE_AUDIO)
    target_compile_definitions(umfeld-lib-interface INTERFACE DISABLE_AUDIO)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DISABLE_AUDIO)
    message(STATUS "DISABLING     : audio")
endif (DISABLE_AUDIO)

if (DISABLE_VIDEO)
    target_compile_definitions(umfeld-lib-interface INTERFACE DISABLE_VIDEO)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DISABLE_VIDEO)
    message(STATUS "DISABLING     : video")
endif (DISABLE_VIDEO)

if (ENABLE_PORTAUDIO)
    target_compile_definitions(umfeld-lib-interface INTERFACE ENABLE_PORTAUDIO)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ENABLE_PORTAUDIO)
    message(STATUS "ENABLING      : PortAudio Driver")
endif (ENABLE_PORTAUDIO)

if (ENABLE_CAPTURE)
    target_compile_definitions(umfeld-lib-interface INTERFACE ENABLE_CAPTURE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ENABLE_CAPTURE)
    message(STATUS "ENABLING      : Capture")
endif (ENABLE_CAPTURE)

##########################################################################################################
### OPEN_GL VERSION + GLAD                                                                             ###
##########################################################################################################

set(UMFELD_DEFAULT_OPENGL_VERSION "core3.3" CACHE STRING "OpenGL version used in umfeld")
# NOTE auto-generated files with GLAD 2 ( on macOS ):
#    brew install glad2
#    glad --api="gl:core=3.3" --out-path=external/glad_opengl_33 c
#    glad --api="gl:compatibility=2.0" --out-path=external/glad_opengl_20 c
#    glad --api="gles2=3.0" --out-path=external/glad_opengles_30 c
# TODO maybe move this into library folder(s)?
set(UMFELD_GLAD_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external" CACHE STRING "GLAD base directory")

# if OpenGL not set use default
if (NOT UMFELD_OPENGL_VERSION)
    set(UMFELD_OPENGL_VERSION "${UMFELD_DEFAULT_OPENGL_VERSION}")
    message(STATUS "OPEN_GL(app)  : OPENGL_VERSION not set, using default: ${UMFELD_DEFAULT_OPENGL_VERSION}")
else ()
    message(STATUS "OPEN_GL(app)  : ${UMFELD_OPENGL_VERSION}")
endif ()

if (UMFELD_OPENGL_VERSION STREQUAL "core3.3" OR UMFELD_OPENGL_VERSION STREQUAL "3.3")
    set(OPENGL_CORE_3_3 TRUE)
elseif (UMFELD_OPENGL_VERSION STREQUAL "2.0")
    set(OPENGL_2_0 TRUE)
elseif (UMFELD_OPENGL_VERSION STREQUAL "es3.0")
    set(OPENGL_ES_3_0 TRUE)
else ()
    message(STATUS "Invalid OPENGL_VERSION: ${UMFELD_OPENGL_VERSION} must be `core3.3`, `2.0`, or `es3.0`")
    message(STATUS "using default: ${UMFELD_DEFAULT_OPENGL_VERSION}")
    set(OPENGL_CORE_3_3 TRUE)
endif ()

# set GLAD path and define
if (OPENGL_CORE_3_3)
    target_compile_definitions(umfeld-lib-interface INTERFACE OPENGL_CORE_3_3)
    target_compile_definitions(${PROJECT_NAME} PUBLIC OPENGL_CORE_3_3)
    set(GLAD_DIR "${UMFELD_GLAD_BASE_DIR}/glad_opengl_33")
    message(STATUS "OPEN_GL       : OPENGL_CORE_3_3")
elseif (OPENGL_2_0)
    target_compile_definitions(umfeld-lib-interface INTERFACE OPENGL_2_0)
    target_compile_definitions(${PROJECT_NAME} PUBLIC OPENGL_2_0)
    set(GLAD_DIR "${UMFELD_GLAD_BASE_DIR}/glad_opengl_20")
    message(STATUS "OPEN_GL       : OPENGL_2_0")
elseif (OPENGL_ES_3_0)
    target_compile_definitions(umfeld-lib-interface INTERFACE OPENGL_ES_3_0)
    target_compile_definitions(${PROJECT_NAME} PUBLIC OPENGL_ES_3_0)
    set(GLAD_DIR "${UMFELD_GLAD_BASE_DIR}/glad_opengles_30")
    message(STATUS "OPEN_GL       : OPENGL_ES_3_0")
endif ()

message(STATUS "GLAD_DIR      : ${GLAD_DIR}")

# add GLAD as library
file(GLOB GLAD_SRC "${GLAD_DIR}/src/*.c")
add_library(glad STATIC ${GLAD_SRC})
target_include_directories(glad PUBLIC "${GLAD_DIR}/include")

##########################################################################################################
### CREATE EXECUTABLE                                                                                  ###
##########################################################################################################

#if (BUILD_STANDALONE)
#    add_executable(${PROJECT_NAME} ${UMFELD_SOURCE_FILES})
#    target_compile_definitions(${PROJECT_NAME} PUBLIC UMFELD_STANDALONE)
#else (BUILD_STANDALONE)
target_sources(${PROJECT_NAME}
        PUBLIC
        ${UMFELD_SOURCE_FILES}
)
#endif (BUILD_STANDALONE)

##########################################################################################################
### CHECK SYSTEM TYPE                                                                                  ###
##########################################################################################################

if (APPLE)
    message(STATUS "SYSTEM        : macOS")
    target_compile_definitions(umfeld-lib-interface INTERFACE SYSTEM_MACOS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC SYSTEM_MACOS)
elseif (UNIX)
    message(STATUS "SYSTEM        : UNIX ( including RPI )")
    target_compile_definitions(umfeld-lib-interface INTERFACE SYSTEM_UNIX)
    target_compile_definitions(${PROJECT_NAME} PUBLIC SYSTEM_UNIX)
elseif (WIN32)
    message(STATUS "SYSTEM        : Windows")
    target_compile_definitions(umfeld-lib-interface INTERFACE SYSTEM_WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC SYSTEM_WIN32)
else ()
    message(STATUS "SYSTEM        : Undefined, Good Luck!")
    target_compile_definitions(umfeld-lib-interface INTERFACE SYSTEM_UNDEFINED)
    target_compile_definitions(${PROJECT_NAME} PUBLIC SYSTEM_UNDEFINED)
endif ()

##########################################################################################################
### SUPRESS WARNINGS FOR APPLE                                                                         ###
##########################################################################################################

if (APPLE)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON)
endif (APPLE)

##########################################################################################################
### FIND PACKAGES + LIBRARIES                                                                          ###
##########################################################################################################

find_package(PkgConfig REQUIRED)
if (WIN32)
elseif (APPLE)
elseif (UNIX)
    find_package(Threads REQUIRED)
else ()
endif ()

# GRAPHICS + VIDEO
if (NOT DISABLE_GRAPHICS)
    # OpenGL
    find_package(glm REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(SDL3 CONFIG REQUIRED)
    find_package(harfbuzz CONFIG REQUIRED)
    pkg_check_modules(FREETYPE REQUIRED freetype2)

    if (APPLE)
    elseif (UNIX)
    elseif (WIN32)
    else ()
        message(STATUS "plattform may not be supported.")
    endif ()

    # FFMPEG
    if (NOT DISABLE_VIDEO)
        pkg_check_modules(FFMPEG REQUIRED
                libavcodec
                libavformat
                libavutil
                libswscale
                libswresample
                libavdevice
        )
        if (APPLE)
        elseif (UNIX)
        elseif (WIN32)
        else ()
            message(STATUS "plattform may not be supported ... assuming windows.")
        endif ()
    endif (NOT DISABLE_VIDEO)
endif (NOT DISABLE_GRAPHICS)

# AUDIO
if (NOT DISABLE_AUDIO)
    if (ENABLE_PORTAUDIO)
        pkg_search_module(PORTAUDIO REQUIRED portaudio-2.0)
    else (ENABLE_PORTAUDIO)
        find_package(SDL3 CONFIG REQUIRED)
    endif (ENABLE_PORTAUDIO)
endif (NOT DISABLE_AUDIO)

# MIDI
if (NOT DISABLE_MIDI)
    pkg_search_module(RTMIDI REQUIRED rtmidi)
endif (NOT DISABLE_MIDI)

##########################################################################################################
### SUPRESS WARNINGS FOR APPLE                                                                         ###
##########################################################################################################

if (WIN32)
elseif (APPLE)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS OFF)
elseif (UNIX)
else ()
endif ()

##########################################################################################################
### INCLUDE + LINK LIBRARIES                                                                           ###
##########################################################################################################

if (HOMEBREW_LIB_PATH)
    target_link_directories(${PROJECT_NAME} PUBLIC ${HOMEBREW_LIB_PATH})
endif ()

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC wsock32 ws2_32 winmm)
elseif (APPLE)
elseif (UNIX)
    target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
else ()
endif ()

# GRAPHICS + VIDEO + AUDIO
if (NOT DISABLE_AUDIO)
    # TODO always add portaudio?!?
    target_link_libraries(${PROJECT_NAME} PUBLIC ${PORTAUDIO_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PUBLIC ${PORTAUDIO_INCLUDE_DIRS})
endif (NOT DISABLE_AUDIO)

if (NOT DISABLE_GRAPHICS OR NOT DISABLE_AUDIO)
    target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3) # NOTE not sure about these explicit lib names
    target_include_directories(${PROJECT_NAME} PUBLIC ${SDL3_INCLUDE_DIRS})
endif (NOT DISABLE_GRAPHICS OR NOT DISABLE_AUDIO)

if (NOT DISABLE_GRAPHICS)
    # OpenGL + GLAD + GLM
    target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENGL_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PUBLIC ${OPENGL_INCLUDE_DIRS})
#    target_link_libraries(${PROJECT_NAME} PUBLIC ${GLEW_LIBRARIES})
#    target_include_directories(${PROJECT_NAME} PUBLIC ${GLEW_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PUBLIC glad)
    #target_link_libraries(${PROJECT_NAME} PUBLIC ${GLM_LIBRARIES})
    #target_include_directories(${PROJECT_NAME} PUBLIC ${GLM_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)
    # font rendering
    target_link_libraries(${PROJECT_NAME} PUBLIC harfbuzz::harfbuzz)
    target_include_directories(${PROJECT_NAME} PUBLIC ${HARFBUZZ_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${FREETYPE_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PUBLIC ${FREETYPE_INCLUDE_DIRS})
    # FFMPEG
    if (NOT DISABLE_VIDEO)
        target_include_directories(${PROJECT_NAME} PUBLIC ${FFMPEG_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PUBLIC ${FFMPEG_LIBRARIES})
        if (APPLE)
            target_link_libraries(${PROJECT_NAME} PUBLIC
                    "-framework AVFoundation"
                    "-framework Foundation"
                    "-framework CoreMedia"
            )
        elseif (UNIX)
        elseif (WIN32)
            message(STATUS "plattform may not be supported")
        else ()
            message(STATUS "plattform may not be supported ... assuming windows.")
        endif ()
    endif (NOT DISABLE_VIDEO)
endif (NOT DISABLE_GRAPHICS)

# MIDI
if (NOT DISABLE_MIDI)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${RTMIDI_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PUBLIC ${RTMIDI_INCLUDE_DIRS})
    # add_definitions(${RTMIDI_CFLAGS_OTHER})
    # message(STATUS "RTMIDI_CFLAGS_OTHER: ${RTMIDI_CFLAGS_OTHER}")
    foreach (flag ${RTMIDI_CFLAGS_OTHER})
        if (flag MATCHES "^-D[^=]+(=.+)?$")  # only pass valid -D macros
            target_compile_definitions(${PROJECT_NAME} PUBLIC "${flag}")
        elseif (flag STREQUAL "-pthread")
            target_compile_options(${PROJECT_NAME} PUBLIC "${flag}")
        endif ()
    endforeach ()
    # target_compile_definitions(${PROJECT_NAME} PUBLIC ${RTMIDI_CFLAGS_OTHER})
endif (NOT DISABLE_MIDI)

##########################################################################################################
### COMPILER FLAGS                                                                                     ###
##########################################################################################################
function(add_umfeld_libs)
    # Check if umfeld-lib-interface target already exists
    if (NOT TARGET umfeld-lib-interface)
        message(FATAL_ERROR "umfeld-lib-interface target not found. Make sure it is defined before calling add_umfeld_libs.")
    endif ()

    # Check if umfeld-lib target already exists
    if (NOT TARGET umfeld-lib)
        message(FATAL_ERROR "umfeld-lib target not found. Make sure it is defined before calling add_umfeld_libs.")
    endif ()

    # Link the libraries if they exist
    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(${PROJECT_NAME} PRIVATE umfeld-lib-interface)
    target_link_libraries(${PROJECT_NAME} PRIVATE umfeld-lib)
endfunction()


##########################################################################################################
### TODO                                                                                               ###
##########################################################################################################

# TODO implement curses for headless mode
## ncurses
#if (UNIX)
#    find_package(Curses REQUIRED)
#    include_directories(${CURSES_INCLUDE_DIR})
#    target_link_libraries(${PROJECT_NAME} PUBLIC ${CURSES_LIBRARIES})
#endif (UNIX)
#if (WIN32)
#    target_link_libraries(${PROJECT_NAME} PUBLIC pdcurses)
#endif (WIN32)

##########################################################################################################
### NOTE                                                                                               ###
##########################################################################################################

# use these in projects:
# - prepare umfeld ( before `add_executable` ): `include(${UMFELD_PATH}/cmake/default-pre.cmake)`
# - prepare umfeld ( after `add_executable` ): `include(${UMFELD_PATH}/cmake/default-post.cmake)`

##########################################################################################################
### TEST                                                                                               ###
##########################################################################################################

option(BUILD_TESTING "Enable tests" OFF)

if (BUILD_TESTING)
    message(STATUS "TESTING       : enabled")

    enable_testing()

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for Mac OS X")

    add_executable(umfeld-tests test/umfeld_test.cpp)
    target_link_libraries(umfeld-tests umfeld-lib umfeld-lib-interface)

    add_test(NAME UmfeldTest COMMAND umfeld-tests)
else ()
endif ()

# run test from CLI with `cmake -B build -DBUILD_TESTING=ON ; cmake --build build ; ctest --test-dir build`

##########################################################################################################
### END OF SCRIPT                                                                                      ###
##########################################################################################################

message(STATUS "----------------------------------------")
