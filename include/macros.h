
#ifndef DECAF_MACROS_H
#define DECAF_MACROS_H

#include <string_view>

namespace decaf {
namespace detail {
  /** Optional-like type for use in message creation.
   *
   * I'm trying to keep this header lightweight so that including it
   * everywhere is less of an issue.
   *
   * This struct should not usually be used directly. Use one of the
   * assertion or abortion macros instead.
   */
  struct message {
    bool has_value;
    std::string_view msg;

    message() : has_value(false) {}
    message(std::string_view msg) : has_value(true), msg(msg) {}
  };

  /**
   * Exit the process with an "assertion failed" message and print a
   * backtrace of where the assertion failed.
   *
   * Usually, this function should not be called directly. Use
   * DECAF_ASSERT instead.
   */
  [[noreturn]] void assert_fail(const char *condition, const char *function, unsigned int line,
                                const char *file, message message);

  /**
   * Exit the process with an abort message and print a backtrace of
   * where the process aborted.
   *
   * Usually, this function should not be called directly. Use DECAF_ABORT
   * or one of the other abortion macros such as DECAF_UNIMPLEMENTED or
   * DECAF_UNREACHABLE instead.
   */
  [[noreturn]] void abort(const char *function, unsigned int line, const char *file,
                          message message);
} // namespace detail
} // namespace decaf

#ifdef __PRETTY_FUNCTION__
#define DECAF_FUNCTION __PRETTY_FUNCTION__
#else
#define DECAF_FUNCTION __FUNCTION__
#endif

/**
 * Abort the process if the condition is not true.
 *
 * There are two valid forms for this assert macro
 * ```
 * DECAF_ASSERT(cond);
 * DECAF_ASSERT(cond, "some message");
 * ```
 *
 * The only difference is that the first one uses a default message and the
 * second one uses the provided message when it fails.
 *
 * Note that the message is only evaluated if the assertion fails.
 */
#define DECAF_ASSERT(cond, ...)                                                                    \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      ::decaf::detail::assert_fail(#cond, DECAF_FUNCTION, __LINE__, __FILE__,                      \
                                   ::decaf::detail::message(__VA_ARGS__));                         \
    }                                                                                              \
  } while (false)

/**
 * Abort the process with an optional message.
 */
#define DECAF_ABORT(...)                                                                           \
  ::decaf::detail::abort(DECAF_FUNCTION, __LINE__, __FILE__, ::decaf::detail::message(__VA_ARGS__))

// Abort the process with a message about unreachable code
#define DECAF_UNREACHABLE() DECAF_ABORT("entered unreachable code")
// Abort the process with a "not implemented" message.
#define DECAF_UNIMPLEMENTED() DECAF_ABORT("not implemented")

#endif
