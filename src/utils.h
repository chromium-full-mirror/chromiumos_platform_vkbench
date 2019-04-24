// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdarg.h>
#include <sys/time.h>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

extern int g_verbose;
void PrintDateTime();
std::vector<std::string> SplitString(const std::string& kInput, char delimiter);
// For thermal monitoring of system.
double GetMachineTemperature();
// Wait for machine to cool with temperature in Celsius and timeout in seconds.
// Returns the time spent waiting and sets the last observed temperature.
double WaitForCoolMachine(double cold_temperature,
                          double timeout,
                          double* temperature);

inline uint64_t GetUTime() {
  struct timeval tv = {};
  gettimeofday(&tv, nullptr);
  return tv.tv_usec + 1000000ULL * static_cast<uint64_t>(tv.tv_sec);
}

bool IsItemInVector(const std::vector<std::string>& list,
                    const char* value,
                    bool empty_value);

inline void DbgPrintf(const char* filename,
                      int line,
                      FILE* fileid,
                      const char* format,
                      ...) {
  va_list args;
  va_start(args, format);
  char string_format[strlen(format) + 1024];
  char debug_header[1024] = "\0";
  if (g_verbose) {
    sprintf(debug_header, "[%s:%d]", filename, line);
    sprintf(string_format, "%30s %s\n", debug_header, format);
  } else {
    sprintf(string_format, "%s\n", format);
  }
  vfprintf(fileid, string_format, args);
  va_end(args);
}

#define DEBUG(fmt, ...)        \
  {                            \
    if (g_verbose) {           \
      LOG(fmt, ##__VA_ARGS__); \
    }                          \
  }
#define LOG(fmt, ...) \
  { DbgPrintf(__FILE__, __LINE__, stdout, fmt, ##__VA_ARGS__); }
#define ERROR(fmt, ...) \
  { DbgPrintf(__FILE__, __LINE__, stderr, fmt, ##__VA_ARGS__); }
#define ABORT(code, fmt, ...)  \
  {                            \
    ERROR(fmt, ##__VA_ARGS__); \
    exit(code);                \
  }

// Put this in the declarations for a class to be uncopyable.
#define DISALLOW_COPY(TypeName) TypeName(const TypeName&) = delete

// Put this in the declarations for a class to be unassignable.
#define DISALLOW_ASSIGN(TypeName) TypeName& operator=(const TypeName&) = delete

// Put this in the declarations for a class to be uncopyable and unassignable.
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  DISALLOW_COPY(TypeName);                 \
  DISALLOW_ASSIGN(TypeName)

// Ignore warning for linter for unused variable.
#define UNUSED(x) (void)(x)

#endif
