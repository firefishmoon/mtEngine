cmake_minimum_required(VERSION 3.15)
project(mtEngine)

if(DEFINED ENV{VCPKG_ROOT})
    set(_vcpkg_root "$ENV{VCPKG_ROOT}")
else()
    # fall back to the in-repo vcpkg directory (adjust as needed)
    set(_vcpkg_root "${CMAKE_SOURCE_DIR}/vcpkg")
endif()
# normalize path to use forward slashes, avoiding escape issues on Windows
file(TO_CMAKE_PATH "${_vcpkg_root}" _vcpkg_root)

list(APPEND CMAKE_MODULE_PATH
     "${_vcpkg_root}/installed/x64-mingw-static/share/stb")

# stb is header-only; we don't need a config file, only the module below
# the module searches for headers in the standard vcpkg include location

# Print vcpkg and glfw paths for debugging
message(STATUS "_vcpkg_root = ${_vcpkg_root}")

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
# stb is header-only; simply point to the vcpkg include directory
set(Stb_INCLUDE_DIR "${_vcpkg_root}/installed/x64-mingw-static/include")
message(STATUS "Stb include dir = ${Stb_INCLUDE_DIR}")
# optionally you could use the FindStb module above, but it only searches
# under Stb_DIR which in vcpkg is a share folder that doesn't contain headers.

file(GLOB_RECURSE MTEngine_SOURCES CONFIGURE_DEPENDS "*.cpp")
# Find glslc shader compiler
add_library(mtengine SHARED ${MTEngine_SOURCES})

# When building the shared library we must export symbols (on Windows use dllexport).
# Define MT_EXPORTS for compilation of the mtengine target so MT_API expands to __declspec(dllexport).
target_compile_definitions(mtengine PRIVATE MT_EXPORTS)

# engine sources include headers from the project root
target_include_directories(mtengine PRIVATE ./)
# add stb include path provided by the find module
target_include_directories(mtengine PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(mtengine PRIVATE glfw)
target_link_libraries(mtengine PRIVATE Vulkan::Vulkan)
target_link_libraries(mtengine PRIVATE glm::glm)
# no link target for stb (header-only)

install(TARGETS mtengine DESTINATION bin)

