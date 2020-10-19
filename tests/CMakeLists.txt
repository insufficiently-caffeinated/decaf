
find_program(CLANG   NAME clang-10)
find_program(CLANGXX NAME clang++-10)  

add_subdirectory(unit)
add_subdirectory(run-pass)
add_subdirectory(run-fail)

# This extra target makes working with the sources easier
# since there isn't multiple targets owning the same file.
add_library(test-common INTERFACE)
target_sources(test-common INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/test-common.hpp")
target_link_libraries(test-common INTERFACE decaf LLVMIRReader)
target_include_directories(test-common INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(test-fail test-fail.cpp)
add_executable(test-pass test-pass.cpp)

target_link_libraries(test-fail PRIVATE test-common)
target_link_libraries(test-pass PRIVATE test-common)
