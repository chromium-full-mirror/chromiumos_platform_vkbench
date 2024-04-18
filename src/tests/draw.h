// Copyright 2020 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_TESTS_DRAW_H_
#define SRC_TESTS_DRAW_H_

#include <string>
#include <vector>

#include "src/testBase.h"
#include "src/vkImage.h"

namespace vkbench {
namespace draw {

class Triangle : public commandTestBase {
 public:
  Triangle(uint64_t width, vkBase* base);
  ~Triangle() override = default;

  const char* Unit() const override { return "us"; }
  Image GetImage() const override;

 protected:
  void Initialize() override;
  void Destroy() override;

 private:
  void CreateRenderPass();
  void CreateGraphicsPipeline();

  vk::Extent2D img_extent_;
  vk::Format img_format_ = vk::Format::eR8G8B8A8Unorm;

  vk::RenderPass render_pass_;
  vk::Pipeline graphics_pipeline_;
  vk::PipelineLayout pipeline_layout_;
  vk::Framebuffer frame_buffer_;
  vkImage* img_;
  DISALLOW_COPY_AND_ASSIGN(Triangle);
};

// GenTests generate all the draw tests.
std::vector<vkbench::testBase*> GenTests();
}  // namespace draw
}  // namespace vkbench
#endif  // SRC_TESTS_DRAW_H_
