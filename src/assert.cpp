
#include "macros.h"

#include <boost/core/demangle.hpp>
#include <boost/stacktrace.hpp>

#include <iostream>
#include <sstream>

namespace decaf::detail {
[[noreturn]] void assert_fail(const char *condition, const char *function, unsigned int line,
                              const char *file, message message) {
  auto stacktrace = boost::stacktrace::stacktrace();
  auto demangled = boost::core::demangle(function);

  std::cerr << "Assertion failed: " << condition << "\n"
            << "  location: " << file << ":" << line << "\n"
            << "  function: " << demangled << "\n";

  if (message.has_value) {
    std::cerr << "  message: " << message.msg << "\n";
  }

  std::cerr << "\n" << stacktrace << "\n";

  std::exit(255);
}

[[noreturn]] void abort(const char *function, unsigned int line, const char *file,
                        message message) {
  auto stacktrace = boost::stacktrace::stacktrace();
  auto demangled = boost::core::demangle(function);

  if (message.has_value) {
    std::cerr << "Aborted with message: " << message.msg << "\n"
              << "  location: " << file << ":" << line << "\n";
  } else {
    std::cerr << "Aborted at " << file << ":" << line << "\n";
  }

  std::cerr << "  function: " << demangled << "\n"
            << "  Stack Trace:\n"
            << stacktrace << "\n";

  std::exit(255);
}
} // namespace decaf::detail
