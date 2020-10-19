
file(
  GLOB_RECURSE tests
  *.cpp
  *.h
)

add_executable(decaf-tests ${tests})

target_include_directories(decaf-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(decaf-tests PRIVATE decaf)
target_link_libraries(decaf-tests PRIVATE GTest::GTest)

add_test(NAME decaf-tests COMMAND decaf-tests)
