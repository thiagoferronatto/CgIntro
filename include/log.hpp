#ifndef LOG_HPP
#define LOG_HPP

#include <cstdarg>
#include <string>

inline std::string globalLog;

inline void logMsg(const char *fmt, ...) {
  char str[1024];
  va_list argList;
  va_start(argList, fmt);
  vsprintf_s(str, fmt, argList);
  fputs(str, stdout);
  globalLog += str;
}

#endif // LOG_HPP