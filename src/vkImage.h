// Copyright 2021 The ChromiumOS Authors
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
          vk::Extent2D extent,
          vk::Format img_format,
          vk::ImageUsageFlags usage,
          vkbench::vkMemoryRequirement requirement,
          vk::ImageTiling tiling = vk::ImageTiling::eOptimal);
  ~vkImage();

  // GetExtent returns the extent of the image.
  const vk::Extent2D& GetExtent2D() const;
  // GetFormat returns the format of the image.
  const vk::Format& GetFormat() const;
  // GetImage returns the underlying vkImage object.
  vk::Image GetImage() const;
  // GetDefaultImageView returns a default image view. The image view would be
  // destroyed by vkImage destructor.
  vk::ImageView GetDefaultImageView();
  // Copy records the command to copy the content of the vkBuffer object
  // to the image.
  void CopyFrom(vk::CommandBuffer, vk::Buffer buffer);
  // GetMappedMemory returns the mapped memory of the underlying vkImage.
  void* GetMappedMemory();
  // UnmapMemory unmap the mapped memory of the underlying vkImage.
  void UnmapMemory();
  // MoveLayout records the command to move the image from src to dst layout.
  void MoveLayout(vk::CommandBuffer cmd,
                  vk::ImageLayout src,
                  vk::ImageLayout dst);
  // FillImage fills the image by the fill_function.
  void FillImage(std::array<uint8_t, 4> (*fill_function)(int x, int y));
  // GetReadableImage copys a image created with eTransferSrc and returns a
  // readable vkImage in vk::ImageLayout::eGeneral layout.
  vkImage* GetReadableImage(vk::ImageLayout src_layout);

 protected:
  vkBase* vkbase_;

  vk::Image img_;
  vk::ImageView img_view_;
  vk::Extent2D extent_;
  vk::Format format_;
  vk::ImageTiling tiling_;
  vk::DeviceMemory memory_;
  void* mapped_memory_;

  DISALLOW_COPY_AND_ASSIGN(vkImage);
};

// DefaultFramebuffer returns a framebuffer with default settings.
vk::Framebuffer DefaultFramebuffer(vk::Device device,
                                   vkImage* img,
                                   vk::RenderPass render_pass,
                                   vk::ImageView img_view);

}  // namespace vkbench
#endif  // SRC_VKIMAGE_H_
