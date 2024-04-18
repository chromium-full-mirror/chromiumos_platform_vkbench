// Copyright 2020 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_TESTS_CLEAR_H_
#define SRC_TESTS_CLEAR_H_

#include <string>
#include <vector>

#include "src/testBase.h"
#include "src/vkImage.h"

namespace vkbench {
namespace clear {

class LoadOp : public commandTestBase {
 public:
  LoadOp(vkBase* base,
         vk::Format format,
         vk::ImageTiling tiling,
         uint32_t width,
         std::string color);
  ~LoadOp() override = default;

  const char* Unit() const override { return "mpixels_sec"; }
  double FormatMeasurement(double time) override;
  Image GetImage() const override;

 protected:
  void Initialize() override;
  void Destroy() override;

 private:
  void CreateRenderPass();
  void CreateFrameBuffers();

  vk::Format img_format_;
  vk::ImageTiling img_tiling_;
  vk::Extent2D img_extent_;
  vk::ClearValue clear_color_;
  vk::RenderPass render_pass_;
  vk::Framebuffer frame_buffer_;
  vkImage* img_;

  DISALLOW_COPY_AND_ASSIGN(LoadOp);
};

class CmdClearImage : public commandTestBase {
 public:
  CmdClearImage(vkBase* base,
                vk::Format format,
                vk::ImageTiling tiling,
                uint32_t width,
                std::string color);
  ~CmdClearImage() override = default;

  const char* Unit() const override { return "mpixels_sec"; }
  double FormatMeasurement(double time) override;
  Image GetImage() const override;

 protected:
  void Initialize() override;
  void Destroy() override;

 private:
  vk::Format img_format_;
  vk::ImageTiling img_tiling_;
  vk::Extent2D img_extent_;
  vk::ClearColorValue clear_color_;
  vkImage* img_;

  DISALLOW_COPY_AND_ASSIGN(CmdClearImage);
};

// GenTests generate all the clear tests.
std::vector<vkbench::testBase*> GenTests();

}  // namespace clear
}  // namespace vkbench
#endif  // SRC_TESTS_CLEAR_H_
