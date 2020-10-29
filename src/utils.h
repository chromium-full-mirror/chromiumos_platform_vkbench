// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <functional>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

extern int g_verbose;
void PrintDateTime();
std::vector<std::string> SplitString(const std::string& kInput, char delimiter);
std::string readFile(const std::string& filename);
void SavePPM(std::string, const char*, uint32_t, uint32_t, vk::DeviceSize);
vk::ShaderModule CreateShaderModule(const vk::Device& device, std::string code);

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

inline void VkCheck(const vk::Result& result, const char* func_str) {
  if (result != vk::Result::eSuccess) {
    char msg[1024];
    snprintf(msg, 1024, "%s failed: %s", func_str,
             vk::to_string(result).c_str());
    throw std::runtime_error(msg);
  }
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

class ScopeGuard {
 public:
  template <class Callable>
  ScopeGuard(Callable&& fn) : fn_(std::forward<Callable>(fn)) {}

  ScopeGuard(ScopeGuard&& other) : fn_(std::move(other.fn_)) {
    other.fn_ = nullptr;
  }

  ~ScopeGuard() {
    // must not throw
    if (fn_)
      fn_();
  }

  ScopeGuard(const ScopeGuard&) = delete;
  void operator=(const ScopeGuard&) = delete;

 private:
  std::function<void()> fn_;
};

// Used to defer a function call for cleanup
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define DEFER(fn) ScopeGuard CONCAT(__defer__, __LINE__) = [&]() { fn; }

#endif
