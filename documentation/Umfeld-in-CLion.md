# Umfeld in CLion

to run an *Umfeld* application as a *CMake* project in *CLion*[^1], follow these steps:

## macOS

everything should be in place if the *Quickstart* guide has been followed ( i.e applications can be compile in CLI ). just click `OK` when opening an new project.

## Windows

if MSYS2 has been installed correctly and the *Quickstart* guide has been followed ( i.e applications can be compile in CLI ) the only thing that NEEDS to be configured in *CLion* is the toolchain:

1. in `Settings… > Build, Execution, Deployment › Toolchains`
2. select `MinGW` toolchain
3. *CLion* comes with its own toolset but we need to change it to `UCRT64` installed with MSYS2 
4. find the `Toolset:` field
5. point the toolset to `UCRT64` installed with MSYS2 ( e.g default location might be `C:\msys64\ucrt64` )
6. a green check symbol and the version number should appear

## Linux

(TODO)

## Opening a Project

1. select `File > Open…` from the menu 
2. navigate to the folder ( or any parent folder ) that contains an *Umfeld* CMake script ( i.e `CMakeLists.txt` ) ( e.g `$PATH_TO_UMFELD$/umfeld-examples` or `$PATH_TO_UMFELD$/umfeld-examples/Basics/minimal` etcetera )
3. in the `Open Project Wizard` just press `OK`, optionally tick the `Reload CMake project on …`
4. *CLion* now opens all related resources ( if the CMake script is configured correctly )
5. find the CMake script `CMakeLists.txt` of the project in the `Project` view ( e.g `CMD+1` on macOS )
6. if necessary open `CMakeLists.txt` and adapt `set(UMFELD_PATH "[PATH_TO_UMFELD_FOLDER]")` to the folder containing *Umfeld* ( e.g the folder cloned from GitHub containing files like `install.sh`, `README.md` etcetera )
7. right-click on `CMakeLists.txt` and select `Load CMake Project` ( or `Reload CMake Project` if CLion already found the script automatically )
8. build and execute project by selecting `Run > Run 'my_project_name'` from the menu or by simply pressing the play button in the toolbar

[^1]: JetBrains is the company behind [CLion](https://www.jetbrains.com/de-de/clion/). this is what *they* say about the IDE: "**CLion** is a powerful and versatile IDE for C and C++ development on Linux, macOS and Windows. It supports modern C++ features, embedded systems, remote and collaborative work, and integrates with various tools and frameworks." we mention and promote this IDE, despite the fact that it is not an open source product. JetBrains offers a free license for non-commerical use as well as an academic license. 