cmake_minimum_required(VERSION 3.21)

project(umfeld-SDK)
set(UMFELD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")

# --------- no need to change anything below this line ---------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#include_directories(".")
file(GLOB SOURCE_FILES "*.cpp")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

add_subdirectory(${UMFELD_PATH} ${CMAKE_BINARY_DIR}/umfeld-lib-${PROJECT_NAME})
add_umfeld_libs()

# --------- application bundle ---------------------------------

find_program(DYLIBBUNDLER_EXECUTABLE
        NAMES dylibbundler
        DOC "Path to 'dylibbundler' (used to bundle macOS dylibs)"
)

if(NOT DYLIBBUNDLER_EXECUTABLE)
    message(WARNING "'dylibbundler' not found. The app will build, but dependent dylibs will NOT be bundled. "
            "Install it (e.g 'brew install dylibbundler') or set DYLIBBUNDLER_EXECUTABLE to its path.")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
        BUILD_WITH_INSTALL_RPATH ON
        INSTALL_RPATH "@executable_path/../libs/"
)

# bundle dynamic libraries
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Bundling dynamic libraries using dylibbundler..."
        COMMAND dylibbundler -od -b -x $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME} -d $<TARGET_FILE_DIR:${PROJECT_NAME}>/../libs/
        COMMAND ${CMAKE_COMMAND} -E echo "Dynamic libraries are bundled."
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/libs/ DESTINATION "../libs")

# ------------------- SDK packaging (Umfeld library + headers) -------------------

# 1) Create a consolidated Umfeld library that links all collected third-party libs
#    Adjust sources if umfeld’s core library is already defined elsewhere.
#    If umfeld already defines a target (e.g. umfeld::core), you can instead add an
#    INTERFACE target that links to it and third-party libs.
set(UMFELD_SRC_DIR "${UMFELD_PATH}/src")
set(UMFELD_INCLUDE_DIR "${UMFELD_PATH}/include")

#file(GLOB_RECURSE UMFELD_SRC_FILES
#        "${UMFELD_SRC_DIR}/*.cpp"
#        "${UMFELD_SRC_DIR}/*.c"
#)
#
#add_library(Umfeld STATIC ${UMFELD_SRC_FILES})
#target_include_directories(Umfeld
#        PUBLIC
#        "$<BUILD_INTERFACE:${UMFELD_INCLUDE_DIR}>"
#        "$<INSTALL_INTERFACE:include>"
#)

# Link the third-party libs pulled in by add_umfeld_libs()
# If add_umfeld_libs() registers variables like UMFELD_THIRDPARTY_LIBS, use them.
# Otherwise, replace 'umfeld_thirdparty' with the appropriate libs/targets.
set(UMFELD_THIRDPARTY_TARGETS "umfeld-lib;umfeld-lib-interface")
target_link_libraries(${PROJECT_NAME} PUBLIC ${UMFELD_THIRDPARTY_TARGETS})

# 2) Version and platform tagging
set(UMFELD_VERSION "1.0.0" CACHE STRING "Umfeld SDK version")
string(TOLOWER "${CMAKE_SYSTEM_NAME}" UMFELD_PLATFORM_LOWER)
set(UMFELD_SDK_DIR "Umfeld-SDK-${UMFELD_VERSION}-${UMFELD_PLATFORM_LOWER}")

# 3) Install layout
include(GNUInstallDirs)

# Headers (public API)
install(DIRECTORY "${UMFELD_INCLUDE_DIR}/"
        DESTINATION "${UMFELD_SDK_DIR}/include"
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
)

# Library
install(TARGETS umfeld-lib glad
        EXPORT UmfeldTargets
        ARCHIVE DESTINATION "${UMFELD_SDK_DIR}/${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${UMFELD_SDK_DIR}/${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${UMFELD_SDK_DIR}/${CMAKE_INSTALL_BINDIR}"
)
set_target_properties(umfeld-lib PROPERTIES OUTPUT_NAME "umfeld")

# Export targets for consumers
install(EXPORT UmfeldTargets
        NAMESPACE Umfeld::
        DESTINATION "${UMFELD_SDK_DIR}/cmake/Umfeld"
)

# Optional tools or DLLs (Windows)
# Add additional runtime tools here as needed
# Example: install(FILES "$<TARGET_FILE:SomeTool>" DESTINATION "${UMFELD_SDK_DIR}/bin")

# Thirdparty licenses/notices
if(EXISTS "${UMFELD_PATH}/LICENSES-libraries")
    install(DIRECTORY "${UMFELD_PATH}/LICENSES-libraries/"
            DESTINATION "${UMFELD_SDK_DIR}/thirdparty")
endif()

# 4) CMake package config files
include(CMakePackageConfigHelpers)

# Generate a ConfigVersion file
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/UmfeldConfigVersion.cmake"
        VERSION "${UMFELD_VERSION}"
        COMPATIBILITY SameMajorVersion
)

# Generate a simple Config file that imports the exported targets
configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/UmfeldConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/UmfeldConfig.cmake"
        INSTALL_DESTINATION "${UMFELD_SDK_DIR}/cmake/Umfeld"
)

# Export targets for consumers
install(EXPORT UmfeldTargets
        NAMESPACE Umfeld::
        DESTINATION "${UMFELD_SDK_DIR}/cmake/Umfeld"
)

# Install the generated Config files
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/UmfeldConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/UmfeldConfigVersion.cmake"
        DESTINATION "${UMFELD_SDK_DIR}/cmake/Umfeld"
)

# 5) Optional pkg-config file
# Create a basic pkg-config template at ${UMFELD_PATH}/cmake/umfeld.pc.in with variables:
#   prefix=@CMAKE_INSTALL_PREFIX@
#   libdir=${prefix}/${CMAKE_INSTALL_LIBDIR}
#   includedir=${prefix}/include
#   Name: Umfeld
#   Description: Umfeld SDK
#   Version: @UMFELD_VERSION@
#   Libs: -L${libdir} -lUmfeld
#   Cflags: -I${includedir}
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/umfeld.pc.in")
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/umfeld.pc.in"
            "${CMAKE_CURRENT_BINARY_DIR}/umfeld.pc"
            @ONLY
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/umfeld.pc"
            DESTINATION "${UMFELD_SDK_DIR}/pkgconfig")
endif()

# from within `deployment` folder:
#     cmake -B cmake-build-debug -DCMAKE_INSTALL_PREFIX="${PWD}/sdk" ; cmake --build cmake-build-debug ; cmake --install cmake-build-debug
