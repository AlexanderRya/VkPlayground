cmake_minimum_required(VERSION 3.15)
project(VkPlayground)

find_package(Vulkan REQUIRED)

add_subdirectory(dependencies/glfw)
add_subdirectory(dependencies/fmt)

set(CMAKE_CXX_STANDARD 20)

add_executable(VkPlayground src/main.cpp
        include/application.hpp
        src/application.cpp
        include/callbacks.hpp
        include/util.hpp
        include/shader.hpp
        src/shader.cpp)

# target_compile_options(VkPlayground PUBLIC -Wall -Wextra -pedantic)

target_include_directories(VkPlayground PRIVATE
        "dependencies/glfw/include"
        "dependencies/fmt/include"
        "include"
        ${Vulkan_INCLUDE_DIRS})

if (WIN32)
    target_compile_definitions(VkPlayground PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if (UNIX)
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        target_compile_definitions(VkPlayground PRIVATE _GLIBCXX_DEBUG)
    endif()
endif()

if (UNIX)
    set_target_properties(glfw fmt
            PROPERTIES
            COMPILE_OPTIONS -Wno-everything)
endif()

if (WIN32)
    target_link_libraries (VkPlayground ${Vulkan_LIBRARIES} glfw fmt)
elseif (UNIX)
    target_link_libraries (VkPlayground ${Vulkan_LIBRARIES} glfw fmt pthread)
endif()