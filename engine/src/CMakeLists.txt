if(DEFINED ENV{VCPKG_ROOT})
    set(_vcpkg_root "$ENV{VCPKG_ROOT}")
else()
    # 回退到仓库内的 vcpkg 目录（按需修改）
    set(_vcpkg_root "${CMAKE_SOURCE_DIR}/vcpkg")
endif()

set(glfw3_DIR "${_vcpkg_root}/installed/x64-mingw-static/share/glfw3" CACHE PATH "Path to glfw3-config.cmake directory" FORCE)

# Print vcpkg and glfw paths for debugging
message(STATUS "_vcpkg_root = ${_vcpkg_root}")
message(STATUS "glfw3_DIR = ${glfw3_DIR}")

find_package(glfw3 CONFIG REQUIRED)
find_package (Vulkan REQUIRED)

# Find glslc shader compiler
add_library(mtengine SHARED 
    ./core/loggersystem.cpp
    ./core/memorysystem.cpp
    ./core/eventsystem.cpp
    ./core/jobsystem.cpp
    ./core/application.cpp
    ./render/vulkan/vulkan_backend.cpp
    ./render/vulkan/vulkan_context.cpp
    ./render/vulkan/vulkan_device.cpp
    ./render/vulkan/vulkan_swapchain.cpp
    ./render/vulkan/vulkan_command_buffer.cpp
    ./render/rendersystem.cpp
)

# When building the shared library we must export symbols (on Windows use dllexport).
# Define MT_EXPORTS for compilation of the mtengine target so MT_API expands to __declspec(dllexport).
target_compile_definitions(mtengine PRIVATE MT_EXPORTS)

target_include_directories(mtengine PRIVATE ./)

target_link_libraries(mtengine PRIVATE glfw)
target_link_libraries(mtengine PRIVATE Vulkan::Vulkan)

install(TARGETS mtengine DESTINATION bin)

