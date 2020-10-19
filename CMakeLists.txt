cmake_minimum_required(VERSION 3.8)

project(decaf LANGUAGES C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(LLVM 10.0 REQUIRED)
find_package(GTest REQUIRED)
find_package(Z3 4.8.7 REQUIRED)
find_package(Boost REQUIRED)
find_package(fmt REQUIRED)

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
elseif(UNIX)
  if (CMAKE_GENERATOR STREQUAL "Ninja")
    # By default error messages are emitted without colour when compiling
    # with Ninja. This turns on coloured diagnostics by default.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      add_compile_options(-fdiagnostics-color=always)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      add_compile_options(fcolor-diagnostics)
    endif()
  endif()
endif()

add_subdirectory(tests)

file(
  GLOB_RECURSE sources
  src/*.cpp
  src/*.hpp
  src/*.h
  include/*.h
)

add_library(decaf STATIC ${sources})
add_executable(decaf-bin main.cpp)

target_include_directories(decaf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(decaf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Apparently these aren't implicitly set up for all systems.
# Easier to just add them here.
target_include_directories(decaf PUBLIC "${LLVM_INCLUDE_DIRS}")
target_include_directories(decaf PUBLIC "${Z3_INCLUDE_DIRS}")
target_include_directories(decaf PUBLIC "${FMT_INCLUDE_DIRS}")
target_include_directories(decaf PUBLIC "${Boost_INCLUDE_DIRS}")
target_link_libraries(decaf PUBLIC LLVMCore ${Z3_LIBRARIES} fmt::fmt)

if (NOT WIN32)
  # Having the executable have the same name as the library results in
  # the output pdb files being overwritten.
  set_target_properties(decaf-bin PROPERTIES OUTPUT_NAME decaf)
endif ()
target_link_libraries(decaf-bin PRIVATE decaf)
target_link_libraries(decaf-bin PRIVATE LLVMIRReader)

file(
  GLOB_RECURSE fmt_sources
  *.c
  *.cpp
  *.h
  *.hpp
)

set(fmt_sources "${fmt_sources}")

add_custom_target(format
  COMMAND clang-format -i ${fmt_sources}
  VERBATIM
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
