// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VKBENCH_DRAWSIZETEST_H
#define VKBENCH_DRAWSIZETEST_H

#include "testBase.h"

namespace vkbench {
class DrawSizeTest : public testBase {
 public:
  DrawSizeTest(uint64_t size, vkBase* base) {
    vk = base;
    img_size_.setHeight(size).setWidth(size);
    snprintf(name_, 100, "DrawSizeTest@%lu", img_size_.height);
    snprintf(desp_, 1024, "Time to draw %lux%lu image.", img_size_.height,
             img_size_.width);
  }
  ~DrawSizeTest() override = default;

  const char* Name() const override { return name_; }
  const char* Desp() const override { return desp_; }
  const char* Unit() const override { return "us"; }
  void SaveImage(std::string filename) const override;

 protected:
  void Initialize() override;
  void Run() override;
  void Destroy() override;

 private:
  void CreateRenderPass();
  void CreateGraphicsPipeline();
  void CreateFrameBuffers();

  char name_[100];
  char desp_[1024];
  std::vector<vk::SubmitInfo> smt_infos_;
  std::vector<vk::CommandBuffer> cmd_buffers_;
  vk::Extent2D img_size_;
  vk::Format img_format_ = vk::Format::eR8G8B8A8Unorm;

  vk::RenderPass render_pass_;
  vk::Pipeline graphics_pipeline_;
  vk::PipelineLayout pipeline_layout_;
  vk::Framebuffer frame_buffer_;
  vkImage* img_;
  vk::ImageView img_view_;

  DISALLOW_COPY_AND_ASSIGN(DrawSizeTest);
};
}  // namespace vkbench
#endif  // VKBENCH_DRAWSIZETEST_H
