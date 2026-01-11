
add_library(mtengine SHARED 
./core/loggersystem.cpp
./core/memorysystem.cpp
./core/eventsystem.cpp
./core/jobsystem.cpp)

# When building the shared library we must export symbols (on Windows use dllexport).
# Define MT_EXPORTS for compilation of the mtengine target so MT_API expands to __declspec(dllexport).
target_compile_definitions(mtengine PRIVATE MT_EXPORTS)


install(TARGETS mtengine DESTINATION bin)

