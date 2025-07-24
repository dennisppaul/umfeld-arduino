cmake_minimum_required(VERSION 3.20)

project(umfeld-lib VERSION 2.3.0)

message(STATUS "----------------------------------------")
message(STATUS "UMFELD LIBRARY")
message(STATUS "----------------------------------------")

option(DISABLE_GRAPHICS "Disable graphics support" OFF)
option(DISABLE_AUDIO "Disable audio ( SDL3 + portaudio ) support" OFF)
option(DISABLE_VIDEO "Disable video ( ffmpeg ) support" OFF)
option(DISABLE_PDF "Disable PDF export ( cairo )" OFF)
option(ENABLE_PORTAUDIO "Enable PortAudio output" ON) # TODO currently portaudio is the default audio engine
option(ENABLE_CAPTURE "Enable Capture" OFF)
option(ENABLE_ANGLE "Enable ANGLE OpenGL ES emulation on macOS ( apple silicon )" OFF)
option(DISABLE_NCURSES "Disable ncurses support" OFF)
option(DISABLE_OPENGL "Disable OpenGL support" OFF)
option(DISABLE_FONT "Disable font rendering ( harfbuzz + freetype ) support" OFF)
option(DISABLE_MIDI "Disable MIDI support" OFF)

option(DISABLE_MAIN "Disable main function" OFF)
option(ENABLE_PEDANTIC_COMPILER "Enable pedantic compiler warnings" OFF)

##########################################################################################################
### INTERNAL OPTIONS + VARIABLES                                                                       ###
##########################################################################################################

set(UMFELD_CXX_VERSION "cxx_std_17")
option(USE_ASCII_LOGGING "Disable UTF-8 symbols for log output" OFF)

##########################################################################################################
### PROPAGATE VERSION                                                                                  ###
##########################################################################################################

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/UmfeldVersion.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/UmfeldVersion.h"
        @ONLY
)

##########################################################################################################
### LIBRARY FUNCTIONS                                                                                  ###
##########################################################################################################

function(set_compile_definition_macro option definition_text)
    # TODO use this to replace `target_compile_definitions` calls in script
    #      target_compile_definitions(umfeld-lib-interface INTERFACE ...)
    #      target_compile_definitions(umfeld-lib PUBLIC ...)
    target_compile_definitions(umfeld-lib-interface INTERFACE ${option})
    target_compile_definitions(umfeld-lib PUBLIC ${option})
    message(STATUS "${definition_text}${option}")
endfunction()

function(setup_library_search_paths)
    # set up common search paths (call once)
    if (APPLE)
        # Homebrew
        find_program(BREW_FOUND brew)
        if (BREW_FOUND)
            execute_process(
                    COMMAND brew --prefix
                    OUTPUT_VARIABLE HOMEBREW_PREFIX
                    OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            set(HOMEBREW_LIB_PATH "${HOMEBREW_PREFIX}/lib")
            set(HOMEBREW_INCLUDE_PATH "${HOMEBREW_PREFIX}/include")
            set(ENV{PKG_CONFIG_PATH} "${HOMEBREW_LIB_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
            list(APPEND CMAKE_PREFIX_PATH "${HOMEBREW_PREFIX}")
            message(STATUS "HOMEBREW PRFX : ${HOMEBREW_PREFIX}")
            message(STATUS "HOMEBREW LIBS : ${HOMEBREW_LIB_PATH}")
            message(STATUS "HOMEBREW INCL : ${HOMEBREW_INCLUDE_PATH}")
        else ()
            message(STATUS "HOMEBREW      : not found")
        endif ()

        # MacPorts
        find_program(PORT_FOUND port)
        if (PORT_FOUND)
            execute_process(
                    COMMAND port dir
                    OUTPUT_VARIABLE MACPORTS_PREFIX
                    OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            set(MACPORTS_LIB_PATH "${MACPORTS_PREFIX}/lib")
            set(MACPORTS_INCLUDE_PATH "${MACPORTS_PREFIX}/include")
            set(ENV{PKG_CONFIG_PATH} "${MACPORTS_LIB_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
            list(APPEND CMAKE_PREFIX_PATH "${MACPORTS_PREFIX}")
            message(STATUS "MACPORTS PRFX : ${MACPORTS_PREFIX}")
            message(STATUS "MACPORTS LIBS : ${MACPORTS_LIB_PATH}")
            message(STATUS "MACPORTS INCL : ${MACPORTS_INCLUDE_PATH}")
        else ()
            message(STATUS "MACPORTS      : not found")
        endif ()

        # Warning if neither was found
        if (NOT BREW_FOUND AND NOT PORT_FOUND)
            message(WARNING "Neither HOMEBREW nor MACPORTS found. Build may fail.")
        endif ()

        # Propagate all useful paths
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
        set(HOMEBREW_PREFIX "${HOMEBREW_PREFIX}" CACHE INTERNAL "")
        set(HOMEBREW_LIB_PATH "${HOMEBREW_LIB_PATH}" CACHE INTERNAL "")
        set(HOMEBREW_INCLUDE_PATH "${HOMEBREW_INCLUDE_PATH}" CACHE INTERNAL "")
        set(MACPORTS_PREFIX "${MACPORTS_PREFIX}" CACHE INTERNAL "")
        set(MACPORTS_LIB_PATH "${MACPORTS_LIB_PATH}" CACHE INTERNAL "")
        set(MACPORTS_INCLUDE_PATH "${MACPORTS_INCLUDE_PATH}" CACHE INTERNAL "")
    endif ()
endfunction()

function(discover_library
        TARGET_NAME
        FIND_PACKAGE_NAME
        FIND_PACKAGE_TARGET
        PKG_CONFIG_NAME
        PKG_CONFIG_VAR
        MANUAL_LIB_NAMES
        MANUAL_HEADER_NAME
        LINK_SCOPE
)
    set(LIB_FOUND FALSE)
    set(FOUND_METHOD "")
    set(USING_DESC "")

    if (USE_ASCII_LOGGING)
        set(SYMBOL_OK "[OK]  ")
        set(SYMBOL_FAIL "[FAIL]")
    else ()
        set(SYMBOL_OK "✓")
        set(SYMBOL_FAIL "✗")
    endif ()

    find_package(${FIND_PACKAGE_NAME} QUIET)

    if (${FIND_PACKAGE_NAME}_FOUND)
        if (TARGET ${FIND_PACKAGE_TARGET})
            target_link_libraries(${TARGET_NAME} ${LINK_SCOPE} ${FIND_PACKAGE_TARGET})
            set(LIB_FOUND TRUE)
            set(FOUND_METHOD "'find_package'")
            set(USING_DESC "target")
        else ()
            string(TOUPPER ${FIND_PACKAGE_NAME} UPPER_NAME)
            if (DEFINED ${UPPER_NAME}_LIBRARIES)
                target_link_libraries(${TARGET_NAME} ${LINK_SCOPE} ${${UPPER_NAME}_LIBRARIES})
                if (DEFINED ${UPPER_NAME}_INCLUDE_DIRS)
                    target_include_directories(${TARGET_NAME} ${LINK_SCOPE} ${${UPPER_NAME}_INCLUDE_DIRS})
                endif ()
                set(LIB_FOUND TRUE)
                set(FOUND_METHOD "'find_package'")
                set(USING_DESC "variables")
            endif ()
        endif ()
    endif ()

    if (NOT LIB_FOUND)
        find_package(PkgConfig QUIET)
        if (PKG_CONFIG_FOUND)
            pkg_check_modules(${PKG_CONFIG_VAR} QUIET IMPORTED_TARGET ${PKG_CONFIG_NAME})
            if (${PKG_CONFIG_VAR}_FOUND)
                target_link_libraries(${TARGET_NAME} ${LINK_SCOPE} PkgConfig::${PKG_CONFIG_VAR})
                set(LIB_FOUND TRUE)
                set(FOUND_METHOD "'pkg-config'  ")
                set(USING_DESC "variables")
            endif ()
        endif ()
    endif ()

    if (NOT LIB_FOUND)
        if (APPLE)
            set(_FALLBACK_PATHS)
            if (DEFINED HOMEBREW_PREFIX)
                list(APPEND _FALLBACK_PATHS "${HOMEBREW_PREFIX}")
            endif ()
            if (DEFINED MACPORTS_PREFIX)
                list(APPEND _FALLBACK_PATHS "${MACPORTS_PREFIX}")
            endif ()
            list(APPEND _FALLBACK_PATHS "/usr/local")

            foreach (PREFIX IN LISTS _FALLBACK_PATHS)
                if (EXISTS "${PREFIX}/lib")
                    target_link_directories(${TARGET_NAME} ${LINK_SCOPE} "${PREFIX}/lib")
                endif ()
                if (EXISTS "${PREFIX}/include")
                    target_include_directories(${TARGET_NAME} ${LINK_SCOPE} "${PREFIX}/include")
                endif ()
            endforeach ()
        endif ()

        find_library(${PKG_CONFIG_VAR}_LIBRARY NAMES ${MANUAL_LIB_NAMES})
        find_path(${PKG_CONFIG_VAR}_INCLUDE_DIR ${MANUAL_HEADER_NAME})

        if (${PKG_CONFIG_VAR}_LIBRARY AND ${PKG_CONFIG_VAR}_INCLUDE_DIR)
            target_link_libraries(${TARGET_NAME} ${LINK_SCOPE} ${${PKG_CONFIG_VAR}_LIBRARY})
            target_include_directories(${TARGET_NAME} ${LINK_SCOPE} ${${PKG_CONFIG_VAR}_INCLUDE_DIR})
            set(LIB_FOUND TRUE)
            set(FOUND_METHOD "manual        ")
            set(USING_DESC "variables")
        endif ()
    endif ()

    set(${FIND_PACKAGE_NAME}_FOUND ${LIB_FOUND} PARENT_SCOPE)

    # -- Final printout --
    string(LENGTH "${FIND_PACKAGE_NAME}" name_len)
    math(EXPR pad_len "18 - ${name_len}")
    string(REPEAT " " ${pad_len} pad)

    if (LIB_FOUND)
        message(STATUS "    PACKAGE: ${FIND_PACKAGE_NAME}${pad}${SYMBOL_OK} found with ${FOUND_METHOD} ( ${USING_DESC} )")
    else ()
        message(STATUS "[PACKAGE]: ${FIND_PACKAGE_NAME}${pad}${SYMBOL_FAIL} not found")
        message(FATAL_ERROR "${FIND_PACKAGE_NAME} not found! Please install ${FIND_PACKAGE_NAME} development files.")
    endif ()
endfunction(discover_library)

setup_library_search_paths()

# @REMOVE -->
##########################################################################################################
### CHECK FOR HOMEBREW + MACPORTS                                                                      ###
##########################################################################################################

# NOTE merged this with `setup_library_search_paths` function
#if (APPLE)
#    find_program(BREW_FOUND brew)
#    if (BREW_FOUND)
#        execute_process(COMMAND brew --prefix
#                OUTPUT_VARIABLE HOMEBREW_PREFIX
#                OUTPUT_STRIP_TRAILING_WHITESPACE)
#        set(HOMEBREW_LIB_PATH "${HOMEBREW_PREFIX}/lib")
#        set(HOMEBREW_INCLUDE_PATH "${HOMEBREW_PREFIX}/include")
#        set(ENV{PKG_CONFIG_PATH} "${HOMEBREW_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
#        message(STATUS "HOMEBREW PRFX : ${HOMEBREW_PREFIX}")
#        message(STATUS "HOMEBREW LIBS : ${HOMEBREW_LIB_PATH}")
#        message(STATUS "HOMEBREW INCL : ${HOMEBREW_INCLUDE_PATH}")
#    else ()
#        message(STATUS "HOMEBREW      : not found")
#    endif ()
#
#    find_program(PORT_FOUND port)
#    if (PORT_FOUND)
#        execute_process(COMMAND port dir
#                OUTPUT_VARIABLE MACPORTS_PREFIX
#                OUTPUT_STRIP_TRAILING_WHITESPACE)
#        set(MACPORTS_LIB_PATH "${MACPORTS_PREFIX}/lib")
#        set(MACPORTS_INCLUDE_PATH "${MACPORTS_PREFIX}/include")
#        set(ENV{PKG_CONFIG_PATH} "${MACPORTS_LIB_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
#        message(STATUS "MACPORTS PRFX : ${MACPORTS_PREFIX}")
#        message(STATUS "MACPORTS LIBS : ${MACPORTS_LIB_PATH}")
#        message(STATUS "MACPORTS INCL : ${MACPORTS_INCLUDE_PATH}")
#    else ()
#        message(STATUS "MACPORTS      : not found")
#    endif ()
#
#    # TODO @dpp(not sure if it is necessary to set include paths like this. shouldn’t this be covered by 'find_package' or 'pkg_check_modules')
#
#    # NOTE combined warning for both package managers
#    if (NOT BREW_FOUND AND NOT PORT_FOUND)
#        message(WARNING "Neither HOMEBREW nor MACPORTS found. Build may fail.")
#    endif ()
#endif (APPLE)

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
# <-- @REMOVE

##########################################################################################################
### ADD SOURCE + HEADER FILES                                                                          ###
##########################################################################################################

set(_UMF_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# common (platform-agnostic) patterns

set(_UMF_PATTERNS
        ${_UMF_BASE_DIR}/*.cpp
        ${_UMF_BASE_DIR}/libraries/*.cpp
        ${_UMF_BASE_DIR}/libraries/*.c
        ${_UMF_BASE_DIR}/libraries/libtess2/*.c
        ${_UMF_BASE_DIR}/libraries/clipper/*.cpp
        ${_UMF_BASE_DIR}/libraries/ip/*.cpp
        ${_UMF_BASE_DIR}/libraries/osc/*.cpp
        ${_UMF_BASE_DIR}/libraries/pffft/*.c
)

# platform-specific additions

if (APPLE)
    list(APPEND _UMF_PATTERNS
            ${_UMF_BASE_DIR}/libraries/ip/posix/*.cpp
    )
    if (ENABLE_CAPTURE)
        list(APPEND _UMF_PATTERNS
                ${_UMF_BASE_DIR}/Capture.mm
        )
    endif (ENABLE_CAPTURE)
elseif (WIN32)
    list(APPEND _UMF_PATTERNS
            ${_UMF_BASE_DIR}/libraries/ip/win32/*.cpp
    )
else () # other UNIX-ish platforms default to posix
    list(APPEND _UMF_PATTERNS
            ${_UMF_BASE_DIR}/libraries/ip/posix/*.cpp
    )
endif ()

file(GLOB UMFELD_SOURCE_FILES CONFIGURE_DEPENDS ${_UMF_PATTERNS})

if (NOT TARGET umfeld-lib-interface)
    add_library(umfeld-lib-interface INTERFACE)
    target_compile_features(umfeld-lib-interface INTERFACE ${UMFELD_CXX_VERSION})
endif ()

if (BUILD_STANDALONE)
    add_executable(umfeld-main ${UMFELD_SOURCE_FILES})
    target_compile_definitions(umfeld-main PUBLIC UMFELD_STANDALONE)
    target_compile_features(umfeld-main PUBLIC ${UMFELD_CXX_VERSION})
else ()
    if (NOT TARGET umfeld-lib)
        add_library(umfeld-lib ${UMFELD_SOURCE_FILES})
        target_compile_features(umfeld-lib PUBLIC ${UMFELD_CXX_VERSION})
    endif ()
endif ()

# specify include directories incl vendored libraries

target_include_directories(umfeld-lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/shaders
        ${CMAKE_CURRENT_SOURCE_DIR}/include/libraries
        ${CMAKE_CURRENT_SOURCE_DIR}/include/libraries/dr_libs
        ${CMAKE_CURRENT_SOURCE_DIR}/include/libraries/ip
        ${CMAKE_CURRENT_SOURCE_DIR}/include/libraries/libtess2
        ${CMAKE_CURRENT_SOURCE_DIR}/include/libraries/osc
        ${CMAKE_CURRENT_SOURCE_DIR}/include/libraries/pffft
)

##########################################################################################################
### GLOBAL VARIABLES + EN/DISABLE LIBRARIES/MODULES                                                    ###
##########################################################################################################

# application name

if (NOT DEFINED UMFELD_APP_NAME)
    set(UMFELD_APP_NAME "${CMAKE_PROJECT_NAME}")
endif ()
#target_compile_definitions(umfeld-lib PUBLIC UMFELD_APP_NAME=\"${UMFELD_APP_NAME}\")
#message(STATUS "APP NAME      : ${UMFELD_APP_NAME}")
set_compile_definition_macro(UMFELD_APP_NAME=\"${UMFELD_APP_NAME}\" "APP NAME      : ")

# module options

if (DISABLE_MAIN)
    message(STATUS "DISABLE_MAIN  : no `main` function entry point. must be defined in project.")
else ()
    target_compile_definitions(umfeld-lib PUBLIC SDL_MAIN_USE_CALLBACKS)
endif (DISABLE_MAIN)

if (ENABLE_PEDANTIC_COMPILER)
    message(STATUS "ENABLE_PEDAN… : enable pedantic compiler warnings.")
    target_compile_options(umfeld-lib PRIVATE
            -Wall # enable all warnings
            -Wextra # enable extra warnings
            -Wpedantic # enable pedantic warnings
#            -Werror # treat warnings as errors
    )
endif (ENABLE_PEDANTIC_COMPILER)

function(define_option_macro option enable_text)
    if (${option})
        target_compile_definitions(umfeld-lib-interface INTERFACE ${option})
        target_compile_definitions(umfeld-lib PUBLIC ${option})
        message(STATUS "${enable_text}")
    endif ()
endfunction()

define_option_macro(DISABLE_GRAPHICS "DISABLING     : graphics")
define_option_macro(DISABLE_AUDIO "DISABLING     : audio")
define_option_macro(DISABLE_VIDEO "DISABLING     : video")
define_option_macro(DISABLE_PDF "DISABLING     : pdf")
define_option_macro(ENABLE_PORTAUDIO "ENABLING      : PortAudio Driver")
define_option_macro(ENABLE_CAPTURE "ENABLING      : Capture")
define_option_macro(ENABLE_ANGLE "ENABLING      : ANGLE")
define_option_macro(DISABLE_NCURSES "DISABLING     : ncurses")
define_option_macro(DISABLE_OPENGL "DISABLING     : OpenGL")
define_option_macro(DISABLE_FONT "DISABLING     : font support with harfbuzz + freetype")
define_option_macro(DISABLE_MIDI "DISABLING     : disable MIDI")

##########################################################################################################
### CHECK SYSTEM TYPE                                                                                  ###
##########################################################################################################

# possible options:
#
# - SYSTEM_MACOS
# - SYSTEM_UNIX
# - SYSTEM_UNIX_RASPBERRY_PI + SYSTEM_UNIX
# - SYSTEM_WINDOWS
# - SYSTEM_UNDEFINED

if (APPLE)
    message(STATUS "SYSTEM        : macOS")
    target_compile_definitions(umfeld-lib-interface INTERFACE SYSTEM_MACOS)
    target_compile_definitions(umfeld-lib PUBLIC SYSTEM_MACOS)
elseif (UNIX)
    # default to UNIX
    set(SYSTEM_TYPE "UNIX")
    set(SYSTEM_DEFINE SYSTEM_UNIX)
    # try to detect Raspberry Pi
    if (EXISTS "/proc/device-tree/model")
        file(READ "/proc/device-tree/model" DEVICE_MODEL)
        string(TOLOWER "${DEVICE_MODEL}" DEVICE_MODEL_LOWER)
        if (DEVICE_MODEL_LOWER MATCHES "raspberry")
            set(RASPBERRY_PI TRUE)
        endif ()
    endif ()
    if (RASPBERRY_PI)
        message(STATUS "SYSTEM        : ${SYSTEM_TYPE} ( on Raspberry Pi )")
        target_compile_definitions(umfeld-lib-interface INTERFACE SYSTEM_UNIX_RASPBERRY_PI)
        target_compile_definitions(umfeld-lib PUBLIC SYSTEM_UNIX_RASPBERRY_PI)
    else ()
        message(STATUS "SYSTEM        : ${SYSTEM_TYPE}")
    endif ()
    target_compile_definitions(umfeld-lib-interface INTERFACE ${SYSTEM_DEFINE})
    target_compile_definitions(umfeld-lib PUBLIC ${SYSTEM_DEFINE})
elseif (WIN32)
    message(STATUS "SYSTEM        : Windows")
    target_compile_definitions(umfeld-lib-interface INTERFACE SYSTEM_WINDOWS)
    target_compile_definitions(umfeld-lib PUBLIC SYSTEM_WINDOWS)
else ()
    message(STATUS "SYSTEM        : Undefined, Good Luck!")
    target_compile_definitions(umfeld-lib-interface INTERFACE SYSTEM_UNDEFINED)
    target_compile_definitions(umfeld-lib PUBLIC SYSTEM_UNDEFINED)
endif ()

##########################################################################################################
### OPEN_GL VERSION + GLAD (vendored)                                                                  ###
##########################################################################################################

# possible options:
#
# - OPENGL_3_3_CORE
# - OPENGL_2_0
# - OPENGL_ES_3_0

if (NOT DISABLE_GRAPHICS)
    if (NOT DISABLE_OPENGL)
        if (RASPBERRY_PI)
            set(UMFELD_DEFAULT_OPENGL_VERSION "OPENGL_ES_3_0" CACHE STRING "OpenGL version used in umfeld")
        else ()
            set(UMFELD_DEFAULT_OPENGL_VERSION "OPENGL_3_3_CORE" CACHE STRING "OpenGL version used in umfeld")
        endif ()
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
            message(STATUS "OPEN_GL(app)  : ${UMFELD_DEFAULT_OPENGL_VERSION} ( UMFELD_OPENGL_VERSION not set, using default )")
        else ()
            message(STATUS "OPEN_GL(app)  : ${UMFELD_OPENGL_VERSION}")
        endif ()

        if (UMFELD_OPENGL_VERSION STREQUAL "OPENGL_3_3_CORE" OR UMFELD_OPENGL_VERSION STREQUAL "3.3")
            set(OPENGL_3_3_CORE TRUE)
        elseif (UMFELD_OPENGL_VERSION STREQUAL "OPENGL_2_0")
            set(OPENGL_2_0 TRUE)
        elseif (UMFELD_OPENGL_VERSION STREQUAL "OPENGL_ES_3_0" OR UMFELD_OPENGL_VERSION STREQUAL "OPENGL_ES_3_0")
            set(OPENGL_ES_3_0 TRUE)
        else ()
            message(STATUS "Invalid OPENGL_VERSION: ${UMFELD_OPENGL_VERSION} must be `OPENGL_3_3_CORE`, `OPENGL_2_0`, or `OPENGL_ES_3_0`")
            message(STATUS "using default: ${UMFELD_DEFAULT_OPENGL_VERSION}")
            set(OPENGL_3_3_CORE TRUE)
        endif ()

        # GLAD

        # set GLAD path and define according to OpenGL version
        if (OPENGL_3_3_CORE)
            target_compile_definitions(umfeld-lib-interface INTERFACE OPENGL_3_3_CORE)
            target_compile_definitions(umfeld-lib PUBLIC OPENGL_3_3_CORE)
            set(GLAD_DIR "${UMFELD_GLAD_BASE_DIR}/glad_opengl_33")
            message(STATUS "OPEN_GL(GLAD) : OPENGL_3_3_CORE")
        elseif (OPENGL_2_0)
            target_compile_definitions(umfeld-lib-interface INTERFACE OPENGL_2_0)
            target_compile_definitions(umfeld-lib PUBLIC OPENGL_2_0)
            set(GLAD_DIR "${UMFELD_GLAD_BASE_DIR}/glad_opengl_20")
            message(STATUS "OPEN_GL(GLAD) : OPENGL_2_0")
        elseif (OPENGL_ES_3_0)
            target_compile_definitions(umfeld-lib-interface INTERFACE OPENGL_ES_3_0)
            target_compile_definitions(umfeld-lib PUBLIC OPENGL_ES_3_0)
            set(GLAD_DIR "${UMFELD_GLAD_BASE_DIR}/glad_opengles_30")
            message(STATUS "OPEN_GL       : OPENGL_ES_3_0")
            message(STATUS "OPEN_GL(GLAD) : OPENGL_ES_3_0")
        endif ()
        # add GLAD as library
        file(GLOB GLAD_SRC "${GLAD_DIR}/../*.c")
        add_library(glad STATIC ${GLAD_SRC})
        if (OPENGL_3_3_CORE)
            target_compile_definitions(glad PRIVATE OPENGL_3_3_CORE)
        elseif (OPENGL_2_0)
            target_compile_definitions(glad PRIVATE OPENGL_2_0)
        elseif (OPENGL_ES_3_0)
            target_compile_definitions(glad PRIVATE OPENGL_ES_3_0)
        else ()
            message(WARNING "OPENGL_VERSION not defined. for GLAD to work it must be either OPENGL_2_0, OPENGL_ES_3_0 or OPENGL_3_3_CORE.")
        endif ()
        target_include_directories(glad PUBLIC "${GLAD_DIR}/include")

        target_link_libraries(umfeld-lib PUBLIC glad)
    endif (NOT DISABLE_OPENGL)
endif (NOT DISABLE_GRAPHICS)

##########################################################################################################
### SUPRESS WARNINGS FOR APPLE                                                                         ###
##########################################################################################################

if (APPLE)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON)
endif (APPLE)

##########################################################################################################
### FIND PACKAGES + LIBRARIES                                                                          ###
##########################################################################################################

# SYSTEM

# @REMOVE -->
find_package(PkgConfig REQUIRED) # TODO check if this is redundant and now handled in `discover_library`
# <-- @REMOVE
if (WIN32)
    target_link_libraries(umfeld-lib PUBLIC wsock32 ws2_32 winmm)
elseif (APPLE)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS OFF)
elseif (UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(umfeld-lib PUBLIC Threads::Threads)
endif ()

message(STATUS "START FINDING PACKAGES ...")

discover_library(
        umfeld-lib
        CURL
        CURL::libcurl
        libcurl
        CURL
        "curl"
        "curl/curl.h"
        PUBLIC
)

# GRAPHICS + FONT + OPENGL

if (NOT DISABLE_GRAPHICS)
    # TODO maybe move glm out of `DISABLE_GRAPHICS`
    discover_library(
            umfeld-lib
            glm             # find_package name
            glm::glm        # modern target name
            glm             # pkg-config name
            GLM             # variable prefix
            ""              # manual lib names ("no library to link")
            "glm/glm.hpp"   # manual header
            PUBLIC          # link scope ( PUBLIC since it's header-only )
    )
    target_compile_definitions(umfeld-lib-interface INTERFACE GLM_ENABLE_EXPERIMENTAL)
    target_compile_definitions(umfeld-lib PUBLIC GLM_ENABLE_EXPERIMENTAL)

    discover_library(       # TODO maybe we need to handle SDL3 differently
            umfeld-lib
            SDL3            # find_package name
            SDL3::SDL3      # modern target name
            sdl3            # pkg-config name
            SDL3            # variable prefix
            "SDL3"          # manual lib names
            "SDL3/SDL.h"    # manual header
            PUBLIC          # link scope
    )

    if (NOT DISABLE_FONT)
        discover_library(       # TODO does freetype require OpenGL
                umfeld-lib
                Freetype
                Freetype::Freetype
                freetype2
                FREETYPE
                "freetype"
                "ft2build.h"
                PUBLIC
        )
        discover_library(
                umfeld-lib
                HarfBuzz
                harfbuzz::harfbuzz
                harfbuzz
                HARFBUZZ
                "harfbuzz"
                "harfbuzz/hb.h"
                PUBLIC
        )
    endif (NOT DISABLE_FONT)

    if (NOT DISABLE_OPENGL)
        discover_library(
                umfeld-lib
                OpenGL
                OpenGL::GL
                gl
                OPENGL
                "GL"
                "GL/gl.h"
                PUBLIC
        )
    endif (NOT DISABLE_OPENGL)
endif (NOT DISABLE_GRAPHICS)

# AUDIO

if (NOT DISABLE_AUDIO)
    discover_library(
            umfeld-lib
            SDL3            # find_package name
            SDL3::SDL3      # modern target name
            sdl3            # pkg-config name
            SDL3            # variable prefix
            "SDL3"          # manual lib names
            "SDL3/SDL.h"    # manual header
            PUBLIC          # link scope
    )
    if (ENABLE_PORTAUDIO)
        discover_library(
                umfeld-lib
                PortAudio
                PortAudio::PortAudio
                portaudio-2.0
                PORTAUDIO
                "portaudio"
                "portaudio.h"
                PUBLIC
        )
    endif (ENABLE_PORTAUDIO)
endif (NOT DISABLE_AUDIO)

# PDF

if (NOT DISABLE_PDF)
    discover_library(
            umfeld-lib
            Cairo
            Cairo::Cairo
            cairo
            CAIRO
            "cairo"
            "cairo/cairo-pdf.h"
            PUBLIC
    )
endif (NOT DISABLE_PDF)

# NCURSES

if (NOT DISABLE_NCURSES)
    if (WIN32)
        discover_library(
                umfeld-lib
                Curses
                Curses::Curses
                pdcurses
                PDCURSES
                "pdcurses"
                "curses.h"
                PUBLIC
        )
    else ()
        discover_library(
                umfeld-lib
                Curses
                Curses::Curses
                ncurses
                NCURSES
                "ncurses"
                "curses.h"
                PUBLIC
        )
    endif ()
endif (NOT DISABLE_NCURSES)

# FFMPEG

if (NOT DISABLE_VIDEO)
    # NOTE FFMPEG needs to be handled manually because of its multiple libraries
    pkg_check_modules(FFMPEG REQUIRED
            libavcodec
            libavformat
            libavutil
            libswscale
            libswresample
            libavdevice
    )
    target_link_directories(umfeld-lib PUBLIC ${FFMPEG_LIBRARY_DIRS})
    target_include_directories(umfeld-lib PUBLIC ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(umfeld-lib PUBLIC ${FFMPEG_LIBRARIES})
    if (APPLE)
        target_link_libraries(umfeld-lib PUBLIC
                "-framework AVFoundation"
                "-framework Foundation"
                "-framework CoreMedia"
        )
    endif ()
endif (NOT DISABLE_VIDEO)

# MIDI

if (NOT DISABLE_MIDI)
    #    discover_library(
    #            umfeld-lib             # target to link to
    #            RtMidi                 # find_package name
    #            RtMidi::rtmidi         # modern CMake target (provided by RtMidi)
    #            rtmidi                 # pkg-config name
    #            RTMIDI                 # variable prefix
    #            "rtmidi"               # manual library name
    #            "RtMidi.h"             # manual header
    #            PRIVATE                # link scope
    #    )
    # NOTE rtmidi requires some manual handling somehow `pkg_check_modules` seems to fail
    pkg_search_module(RTMIDI REQUIRED rtmidi)
    target_link_directories(umfeld-lib PUBLIC ${RTMIDI_LIBRARY_DIRS})
    target_link_libraries(umfeld-lib PUBLIC ${RTMIDI_LIBRARIES})
    target_include_directories(umfeld-lib PUBLIC ${RTMIDI_INCLUDE_DIRS})
    foreach (flag ${RTMIDI_CFLAGS_OTHER})
        if (flag MATCHES "^-D[^=]+(=.+)?$")  # only pass valid -D macros
            target_compile_definitions(umfeld-lib PUBLIC "${flag}")
        elseif (flag STREQUAL "-pthread")
            target_compile_options(umfeld-lib PUBLIC "${flag}")
        endif ()
    endforeach ()
endif (NOT DISABLE_MIDI)

message(STATUS "... DONE")

# GRAPHICS + VIDEO
#if (NOT DISABLE_GRAPHICS)
# OpenGL
#    find_package(glm REQUIRED)
#    find_package(OpenGL REQUIRED)
#    find_package(SDL3 CONFIG REQUIRED)

# font rendering
#    find_package(harfbuzz CONFIG REQUIRED)
#    pkg_check_modules(FREETYPE REQUIRED freetype2)

#    # PDF export
#    if (DISABLE_PDF)
#        message(STATUS "PDF_EXPORT    : DISABLED")
#    else (DISABLE_PDF)
#        message(STATUS "PDF_EXPORT    : ENABLED")
#        pkg_check_modules(CAIRO REQUIRED cairo)
#    endif (DISABLE_PDF)

#    # Ncurses
#    if (DISABLE_NCURSES)
#        message(STATUS "NCURSES(TERM) : DISABLED")
#    else (DISABLE_NCURSES)
#        message(STATUS "NCURSES(TERM) : ENABLED")
#        # find_package(Curses REQUIRED)
#        pkg_check_modules(NCURSES REQUIRED ncurses)
#        pkg_check_modules(NCURSES_PKG ncurses)
#
#        if (NCURSES_PKG_FOUND)
#            set(NCURSES_INCLUDE_DIR ${NCURSES_PKG_INCLUDE_DIRS})
#            set(NCURSES_LIBRARY ${NCURSES_PKG_LIBRARIES})
#        else ()
#            find_package(Curses REQUIRED)
#            set(NCURSES_INCLUDE_DIR ${CURSES_INCLUDE_DIR})
#            set(NCURSES_LIBRARY ${CURSES_LIBRARIES})
#        endif ()
#    endif (DISABLE_NCURSES)

# TODO check if ncurses needs extra treatment on other platforms
#if (UNIX)
#    find_package(Curses REQUIRED)
#    include_directories(${CURSES_INCLUDE_DIR})
#    target_link_libraries(umfeld-lib PUBLIC ${CURSES_LIBRARIES})
#endif (UNIX)
#if (WIN32)
#    target_link_libraries(umfeld-lib PUBLIC pdcurses)
#endif (WIN32)

# curl
#    pkg_check_modules(CURL REQUIRED libcurl)
#    find_package(CURL REQUIRED)
#    if (APPLE)
#    elseif (UNIX)
#    elseif (WIN32)
#    else ()
#        message(STATUS "platform may not be supported.")
#    endif ()

#    # FFMPEG
#    if (NOT DISABLE_VIDEO)
#        pkg_check_modules(FFMPEG REQUIRED
#                libavcodec
#                libavformat
#                libavutil
#                libswscale
#                libswresample
#                libavdevice
#        )
#        if (APPLE)
#        elseif (UNIX)
#        elseif (WIN32)
#        else ()
#            message(STATUS "platform may not be supported ... assuming windows.")
#        endif ()
#    endif (NOT DISABLE_VIDEO)
#endif (NOT DISABLE_GRAPHICS)

## AUDIO
#if (NOT DISABLE_AUDIO)
#    if (ENABLE_PORTAUDIO)
#        pkg_search_module(PORTAUDIO REQUIRED portaudio-2.0)
#    else (ENABLE_PORTAUDIO)
#        #        find_package(SDL3 CONFIG REQUIRED)
#    endif (ENABLE_PORTAUDIO)
#endif (NOT DISABLE_AUDIO)

# MIDI
#if (NOT DISABLE_MIDI)
#    pkg_search_module(RTMIDI REQUIRED rtmidi)
#endif (NOT DISABLE_MIDI)

##########################################################################################################
### SUPRESS WARNINGS FOR APPLE                                                                         ###
##########################################################################################################

#if (WIN32)
#elseif (APPLE)
#    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS OFF)
#elseif (UNIX)
#else ()
#endif ()

##########################################################################################################
### INCLUDE + LINK LIBRARIES                                                                           ###
##########################################################################################################

# @REMOVE -->
#if (HOMEBREW_LIB_PATH)
#    target_link_directories(umfeld-lib PUBLIC ${HOMEBREW_LIB_PATH})
#    target_include_directories(umfeld-lib PUBLIC ${HOMEBREW_INCLUDE_PATH})
#endif ()
#
#if (MACPORTS_LIB_PATH)
#    target_link_directories(umfeld-lib PUBLIC ${MACPORTS_LIB_PATH})
#    target_include_directories(umfeld-lib PUBLIC ${MACPORTS_INCLUDE_PATH})
#endif ()
# <-- @REMOVE

# SYSTEM
#if (WIN32)
#    target_link_libraries(umfeld-lib PUBLIC wsock32 ws2_32 winmm)
#elseif (APPLE)
#elseif (UNIX)
#    target_link_libraries(umfeld-lib PUBLIC Threads::Threads)
#else ()
#endif ()

## GRAPHICS + VIDEO + AUDIO
#if (NOT DISABLE_AUDIO)
#    if (ENABLE_PORTAUDIO)
#        target_link_libraries(umfeld-lib PUBLIC ${PORTAUDIO_LIBRARIES})
#        target_include_directories(umfeld-lib PUBLIC ${PORTAUDIO_INCLUDE_DIRS})
#    endif (ENABLE_PORTAUDIO)
#endif (NOT DISABLE_AUDIO)

#if (NOT DISABLE_GRAPHICS OR NOT DISABLE_AUDIO)
#    target_link_libraries(umfeld-lib PUBLIC SDL3::SDL3) # NOTE not sure about these explicit lib names
#    target_include_directories(umfeld-lib PUBLIC ${SDL3_INCLUDE_DIRS})
#endif (NOT DISABLE_GRAPHICS OR NOT DISABLE_AUDIO)

#if (NOT DISABLE_GRAPHICS)
# GLM
#    target_compile_definitions(umfeld-lib PUBLIC GLM_ENABLE_EXPERIMENTAL)
#    target_compile_definitions(umfeld-lib-interface INTERFACE GLM_ENABLE_EXPERIMENTAL)
# OpenGL + GLAD + GLM
#    target_link_libraries(umfeld-lib PUBLIC ${OPENGL_LIBRARIES})
#    target_include_directories(umfeld-lib PUBLIC ${OPENGL_INCLUDE_DIRS})
#    target_link_libraries(umfeld-lib PUBLIC glad)
#    target_link_libraries(umfeld-lib PUBLIC glm::glm)
# font rendering
#    target_link_libraries(umfeld-lib PUBLIC harfbuzz::harfbuzz)
#    target_include_directories(umfeld-lib PUBLIC ${HARFBUZZ_INCLUDE_DIRS})
#    target_link_libraries(umfeld-lib PUBLIC ${FREETYPE_LIBRARIES})
#    target_include_directories(umfeld-lib PUBLIC ${FREETYPE_INCLUDE_DIRS})
#    # FFMPEG
#    if (NOT DISABLE_VIDEO)
#        target_include_directories(umfeld-lib PUBLIC ${FFMPEG_INCLUDE_DIRS})
#        target_link_libraries(umfeld-lib PUBLIC ${FFMPEG_LIBRARIES})
#        if (APPLE)
#            target_link_libraries(umfeld-lib PUBLIC
#                    "-framework AVFoundation"
#                    "-framework Foundation"
#                    "-framework CoreMedia"
#            )
#        elseif (UNIX)
#        elseif (WIN32)
#            message(STATUS "platform may not be supported")
#        else ()
#            message(STATUS "platform may not be supported ... assuming windows.")
#        endif ()
#    endif (NOT DISABLE_VIDEO)

# PDF export
#    if (NOT DISABLE_PDF)
#        target_include_directories(umfeld-lib PUBLIC ${CAIRO_INCLUDE_DIRS})
#        target_link_libraries(umfeld-lib PUBLIC ${CAIRO_LIBRARIES})
#    endif (NOT DISABLE_PDF)

#    if (NOT DISABLE_NCURSES)
#        target_include_directories(umfeld-lib PRIVATE ${NCURSES_INCLUDE_DIRS})
#        target_link_libraries(umfeld-lib PRIVATE ${NCURSES_LIBRARY})
#    endif (NOT DISABLE_NCURSES)

#endif (NOT DISABLE_GRAPHICS)

# MIDI
#if (NOT DISABLE_MIDI)
#    target_link_libraries(umfeld-lib PUBLIC ${RTMIDI_LIBRARIES})
#    target_include_directories(umfeld-lib PUBLIC ${RTMIDI_INCLUDE_DIRS})
#    foreach (flag ${RTMIDI_CFLAGS_OTHER})
#        if (flag MATCHES "^-D[^=]+(=.+)?$")  # only pass valid -D macros
#            target_compile_definitions(umfeld-lib PUBLIC "${flag}")
#        elseif (flag STREQUAL "-pthread")
#            target_compile_options(umfeld-lib PUBLIC "${flag}")
#        endif ()
#    endforeach ()
#endif (NOT DISABLE_MIDI)

#target_include_directories(umfeld-lib PUBLIC ${CURL_INCLUDE_DIRS})
#target_link_libraries(umfeld-lib PUBLIC ${CURL_LIBRARIES})
#target_link_libraries(umfeld-lib PUBLIC CURL::libcurl)

##########################################################################################################
### ENTRY POINT FOR APPLICATIONS SCRIPT                                                                ###
##########################################################################################################

function(add_umfeld_libs)
    # Check if umfeld-lib-interface target already exists

    if (NOT TARGET umfeld-lib-interface)
        message(FATAL_ERROR "umfeld-lib-interface target not found. Make sure to include Umfeld eith `add_subdirectory(UMFELD_PATH...` before calling `add_umfeld_libs()`.")
    endif ()

    # Check if umfeld-lib target already exists

    if (NOT TARGET umfeld-lib)
        message(FATAL_ERROR "umfeld-lib target not found. Make sure to include Umfeld eith `add_subdirectory(UMFELD_PATH...` before calling `add_umfeld_libs()`.")
    endif ()

    # Link the libraries if they exist

    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(${PROJECT_NAME} PRIVATE umfeld-lib-interface)
    target_link_libraries(${PROJECT_NAME} PRIVATE umfeld-lib)

    # @REMOVE -->
    # Add library + include search paths for macOS package managers
    if (APPLE)
        # NOTE this sets link directories for calling application
        if (HOMEBREW_LIB_PATH)
            target_link_directories(${PROJECT_NAME} PRIVATE ${HOMEBREW_LIB_PATH})
        endif ()
        if (HOMEBREW_INCLUDE_PATH)
            target_include_directories(${PROJECT_NAME} PRIVATE ${HOMEBREW_INCLUDE_DIRS})
        endif ()
        if (MACPORTS_LIB_PATH)
            target_link_directories(${PROJECT_NAME} PRIVATE ${MACPORTS_LIB_PATH})
        endif ()
        if (MACPORTS_INCLUDE_PATH)
            target_include_directories(${PROJECT_NAME} PRIVATE ${MACPORTS_INCLUDE_PATH})
        endif ()
    endif ()
    # <-- @REMOVE

    # TODO maybe make this more platform specific?
    # pffft SIMD detection

    if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        message(STATUS "PROCESSOR     : ARM/NEON")
        add_compile_definitions(PFFFT_SIMD_DISABLE) # pffft enables NEON by default
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86" OR CMAKE_SYSTEM_PROCESSOR MATCHES "amd64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "i686")
        message(STATUS "PROCESSOR     : x86/SSE detected")
        add_compile_definitions(PFFFT_SIMD_ENABLE)
        if (MSVC)
            add_compile_options(/arch:SSE2)
        else ()
            add_compile_options(-msse2 -mfpmath=sse)
        endif ()
    endif ()

    # copy data folder resources

    get_target_property(SKETCH_SOURCE_DIR ${PROJECT_NAME} SOURCE_DIR)
    set(DATA_DIR_SRC "${SKETCH_SOURCE_DIR}/data")
    set(DATA_DIR_DST "${CMAKE_CURRENT_BINARY_DIR}/data")

    if (EXISTS "${DATA_DIR_SRC}")
        message(STATUS "'data' FOLDER : found 'data' folder, copying to build directory from")
        message(STATUS "                '${DATA_DIR_SRC}' to")
        message(STATUS "                '${DATA_DIR_DST}'.")
        add_custom_command(
                TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${DATA_DIR_SRC}" "${DATA_DIR_DST}"
                COMMENT "Copying data folder to output directory"
                COMMAND ${CMAKE_COMMAND} -E echo "--- copied data folder to build directory."
        )
    else ()
        message(STATUS "'data' FOLDER : not found")
    endif ()

    message(STATUS "----------------------------------------")
endfunction()

##########################################################################################################
### TEST                                                                                               ###
##########################################################################################################

# run test from CLI with `cmake -B build -DBUILD_TESTING=ON ; cmake --build build ; ctest --test-dir build`

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
endif ()

##########################################################################################################
### ANGLE                                                                                              ###
##########################################################################################################

if (NOT DISABLE_GRAPHICS)
    if (ENABLE_ANGLE)
        if (APPLE)
            # TODO maybe move this into library folder(s)?
            set(UMFELD_ANGLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/angle" CACHE STRING "ANGLE directory")
            message(STATUS "ANGLE_SUPPORT : ENABLED")
            message(STATUS "ANGLE_DIR     : ${UMFELD_ANGLE_DIR}")

            add_custom_command(TARGET umfeld-lib POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${UMFELD_ANGLE_DIR}/libEGL.dylib
                    ${UMFELD_ANGLE_DIR}/libGLESv2.dylib
                    # NOTE this relies on the build output of `umfeld-lib` being placed inside a subfolder
                    #      like `${CMAKE_BINARY_DIR}/umfeld-lib-umfeld-lib` so that copying ../ lands in app dir
                    $<TARGET_FILE_DIR:umfeld-lib>/..
            )
        else ()
            message(STATUS "ANGLE_SUPPORT : DISABLED ( not supported on this platform )")
        endif (APPLE)
    endif (ENABLE_ANGLE)

    if (APPLE AND UMFELD_OPENGL_VERSION STREQUAL "OPENGL_ES_3_0" AND NOT ENABLE_ANGLE)
        message(WARNING "OpenGL ES 3.0 requires `ENABLE_ANGLE` set to `ON` on macOS")
    endif ()
endif (NOT DISABLE_GRAPHICS)

##########################################################################################################
### END OF SCRIPT                                                                                      ###
##########################################################################################################