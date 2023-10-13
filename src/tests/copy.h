// Copyright 2021 The ChromiumOS Authors
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
  ImageToImage(uint64_t width, std::string color, vkBase* base);
  ~ImageToImage() override = default;

  const char* Unit() const override { return "us"; }
  Image GetImage() const override;

 protected:
  void Initialize() override;
  void Destroy() override;

 private:
  std::string color_;
  vk::Extent2D img_extent_;
  vk::Format img_format_ = vk::Format::eR8G8B8A8Unorm;

  vkbench::vkImage *src_img_, *dest_img_;

  DISALLOW_COPY_AND_ASSIGN(ImageToImage);
};

class BufferToImage : public commandTestBase {
 public:
  BufferToImage(uint64_t width,
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
  vk::Extent2D img_extent_;
  vk::Format img_format_ = vk::Format::eR8G8B8A8Unorm;

  vkbench::vkBuffer* buffer_;
  vkbench::vkImage* img_;
  vkbench::vkMemoryRequirement buffer_memory_requirement_;

  DISALLOW_COPY_AND_ASSIGN(BufferToImage);
};

class BufferStream : public singleCommandTestBase {
 public:
  BufferStream(vkBase* base);
  ~BufferStream() override = default;

  const char* Unit() const override { return "mbytes_sec"; }
  double FormatMeasurement(double time) override {
    return sizeof(vertices_) / time;
  }

 protected:
  void Initialize() override;
  void Setup(int i) override;
  void Destroy() override;

 private:
  void CreateRenderPass();
  void CreateGraphicsPipeline();

  // clockwise such that the triangle is culled
  const float vertices_[3][2] = {
      {0.0f, 0.0f},
      {0.0f, 1.0f},
      {1.0f, 0.0f},
  };
  uint32_t vertex_buffer_size_ = 1024 * 1024;

  vkbench::vkBuffer* src_buffer_;
  vkbench::vkBuffer* dst_buffer_;

  vk::Extent2D img_extent_;
  vk::Format img_format_ = vk::Format::eR8G8B8A8Unorm;
  vkbench::vkImage* img_;
  vk::Framebuffer frame_buffer_;

  vk::RenderPassBeginInfo render_pass_info_;
  vk::RenderPass render_pass_;
  vk::Pipeline graphics_pipeline_;
  vk::PipelineLayout pipeline_layout_;
  DISALLOW_COPY_AND_ASSIGN(BufferStream);
};

// GenTests generate all the submit tests.
std::vector<vkbench::testBase*> GenTests();
}  // namespace copy
}  // namespace vkbench
#endif  // SRC_TESTS_COPY_H_
