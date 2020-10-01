cmake_minimum_required(VERSION 3.8)

project(decaf)

# C++17 has a bunch of nice stuff, seems like a good level to target.
set(CXX_STANDARD_REQUIRED 17)

find_package(LLVM 10.0 REQUIRED)
find_package(GTest REQUIRED)
find_package(Z3 REQUIRED)

enable_testing()

file(
  sources GLOB_RECURSE
  src/*.cpp
  src/*.hpp
  src/*.h
  include/*.h
)

file(
  tests GLOB_RECURSE
  tests/*.cpp
  tests/*.h
  tests/*.hpp
)

add_library(decaf STATIC ${sources})
add_executable(decaf-tests ${tests})
add_executable(decaf-bin main.cpp)

target_include_directories(decaf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(decaf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(decaf PUBLIC LLVMCore z3::libz3)

target_include_directories(decaf-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_link_libraries(decaf-tests PRIVATE decaf)

target_link_libraries(decaf-bin PRIVATE decaf)
