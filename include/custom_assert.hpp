#ifndef CUSTOM_ASSERT_HPP
#define CUSTOM_ASSERT_HPP

#include <cstdarg>
#include <cstdio>
#include <string>

#undef ASSERT
#ifdef NDEBUG
#define ASSERT(expr, fmt, ...) ((void)0)
#else
#undef STRINGIZE_HELPER
#define STRINGIZE_HELPER(arg) #arg
#undef STRINGIZE
#define STRINGIZE(arg) STRINGIZE_HELPER(arg)
#define ASSERT(expr, fmt, ...)                                                 \
  if (!(expr)) {                                                               \
    fputs("\033[91m" __FILE__ ":" STRINGIZE(__LINE__) "\033[0m Assertion of (" #expr ") failed: ", stderr);        \
    fprintf_s(stderr, fmt, __VA_ARGS__);                                       \
    std::terminate();                                                          \
  }                                                                            \
  (void)0
#endif

#endif // CUSTOM_ASSERT_HPP