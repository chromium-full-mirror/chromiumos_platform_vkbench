// Copyright 2021 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_TESTS_COPY_H_
#define SRC_TESTS_COPY_H_

#include <string>
#include <vector>

#include "src/testBase.h"
#include "src/vkBuffer.h"
#include "src/vkImage.h"

namespace vkbench {
namespace copy {

// Copy image copies the sampled image
class ImageToImage : public commandTestBase {
 public:
  ImageToImage(uint64_t size, std::string color, vkBase* base);
  ~ImageToImage() override = default;

  const char* Unit() const override { return "us"; }
  Image GetImage() const override;

 protected:
  void Initialize() override;
  void Destroy() override;

 private:
  std::string color_;
  vk::Extent2D img_size_;
  vk::Format img_format_ = vk::Format::eR8G8B8A8Unorm;

  vkbench::vkImage *src_img_, *dest_img_;

  DISALLOW_COPY_AND_ASSIGN(ImageToImage);
};

class BufferToImage : public commandTestBase {
 public:
  BufferToImage(uint64_t img_size,
                std::string color,
                std::string buffer_memory_property,
                vkBase* base);
  ~BufferToImage() override = default;

  const char* Unit() const override { return "us"; }
  Image GetImage() const override;

 protected:
  void Initialize() override;
  void Destroy() override;

 private:
  std::string color_;
  vk::Extent2D img_size_;
  vk::Format img_format_ = vk::Format::eR8G8B8A8Unorm;

  vkbench::vkBuffer* buffer_;
  vkbench::vkImage* img_;
  vkbench::vkMemoryRequirement buffer_memory_requirement_;

  DISALLOW_COPY_AND_ASSIGN(BufferToImage);
};

// GenTests generate all the submit tests.
std::vector<vkbench::testBase*> GenTests();
}  // namespace copy
}  // namespace vkbench
#endif  // SRC_TESTS_COPY_H_
