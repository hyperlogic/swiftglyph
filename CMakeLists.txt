cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
set(PROJECT_NAME swiftglyph)

project(${PROJECT_NAME} LANGUAGES CXX)

if(WIN32)
    set(VCPKG_INCLUDE_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows/include")
    set(VCPKG_LIB_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows/lib")
    #set(SDL2_LIBRARIES ${VCPKG_LIB_DIR}/SDL2.lib ${VCPKG_LIB_DIR}/manual-link/SDL2main.lib)
    set(FREETYPE_LIBRARIES ${VCPKG_LIB_DIR}/freetype.lib ${VCPKG_LIB_DIR}/bz2.lib)
    include_directories($VCPKG_INCLUDE_DIR)
else()
    find_package(Freetype REQUIRED)
endif()

add_executable(${PROJECT_NAME} swiftglyph.cpp tga.cpp)

target_link_libraries(${PROJECT_NAME} ${FREETYPE_LIBRARIES})
