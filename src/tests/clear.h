// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_TESTS_CLEAR_H_
#define SRC_TESTS_CLEAR_H_

#include "../testBase.h"

namespace vkbench {
namespace clear {

class Clear : public testBase {
 public:
  Clear(vkBase* base);
  ~Clear() override = default;

  const char* Name() const override { return name_.c_str(); }
  const char* Desp() const override { return desp_.c_str(); }
  const char* Unit() const override { return "mpixels_sec"; }
  double FormatMeasurement(double time) override {
    return img_size_.width * img_size_.height / time;
  }
  Image GetImage() const override;

 protected:
  void Initialize() override;
  void Run() override;
  void Destroy() override;

 private:
  void CreateRenderPass();
  void CreateFrameBuffers();

  std::string name_;
  std::string desp_;
  vk::ClearValue clear_color_;
  std::vector<vk::SubmitInfo> smt_infos_;
  std::vector<vk::CommandBuffer> cmd_buffers_;
  vk::Extent2D img_size_{256, 256};
  vk::Format img_format_ = vk::Format::eR8G8B8A8Unorm;

  vk::RenderPass render_pass_;
  vk::Framebuffer frame_buffer_;
  vkImage* img_;
  vk::ImageView img_view_;

  DISALLOW_COPY_AND_ASSIGN(Clear);
};
// GenTests generate all the clear tests.
std::vector<vkbench::testBase*> GenTests();

}  // namespace clear
}  // namespace vkbench
#endif  // SRC_TESTS_CLEAR_H_
