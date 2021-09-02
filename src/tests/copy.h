// Copyright 2021 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_TESTS_COPY_H_
#define SRC_TESTS_COPY_H_

#include <string>
#include <vector>

#include "../testBase.h"

namespace vkbench {
namespace copy {

// Copy image copies the sampled image
class CopyImage : public testBase {
 public:
  CopyImage(uint64_t size, std::string color, vkBase* base);
  ~CopyImage() override = default;

  const char* Name() const override { return name_.c_str(); }
  const char* Desp() const override { return desp_.c_str(); }
  const char* Unit() const override { return "us"; }
  Image GetImage() const override;

 protected:
  void Initialize() override;
  void Run() override;
  void Destroy() override;

 private:
  std::string name_;
  std::string desp_;
  std::string color_;
  std::vector<vk::CommandBuffer> cmds_;
  vk::Extent2D img_size_;
  vk::Format img_format_ = vk::Format::eR8G8B8A8Unorm;

  vkbench::vkImage *src_img_, *dest_img_;

  DISALLOW_COPY_AND_ASSIGN(CopyImage);
};

// GenTests generate all the submit tests.
std::vector<vkbench::testBase*> GenTests();
}  // namespace copy
}  // namespace vkbench
#endif  // SRC_TESTS_COPY_H_
