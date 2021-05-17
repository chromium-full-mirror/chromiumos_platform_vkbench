// Copyright 2021 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./copy.h"
#include <fmt/format.h>
extern FilePath g_spirv_dir;

namespace vkbench {
namespace copy {
const char* TEST_PREFIX = "copy";
const std::vector<std::string> COLORS = {"noise", "white"};

CopyImage::CopyImage(uint64_t size, std::string color, vkBase* base) {
  vk = base;
  img_size_.setHeight(size).setWidth(size);
  color_ = color;
  name_ = fmt::format("{}.CopyImage.{}.{}", TEST_PREFIX, color, size);
  desp_ = fmt::format("Copy {}x{} size {} image between buffers.", size, size,
                      color);
}

void CopyImage::Initialize() {
  vkImage* temp_img = new vkImage(vk, img_size_, img_format_,
                                  vk::ImageUsageFlagBits::eTransferSrc,
                                  vk::MemoryPropertyFlagBits::eHostCoherent |
                                      vk::MemoryPropertyFlagBits::eHostVisible,
                                  vk::ImageTiling::eLinear);
  DEFER(delete temp_img);
  src_img_ = new vkbench::vkImage(vk, img_size_, img_format_,
                                  vk::ImageUsageFlagBits::eTransferSrc |
                                      vk::ImageUsageFlagBits::eTransferDst,
                                  vk::MemoryPropertyFlagBits::eDeviceLocal,
                                  vk::ImageTiling::eOptimal);
  dest_img_ = new vkbench::vkImage(
      vk, img_size_, img_format_, vk::ImageUsageFlagBits::eTransferDst,
      vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageTiling::eOptimal);
  if (color_ == "white") {
    temp_img->FillImage([](int x, int y) {
      return std::array<uint8_t, 4>{255, 255, 255, 255};
    });
  } else if (color_ == "noise") {
    // Use fixed random seed so it can be reproduced.
    srand(0);
    temp_img->FillImage([](int x, int y) {
      uint8_t r = 1.0 * rand() / RAND_MAX * 255;
      uint8_t g = 1.0 * rand() / RAND_MAX * 255;
      uint8_t b = 1.0 * rand() / RAND_MAX * 255;
      return std::array<uint8_t, 4>{r, g, b, 255};
    });
  } else {
    RUNTIME_ERROR("Unsupported colors for copy test: %s", color_.c_str());
  }

  std::vector<vk::CommandBuffer> cmds = vk->GetDevice().allocateCommandBuffers(
      {vk->GetCommandPool(), vk::CommandBufferLevel::ePrimary, 1});
  DEFER(vk->GetDevice().freeCommandBuffers(vk->GetCommandPool(), cmds));
  cmds[0].begin(vk::CommandBufferBeginInfo());
  dest_img_->MoveLayout(cmds[0], vk::ImageLayout::ePreinitialized,
                        vk::ImageLayout::eTransferDstOptimal);
  temp_img->MoveLayout(cmds[0], vk::ImageLayout::ePreinitialized,
                       vk::ImageLayout::eTransferSrcOptimal);
  src_img_->MoveLayout(cmds[0], vk::ImageLayout::ePreinitialized,
                       vk::ImageLayout::eTransferDstOptimal);

  // Copy the generated linear tiling image to our source image.
  vk::ImageCopy copy_region;
  copy_region.srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  copy_region.dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  copy_region.extent.setWidth(img_size_.width)
      .setHeight(img_size_.height)
      .setDepth(1);
  cmds[0].copyImage(temp_img->GetImage(), vk::ImageLayout::eTransferSrcOptimal,
                    src_img_->GetImage(), vk::ImageLayout::eTransferDstOptimal,
                    1, &copy_region);
  src_img_->MoveLayout(cmds[0], vk::ImageLayout::eTransferDstOptimal,
                       vk::ImageLayout::eTransferSrcOptimal);
  cmds[0].end();
  vk::SubmitInfo info;
  info.setCommandBufferCount(cmds.size()).setPCommandBuffers(cmds.data());
  vk->GetGFXQueue().submit(info, {});
  vk->GetGFXQueue().waitIdle();

  // Create the command buffers to test the speed of copying.
  cmd_buffers_ = vk->GetDevice().allocateCommandBuffers(
      {vk->GetCommandPool(), vk::CommandBufferLevel::ePrimary, 1});
  vk::ImageCopy img_copy_region;
  img_copy_region.srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  img_copy_region.dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  img_copy_region.extent.setWidth(img_size_.width)
      .setHeight(img_size_.height)
      .setDepth(1);

  cmd_buffers_[0].begin(vk::CommandBufferBeginInfo());
  cmd_buffers_[0].copyImage(
      src_img_->GetImage(), vk::ImageLayout::eTransferSrcOptimal,
      dest_img_->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1,
      &img_copy_region);
  cmd_buffers_[0].end();
  info.setCommandBufferCount(1).setPCommandBuffers(cmd_buffers_.data());
  smt_infos_.push_back(info);
}

void CopyImage::Run() {
  vk->GetGFXQueue().submit(smt_infos_, {});
  vk->GetGFXQueue().waitIdle();
}

void CopyImage::Destroy() {
  vk::Device device = vk->GetDevice();
  delete src_img_;
  delete dest_img_;
  device.freeCommandBuffers(vk->GetCommandPool(), cmd_buffers_);
  smt_infos_.clear();
}

Image CopyImage::GetImage() const {
  const vk::Device device = vk->GetDevice();
  vkImage* dest = vk->GetReadableImage(
      src_img_, vk::ImageLayout::eTransferSrcOptimal, img_size_, img_format_);
  DEFER(delete dest);
  vk::SubresourceLayout sub_resource_layout = device.getImageSubresourceLayout(
      dest->GetImage(), {vk::ImageAspectFlagBits::eColor, 0, 0});
  return Image((const unsigned char*)dest->GetMappedMemory(), img_size_,
               sub_resource_layout);
}

std::vector<testBase*> GenTests() {
  std::vector<testBase*> tests;
  for (auto size : {64, 128, 512, 1024}) {
    for (auto color : COLORS) {
      tests.push_back(new CopyImage(size, color, vkBase::GetInstance()));
    }
  }
  return tests;
}

}  // namespace copy
}  // namespace vkbench
