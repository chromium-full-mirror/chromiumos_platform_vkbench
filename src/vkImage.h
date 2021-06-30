// Copyright 2021 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_VKIMAGE_H_
#define SRC_VKIMAGE_H_

#include <vulkan/vulkan.hpp>

#include <vector>

#include "src/vkBase.h"

namespace vkbench {
class vkImage {
 public:
  vkImage(vkBase* vkbase,
          vk::Extent2D img_size,
          vk::Format img_format,
          vk::ImageUsageFlags usage,
          vkbench::vkMemoryRequirement requirement,
          vk::ImageTiling tiling = vk::ImageTiling::eOptimal);
  ~vkImage();

  vk::Image GetImage() const { return img_; }
  void* GetMappedMemory();
  void UnmapMemory();
  // MoveLayout records the command to move the image from src to dst layout.
  void MoveLayout(vk::CommandBuffer cmd,
                  vk::ImageLayout src,
                  vk::ImageLayout dst);
  // FillImage fills the image by the fill_function.
  void FillImage(std::array<uint8_t, 4> (*fill_function)(int x, int y));

  // GetReadableImage copys a image created with eTransferSrc and returns a
  // readable vkImage in vk::ImageLayout::eGeneral layout.
  vkImage* GetReadableImage(vk::ImageLayout src_layout,
                            vk::Extent2D size,
                            vk::Format format);

 private:
  vkBase* vkbase_;

  vk::Image img_;
  vk::Extent2D size_;
  vk::Format format_;
  vk::ImageTiling tiling_;
  vk::DeviceMemory memory_;
  void* mapped_memory_;

  DISALLOW_COPY_AND_ASSIGN(vkImage);
};

}  // namespace vkbench
#endif  // SRC_VKIMAGE_H_
