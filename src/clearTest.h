// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VKBENCH_CLEARTEST_H
#define VKBENCH_CLEARTEST_H

#include "testBase.h"

namespace vkbench {
class ClearTest : public testBase {
 public:
  ClearTest(vkBase* base) {
    vk = base;
    clear_color_.setColor({std::array<float, 4>{0.f, 0.f, 1.f}});  // Blue
    snprintf(name_, 100, "ClearTest");
    snprintf(desp_, 1024, "Time to clear window to specific color.");
  }
  ~ClearTest() override = default;

  const char* Name() const override { return name_; }
  const char* Desp() const override { return desp_; }
  const char* Unit() const override { return "mpixels_sec"; }
  virtual double FormatMeasurement(double time) override {
    return img_size_.width * img_size_.height / time;
  }
  void SaveImage(std::string filename) const override;

 protected:
  void Initialize() override;
  void Run() override;
  void Destroy() override;

 private:
  void CreateRenderPass();
  void CreateFrameBuffers();

  char name_[100];
  char desp_[1024];
  vk::ClearValue clear_color_;
  std::vector<vk::SubmitInfo> smt_infos_;
  std::vector<vk::CommandBuffer> cmd_buffers_;
  vk::Extent2D img_size_{256, 256};
  vk::Format img_format_ = vk::Format::eR8G8B8A8Unorm;

  vk::RenderPass render_pass_;
  vk::Framebuffer frame_buffer_;
  vkImage* img_;
  vk::ImageView img_view_;

  DISALLOW_COPY_AND_ASSIGN(ClearTest);
};
}  // namespace vkbench
#endif  // VKBENCH_CLEARTEST_H
