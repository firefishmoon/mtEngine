# add_subdirectory(src)

if(DEFINED ENV{VCPKG_ROOT})
    set(_vcpkg_root "$ENV{VCPKG_ROOT}")
else()
    # fall back to the in-repo vcpkg directory (adjust as needed)
    set(_vcpkg_root "${CMAKE_SOURCE_DIR}/vcpkg")
endif()

# set(glfw3_DIR "${_vcpkg_root}/installed/x64-mingw-static/share/glfw3" CACHE PATH "Path to glfw3-config.cmake directory")
# message(STATUS "glfw3_DIR = ${glfw3_DIR}")

# find_package(glfw3 CONFIG REQUIRED)
set(glm_DIR "${_vcpkg_root}/installed/x64-mingw-static/share/glm" CACHE PATH "Path to glm-config.cmake directory" FORCE)

find_package(glm CONFIG REQUIRED)

add_executable(testbed
    main.cpp)

# Make engine headers available to testbed
target_include_directories(testbed PRIVATE ${CMAKE_SOURCE_DIR}/engine/src)

target_link_libraries(testbed PRIVATE mtengine)
target_link_libraries(testbed PRIVATE glm::glm)

set_target_properties(testbed PROPERTIES BUILD_RPATH "${CMAKE_BINARY_DIR}/engine/src" INSTALL_RPATH "@executable_path")


install(TARGETS testbed)
