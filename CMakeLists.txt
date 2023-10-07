cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# 开启控制台输出
set(CMAKE_EXE_LINKER_FLAGS "-Wl,-subsystem,console")

project(skToyRenderer
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "a toy 2d renderer made in vulkan")

# fetch SDL under Windows
# for Appveyor CI/CD
if(WIN32)
    include(cmake/fetch_sdl.cmake)
    option(FETCH_SDL "fetch SDL2 SDL2_image SDL2_ttf from web" FALSE)

    if(FETCH_SDL)
        FetchSDL()
    endif()
endif()

include(cmake/utility.cmake)
include(cmake/copyFiles.cmake)
include(cmake/FindSDL2.cmake)
include(cmake/FindSDL2_image.cmake)
include(cmake/findVulkan.cmake)

set(ProjectName "skToyRenderer")

find_program(GLSLC_PROGRAM glslc REQUIRED)
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_SOURCE_DIR}/shaders/shader.vert -o ${CMAKE_SOURCE_DIR}/shaders/vert.spv)
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_SOURCE_DIR}/shaders/shader.frag -o ${CMAKE_SOURCE_DIR}/shaders/frag.spv)

file(GLOB_RECURSE HEADER "src/*.hpp")
file(GLOB_RECURSE SRC "src/*.cpp")

add_library(${ProjectName} STATIC ${SRC} ${HEADER})
target_precompile_headers(${ProjectName} PUBLIC "src/pch.hpp")
target_include_directories(${ProjectName} PUBLIC ${HEADER})
target_link_libraries(${ProjectName} PUBLIC Vulkan::Vulkan)
target_link_libraries(${ProjectName} PUBLIC SDL2 SDL2_image)
target_compile_features(${ProjectName} PUBLIC cxx_std_17)

# add_subdirectory(sandbox)