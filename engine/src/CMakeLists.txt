set(glfw3_DIR "${_vcpkg_root}/installed/x64-mingw-static/share/glfw3" CACHE PATH "Path to glfw3-config.cmake directory")

find_package(glfw3 CONFIG REQUIRED)
find_package (Vulkan REQUIRED)

add_library(mtengine SHARED 
./core/loggersystem.cpp
./core/memorysystem.cpp
./core/eventsystem.cpp
./core/jobsystem.cpp
./core/application.cpp
./render/vulkan/vulkan_backend.cpp
./render/rendersystem.cpp
)

# When building the shared library we must export symbols (on Windows use dllexport).
# Define MT_EXPORTS for compilation of the mtengine target so MT_API expands to __declspec(dllexport).
target_compile_definitions(mtengine PRIVATE MT_EXPORTS)

target_include_directories(mtengine PRIVATE ./)

target_link_libraries(mtengine PRIVATE glfw)
target_link_libraries(mtengine PRIVATE Vulkan::Vulkan)

install(TARGETS mtengine DESTINATION bin)

