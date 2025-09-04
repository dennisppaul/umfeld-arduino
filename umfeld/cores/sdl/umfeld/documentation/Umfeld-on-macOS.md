# Umfeld on macOS

macOS specific options

## Build Universal Binaries

To build applications that run on both Intel-based and ARM-based processors, include the following CMake option in your application's build script:"

```cmake
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for Mac OS X")
```

## Camera Access via `Info.plist`

macOS requires and requests permission to use a camera. in order to allow access add the following block to your CMake script:

`CMakeLists.txt`:

```cmake
if (APPLE)
    # Specify the path to your Info.plist file
    set(INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")

    # Ensure the Info.plist file exists
    if (EXISTS "${INFO_PLIST}")
        message(STATUS "adding Info.plist at ${INFO_PLIST}")
        target_link_options(${PROJECT_NAME} INTERFACE "SHELL:-sectcreate __TEXT __info_plist ${INFO_PLIST}")
    else ()
        message(FATAL_ERROR "Info.plist not found at ${INFO_PLIST}")
    endif ()
endif ()
```

and add an `Info.plist` file with the following lines:

```
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
 "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>NSCameraUsageDescription</key>
    <string>This app requires access to the camera.</string>
    <key>NSCameraUseContinuityCameraDeviceType</key>
    <true/>
</dict>
</plist>
```
