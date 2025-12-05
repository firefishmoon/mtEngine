add_subdirectory(core)

if(DEFINED ENV{VCPKG_ROOT})
    set(_vcpkg_root "$ENV{VCPKG_ROOT}")
else()
    # 回退到仓库内的 vcpkg 目录（按需修改）
    set(_vcpkg_root "${CMAKE_SOURCE_DIR}/vcpkg")
endif()

set(glfw3_DIR "${_vcpkg_root}/installed/x64-mingw-static/share/glfw3" CACHE PATH "Path to glfw3-config.cmake directory")
message(STATUS "glfw3_DIR = ${glfw3_DIR}")

find_package(glfw3 CONFIG REQUIRED)

add_executable(mtEngine main.cpp)
target_link_libraries(mtEngine PRIVATE core)

target_link_libraries(mtEngine PRIVATE glfw)

install(TARGETS mtEngine)
