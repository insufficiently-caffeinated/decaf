
file(GLOB c_tests   CONFIGURE_DEPENDS *.c)
file(GLOB cpp_tests CONFIGURE_DEPENDS *.cpp)
file(GLOB ir_tests  CONFIGURE_DEPENDS *.ll)

foreach(test ${c_tests})
  file(RELATIVE_PATH test_file "${CMAKE_CURRENT_SOURCE_DIR}" "${test}")
  set(TEST_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${test_file}.ll")

  add_custom_command(
    OUTPUT "${TEST_OUTPUT}"
    COMMAND "clang-10" "${test}" -S -emit-llvm -o "${TEST_OUTPUT}" -O3 -fcolor-diagnostics
    MAIN_DEPENDENCY "${test}"
  )

  add_test(
    NAME "run-fail/${test_file}"
    COMMAND test-fail "${TEST_OUTPUT}" test
  )

  list(APPEND test_files "${TEST_OUTPUT}")
endforeach()

# TODO: Add support for LLVM IR (.ll) test case files. They'll likely be
#       needed for more tests later on. While were at it, we should
#       probably add support for C++-based test cases.

# Custom commands don't run by default. We need a target to force them
# to run.
add_custom_target(
  run-fail-tests ALL
  DEPENDS "${test_files}"
)

