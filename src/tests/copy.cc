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

ImageToImage::ImageToImage(uint64_t width, std::string color, vkBase* base) {
  vk = base;
  img_extent_.setHeight(width).setWidth(width);
  color_ = color;
  name_ = fmt::format("{}.ImageToImage.{}.{}", TEST_PREFIX, color, width);
  desp_ = fmt::format("Copy {}x{} size {} image using vkCopyImage.", width,
                      width, color);
}

void ImageToImage::Initialize() {
  vkImage* temp_img = new vkbench::vkImage(
      vk, img_extent_, img_format_, vk::ImageUsageFlagBits::eTransferSrc,
      {vk::MemoryPropertyFlagBits::eHostCoherent |
       vk::MemoryPropertyFlagBits::eHostVisible},
      vk::ImageTiling::eLinear);
  DEFER(delete temp_img);
  src_img_ = new vkbench::vkImage(vk, img_extent_, img_format_,
                                  vk::ImageUsageFlagBits::eTransferSrc |
                                      vk::ImageUsageFlagBits::eTransferDst,
                                  {vk::MemoryPropertyFlagBits::eDeviceLocal},
                                  vk::ImageTiling::eOptimal);
  dest_img_ = new vkbench::vkImage(
      vk, img_extent_, img_format_, vk::ImageUsageFlagBits::eTransferDst,
      {vk::MemoryPropertyFlagBits::eDeviceLocal}, vk::ImageTiling::eOptimal);
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
    RUNTIME_ERROR("Unsupported colors for ImageToImage test: {}", color_);
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
  copy_region.extent.setWidth(img_extent_.width)
      .setHeight(img_extent_.height)
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
  img_copy_region.extent.setWidth(img_extent_.width)
      .setHeight(img_extent_.height)
      .setDepth(1);
  cmd.begin(vk::CommandBufferBeginInfo());
  cmd.copyImage(src_img_->GetImage(), vk::ImageLayout::eTransferSrcOptimal,
                dest_img_->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1,
                &img_copy_region);
  cmd.end();
}

void ImageToImage::Destroy() {
  vkbench::commandTestBase::Destroy();
  delete src_img_;
  delete dest_img_;
}

Image ImageToImage::GetImage() const {
  const vk::Device device = vk->GetDevice();
  vkImage* dest = src_img_->GetReadableImage(
      vk::ImageLayout::eTransferSrcOptimal, img_extent_, img_format_);
  DEFER(delete dest);
  vk::SubresourceLayout sub_resource_layout = device.getImageSubresourceLayout(
      dest->GetImage(), {vk::ImageAspectFlagBits::eColor, 0, 0});
  return Image((const unsigned char*)dest->GetMappedMemory(), img_extent_,
               sub_resource_layout);
}

BufferToImage::BufferToImage(uint64_t img_extent,
                             std::string color,
                             std::string buffer_memory_property,
                             vkBase* base) {
  vk = base;
  img_extent_.setHeight(img_extent).setWidth(img_extent);
  color_ = color;

  buffer_memory_requirement_.prefer_exclude =
      vk::MemoryPropertyFlagBits::eDeviceLocal;
  buffer_memory_requirement_.include = vk::MemoryPropertyFlagBits::eHostVisible;
  if (buffer_memory_property == "HostCoherent") {
    buffer_memory_requirement_.include =
        vk::MemoryPropertyFlagBits::eHostCoherent;
  } else if (buffer_memory_property == "NonHostCoherent") {
    buffer_memory_requirement_.exclude =
        vk::MemoryPropertyFlagBits::eHostCoherent;
  } else {
    NOT_SUPPORT("memory property {} is not supported yet.",
                buffer_memory_property);
  }
  name_ = fmt::format("{}.BufferToImage.{}.{}.{}", TEST_PREFIX,
                      buffer_memory_property, color, img_extent);
  desp_ = fmt::format(
      "Copy {} {} buffer to {}x{} image using vkCmdCopyBufferToImage.", color,
      buffer_memory_requirement_.String(), img_extent, img_extent);
}

void BufferToImage::Initialize() {
  // Create a buffer which capable to an image.
  uint32_t buffer_size = img_extent_.width * img_extent_.height * 4;
  buffer_ = new vkbench::vkBuffer(vk, buffer_size,
                                  vk::BufferUsageFlagBits::eTransferSrc |
                                      vk::BufferUsageFlagBits::eTransferDst,
                                  buffer_memory_requirement_);
  img_ = new vkbench::vkImage(vk, img_extent_, img_format_,
                              vk::ImageUsageFlagBits::eTransferSrc |
                                  vk::ImageUsageFlagBits::eTransferDst |
                                  vk::ImageUsageFlagBits::eColorAttachment,
                              {vk::MemoryPropertyFlagBits::eDeviceLocal},
                              vk::ImageTiling::eOptimal);

  // Fills the buffer with our data.
  auto data = (unsigned char*)buffer_->GetMappedMemory();
  for (auto i = 0; i < buffer_size; i++) {
    if (i % 4 == 3) {
      // Alpha channel, always set to 255.
      data[i] = 255;
      continue;
    }

    if (color_ == "white") {
      data[i] = 255;
    } else if (color_ == "noise") {
      data[i] = randi(255);
    } else {
      RUNTIME_ERROR("color {} not support", color_);
    }
  }

  vk::CommandBuffer cmd = GetCommandBuffer();
  cmd.begin(vk::CommandBufferBeginInfo());
  img_->MoveLayout(cmd, vk::ImageLayout::ePreinitialized,
                   vk::ImageLayout::eTransferDstOptimal);
  cmd.end();
  vk->SubmitAndWait(cmd);

  // Records the command to copy buffer to image.
  cmd.begin(vk::CommandBufferBeginInfo());
  vk::BufferImageCopy image_copy;
  // BufferRowLength=0 or BufferImageHeight=0 means buffer tightly packed
  // according to the imageExtent
  image_copy.setBufferOffset(0).setBufferRowLength(0).setBufferImageHeight(0);
  image_copy.setImageOffset(0)
      .setImageExtent(vk::Extent3D(img_extent_, 1))
      .setImageSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
  cmd.copyBufferToImage(buffer_->GetBuffer(), img_->GetImage(),
                        vk::ImageLayout::eTransferDstOptimal, image_copy);
  cmd.end();
}

void BufferToImage::Destroy() {
  vkbench::commandTestBase::Destroy();
  delete buffer_;
  delete img_;
}

Image BufferToImage::GetImage() const {
  const vk::Device device = vk->GetDevice();
  vkImage* dest = img_->GetReadableImage(vk::ImageLayout::eTransferDstOptimal,
                                         img_extent_, img_format_);
  DEFER(delete dest);
  vk::SubresourceLayout sub_resource_layout = device.getImageSubresourceLayout(
      dest->GetImage(), {vk::ImageAspectFlagBits::eColor, 0, 0});
  return Image((const unsigned char*)dest->GetMappedMemory(), img_extent_,
               sub_resource_layout);
}

std::vector<testBase*> GenTests() {
  std::vector<testBase*> tests;
  for (auto width : {64, 128, 512, 1024}) {
    for (auto color : COLORS) {
      tests.push_back(new ImageToImage(width, color, vkBase::GetInstance()));
    }
  }

  for (auto width : {256, 512, 1024}) {
    for (auto color : COLORS) {
      for (auto memory_type : {"HostCoherent", "NonHostCoherent"}) {
        tests.push_back(new BufferToImage(width, color, memory_type,
                                          vkBase::GetInstance()));
      }
    }
  }
  return tests;
}

}  // namespace copy
}  // namespace vkbench
