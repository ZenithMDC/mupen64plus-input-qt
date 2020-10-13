#
# mupen64plus-input-qt
#
cmake_minimum_required(VERSION 3.18)

project(mupen64plus-input-qt)

set(CMAKE_AUTOMOC ON)
find_package(Qt5 COMPONENTS Widgets Core REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

set(SOURCES
    configdialog.cpp
    qt2sdl2.cpp
    main.cpp
)

if (WIN32 OR MSYS OR MINGW)
    list(APPEND SOURCES osal/osal_dynamiclib_win32.c)
elseif(UNIX)
    list(APPEND SOURCES osal/osal_dynamiclib_unix.c)   
endif()

add_library(mupen64plus-input-qt SHARED ${SOURCES})

target_link_libraries(mupen64plus-input-qt ${SDL2_LIBRARIES} Qt5::Widgets)

target_include_directories(mupen64plus-input-qt PRIVATE ${APIDIR})
