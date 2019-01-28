### CMakeLists.txt for CUDA

cmake_minimum_required(VERSION 2.8)
find_package(PkgConfig REQUIRED)
find_package(Vulkan REQUIRED)

pkg_search_module(GLFW REQUIRED glfw3)

# Pass options to NVCC
set(
    CUDA_NVCC_FLAGS
    ${CUDA_NVCC_FLAGS};
    -O3 -gencode arch=compute_52,code=sm_52
    )

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX17)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
elseif(COMPILER_SUPPORTS_CXX14)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
elseif(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
endif()

# Specify include directories
include_directories(
			include
            ~/glm/glm-0.9.9.3/glm
            ${GLFW_INCLUDE_DIRS}
            ${Vulkan_INCLUDE_DIR} 
    )

# For compilation ...
# Specify target & source files to compile it from
add_executable(
  doomfs
  src/main.cpp
)

target_link_libraries(doomfs
    ${GLFW_STATIC_LIBRARIES}
    ${Vulkan_LIBRARIES}
)

add_custom_command(
        TARGET doomfs POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/data/doom1.wad
                ${CMAKE_CURRENT_BINARY_DIR}/doom1.wad)
add_custom_command(
        TARGET doomfs PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_BINARY_DIR}/shaders)
add_custom_command(
        TARGET doomfs POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/shaders/*
                ${CMAKE_CURRENT_BINARY_DIR}/shaders)