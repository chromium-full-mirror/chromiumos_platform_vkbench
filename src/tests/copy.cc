// Copyright 2021 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fmt/format.h>

#include "src/tests/copy.h"

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
  vkImage* temp_img = new vkbench::vkImage(
      vk, img_size_, img_format_, vk::ImageUsageFlagBits::eTransferSrc,
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
    temp_img->FillImage([](int x, int y) {
      return std::array<uint8_t, 4>{static_cast<uint8_t>(randi(255)),
                                    static_cast<uint8_t>(randi(255)),
                                    static_cast<uint8_t>(randi(255)), 255};
    });
  } else {
    RUNTIME_ERROR("Unsupported colors for copy test: {}", color_);
  }

  vk::CommandBuffer cmd = GetCommandBuffer();
  cmd.begin(vk::CommandBufferBeginInfo());
  dest_img_->MoveLayout(cmd, vk::ImageLayout::ePreinitialized,
                        vk::ImageLayout::eTransferDstOptimal);
  temp_img->MoveLayout(cmd, vk::ImageLayout::ePreinitialized,
                       vk::ImageLayout::eTransferSrcOptimal);
  src_img_->MoveLayout(cmd, vk::ImageLayout::ePreinitialized,
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
  cmd.copyImage(temp_img->GetImage(), vk::ImageLayout::eTransferSrcOptimal,
                src_img_->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1,
                &copy_region);
  src_img_->MoveLayout(cmd, vk::ImageLayout::eTransferDstOptimal,
                       vk::ImageLayout::eTransferSrcOptimal);
  cmd.end();
  vk->SubmitAndWait(cmd);

  // Create the command buffers to test the speed of copying.
  vk::ImageCopy img_copy_region;
  img_copy_region.srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  img_copy_region.dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  img_copy_region.extent.setWidth(img_size_.width)
      .setHeight(img_size_.height)
      .setDepth(1);
  cmd.begin(vk::CommandBufferBeginInfo());
  cmd.copyImage(src_img_->GetImage(), vk::ImageLayout::eTransferSrcOptimal,
                dest_img_->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1,
                &img_copy_region);
  cmd.end();
}

void CopyImage::Destroy() {
  vkbench::commandTestBase::Destroy();
  delete src_img_;
  delete dest_img_;
}

Image CopyImage::GetImage() const {
  const vk::Device device = vk->GetDevice();
  vkImage* dest = src_img_->GetReadableImage(
      vk::ImageLayout::eTransferSrcOptimal, img_size_, img_format_);
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
