cmake_minimum_required(VERSION 3.8)

project(decaf LANGUAGES C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(LLVM 10.0 REQUIRED)
find_package(GTest REQUIRED)
find_package(Z3 REQUIRED)
find_package(Boost REQUIRED)

enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# C++17 has a bunch of nice stuff, seems like a good level to target.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Compiler-specific warning options
# Add more here as we find ones we want enabled.
if (MSVC)
  # This disables all warnings in headers included with angle brackets
  #
  # Since these should always be external dependencies in which we don't
  # care about warnings anyway this is a good thing
  add_compile_options(
    /experimental:external
    /external:anglebrackets
    /external:W0
  )

  # Enable more detailed warnings (but not all of them).
  add_compile_options(/W4)

  # Some code within LLVM triggers this. It's not something we can fix so
  # better to silence the warning than have it drown everything out.
  add_compile_definitions(_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS=1)
endif()

file(
  GLOB_RECURSE sources
  src/*.cpp
  src/*.hpp
  src/*.h
  include/*.h
)

file(
  GLOB_RECURSE tests
  tests/*.cpp
  tests/*.h
  tests/*.hpp
)

set(fmt_sources "${sources}" "${tests}")

add_library(decaf STATIC ${sources})
add_executable(decaf-tests ${tests})
add_executable(decaf-bin main.cpp)

target_include_directories(decaf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(decaf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Apparently these aren't implicitly set up for all systems.
# Easier to just add them here.
target_include_directories(decaf PUBLIC "${LLVM_INCLUDE_DIRS}")
target_include_directories(decaf PUBLIC "${Z3_INCLUDE_DIRS}")
target_include_directories(decaf PUBLIC "${FMT_INCLUDE_DIRS}")
target_include_directories(decaf PUBLIC "${Boost_INCLUDE_DIRS}")
target_link_libraries(decaf PUBLIC LLVMCore "${Z3_LIBRARIES}")

target_include_directories(decaf-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_link_libraries(decaf-tests PRIVATE decaf)
target_link_libraries(decaf-tests PRIVATE GTest::GTest)
add_test(NAME decaf-tests COMMAND decaf-tests)

set_target_properties(decaf-bin PROPERTIES OUTPUT_NAME decaf)
target_link_libraries(decaf-bin PRIVATE decaf)
target_link_libraries(decaf-bin PRIVATE LLVMIRReader)

add_custom_target(format
  COMMAND clang-format -i ${fmt_sources}
  VERBATIM
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
