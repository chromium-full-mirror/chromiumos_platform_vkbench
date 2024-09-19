// Copyright 2020 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_FILEPATH_H_
#define SRC_FILEPATH_H_

#include <fmt/format.h>
#include <iostream>
#include <string>

class FilePath {
 public:
  FilePath() { this->path_ = std::string(""); }
  FilePath(const FilePath& that) { this->path_ = that.path_; }
  explicit FilePath(std::string path) { this->path_ = path; }
  explicit FilePath(const char* path) { this->path_ = path; }
  ~FilePath() = default;

  FilePath DirName() const;
  const std::string& value() const;
  const char* c_str() const;
  bool IsSeparator(char character) const;
  FilePath Append(const FilePath& path);
  void StripTrailingSeparatorsInternal();

 private:
  std::string path_;
  char kStringTerminator = '\0';
  char kSeparators[2] = "/";
  char kCurrentDirectory[2] = ".";
};

template <>
struct fmt::formatter<FilePath> : formatter<string_view> {
  template <typename FormatContext>
  auto format(FilePath c, FormatContext& ctx) const {
    return formatter<string_view>::format(std::string(c.c_str()), ctx);
  }
};

bool CreateDirectory(const FilePath&);
std::string::size_type FindDriveLetter(std::string path);

#endif  // SRC_FILEPATH_H_
