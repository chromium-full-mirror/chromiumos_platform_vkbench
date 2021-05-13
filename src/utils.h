// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <fmt/format.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "./filepath.h"

namespace vkbench {
class Image {
 public:
  Image() : data_(nullptr) {}
  Image(const unsigned char* data,
        const vk::Extent2D size,
        vk::SubresourceLayout resource_layout)
      : size_(size), resource_layout_(resource_layout) {
    data_ = new unsigned char[resource_layout_.size];
    memcpy(data_, data, resource_layout_.size);
  }
  ~Image() { delete data_; }

  void Save(FilePath);

 private:
  void savePPM(FilePath);
  void savePNG(FilePath);

  unsigned char* data_;
  vk::Extent2D size_;
  vk::SubresourceLayout resource_layout_;
};
}  // namespace vkbench

extern int g_verbose;
void PrintDateTime();
std::vector<std::string> SplitString(const std::string& kInput, char delimiter);
std::string readShaderFile(const std::string& filename);
vk::ShaderModule CreateShaderModule(const vk::Device& device, std::string code);

inline uint64_t GetUTime() {
  struct timeval tv = {};
  gettimeofday(&tv, nullptr);
  return tv.tv_usec + 1000000ULL * static_cast<uint64_t>(tv.tv_sec);
}

bool IsItemInVector(const std::vector<std::string>& list,
                    const char* value,
                    bool empty_value);
bool check_file_existence(const char* file_path, struct stat* buffer = NULL);
bool check_dir_existence(const char* file_path);

inline void DbgPrintf(bool should_abort,
                      const char* filename,
                      int line,
                      FILE* fileid,
                      const char* format,
                      ...) {
  va_list args;
  va_start(args, format);
  std::string debug_header, string_format;
  if (g_verbose) {
    debug_header = fmt::format("[{}:{}]", filename, line);
    string_format = fmt::format("{:>30} {}\n", debug_header, format);
  } else {
    string_format = fmt::format("{}\n", format);
  }
  vfprintf(fileid, string_format.c_str(), args);
  va_end(args);
  if (should_abort)
    throw std::runtime_error("");
}

#define DEBUG(fmt, ...)        \
  {                            \
    if (g_verbose) {           \
      LOG(fmt, ##__VA_ARGS__); \
    }                          \
  }
#define LOG(fmt, ...) \
  { DbgPrintf(false, __FILE__, __LINE__, stdout, fmt, ##__VA_ARGS__); }
#define ERROR(fmt, ...) \
  { DbgPrintf(false, __FILE__, __LINE__, stderr, fmt, ##__VA_ARGS__); }
#define RUNTIME_ERROR(fmt, ...) \
  { DbgPrintf(true, __FILE__, __LINE__, stderr, fmt, ##__VA_ARGS__); }

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

#endif  // SRC_UTILS_H_
