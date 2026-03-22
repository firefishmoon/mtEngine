# add_subdirectory(src)

find_package(glm CONFIG REQUIRED)

add_executable(testbed
    main.cpp)

# Make engine headers available to testbed
target_include_directories(testbed PRIVATE ${CMAKE_SOURCE_DIR}/engine/src)

target_link_libraries(testbed PRIVATE mtengine)
target_link_libraries(testbed PRIVATE glm::glm)

set_target_properties(testbed PROPERTIES BUILD_RPATH "${CMAKE_BINARY_DIR}/engine/src" INSTALL_RPATH "@executable_path")


install(TARGETS testbed)
