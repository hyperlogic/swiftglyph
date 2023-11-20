# mkdir build
# cd build
# cmake -DCMAKE_TOOLCHAIN_FILE="c:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake" ..
# cmake --build .

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
set(PROJECT_NAME swiftglyph)

project(${PROJECT_NAME} LANGUAGES CXX)

find_package(Freetype REQUIRED)

add_executable(${PROJECT_NAME} swiftglyph.cpp tga.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE Freetype::Freetype)
