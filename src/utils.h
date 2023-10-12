// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <fmt/format.h>
#include <fmt/printf.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <vulkan/vulkan.hpp>

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "src/filepath.h"

namespace vkbench {
class Image {
 public:
  Image() = delete;
  Image(const std::string& file_name);
  Image(const unsigned char* data,
        const vk::Extent2D extent,
        vk::SubresourceLayout resource_layout)
      : extent_(extent), resource_layout_(resource_layout) {
    data_ = new unsigned char[resource_layout_.size];
    memcpy(data_, data, resource_layout_.size);
  }
  ~Image() { delete data_; }

  void Save(FilePath);

  const vk::Extent2D Extent2D() const { return extent_; }
  const unsigned char* Data() const { return data_; }
  const vk::SubresourceLayout Layout() const { return resource_layout_; }

 private:
  // load loads image from file_path. It assumes data is in R8G8B8A8 format.
  void load(FilePath file_path);
  // savePPM saves data into file_path in PPM format. It assumes data is in
  // R8G8B8A8 format.
  void savePPM(FilePath file_path);
  // savePNG saves data into file_path in PNG format. It assumes data is in
  // R8G8B8A8 format.
  void savePNG(FilePath file_path);
  // loadPNG loads data from file_path with PNG format. It assumes data is in
  // R8G8B8A8 format.
  void loadPNG(FilePath file_path);

  unsigned char* data_ = nullptr;
  vk::Extent2D extent_;
  vk::SubresourceLayout resource_layout_;
};

class not_supported_exception : public std::runtime_error {
 public:
  not_supported_exception(std::string msg) : std::runtime_error(msg) {}
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

inline uint64_t GetNTime() {
  struct timespec tv = {};
  clock_gettime(CLOCK_MONOTONIC, &tv);
  return tv.tv_nsec + 1000000000ULL * static_cast<uint64_t>(tv.tv_sec);
}

bool IsItemInVector(const std::vector<std::string>& list,
                    const char* value,
                    bool empty_value);
bool check_file_existence(const char* file_path, struct stat* buffer = NULL);
bool check_dir_existence(const char* file_path);
// randi returns a random number between 0 and max.
uint32_t randi(uint32_t max);

enum DbgThrowType { THROW_NONE = 0, THROW_NOT_SUPPORT = 1, THROW_RUNTIME = 2 };

inline void DbgPrintf(DbgThrowType type,
                      const char* filename,
                      int line,
                      FILE* fileid,
                      const fmt::string_view format,
                      fmt::format_args args) {
  std::string content = fmt::vformat(format, args);
  if (g_verbose) {
    // Append debugging info. Only shows the basename of the file.
    FilePath f = FilePath(filename);
    int dir_length = strlen(f.DirName().c_str());
    std::string debug_header =
        fmt::format("[{}:{}]", filename + dir_length + 1, line);
    content = fmt::format("{:<15} {}", debug_header, content);
  }
  switch (type) {
    case THROW_NOT_SUPPORT:
      throw vkbench::not_supported_exception(content);
    case THROW_RUNTIME:
      throw std::runtime_error(content);
    case THROW_NONE:
      // Simply print the message.
      fmt::print(fileid, "{}\n", content);
  }
}

template <typename S, typename... Args>
void vlog(DbgThrowType type,
          const char* file,
          int line,
          FILE* fileid,
          const S& format,
          Args&&... args) {
  DbgPrintf(type, file, line, fileid, format,
            fmt::make_format_args(args...));
}

#define DEBUG(fmt, ...)        \
  do {                         \
    if (g_verbose) {           \
      LOG(fmt, ##__VA_ARGS__); \
    }                          \
  } while (0)
#define LOG(fmt, ...) \
  vlog(THROW_NONE, __FILE__, __LINE__, stdout, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) \
  vlog(THROW_NONE, __FILE__, __LINE__, stderr, fmt, ##__VA_ARGS__)
#define RUNTIME_ERROR(fmt, ...) \
  vlog(THROW_RUNTIME, __FILE__, __LINE__, stderr, fmt, ##__VA_ARGS__)
#define NOT_SUPPORT(fmt, ...) \
  vlog(THROW_NOT_SUPPORT, __FILE__, __LINE__, stderr, fmt, ##__VA_ARGS__)

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
