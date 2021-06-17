// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_VKBASE_H_
#define SRC_VKBASE_H_

#include <vulkan/vulkan.hpp>

#include <vector>

#include "src/utils.h"

namespace vkbench {
class vkBase;

class vkImage {
 public:
  vkImage(vkBase* vkbase,
          vk::Extent2D img_size,
          vk::Format img_format,
          vk::ImageUsageFlags usage,
          vk::MemoryPropertyFlags memory_properties,
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

class vkBase {
 public:
  virtual ~vkBase() = default;
  static vkBase* GetInstance();

  virtual void Initialize();
  virtual bool IsInitialized() const;
  virtual void Destroy();

  const vk::Device& GetDevice() const { return device_; }
  const vk::Queue& GetGFXQueue() const { return gfx_queue_; }
  const vk::CommandPool& GetCommandPool() const { return cmd_pool_; }
  const uint32_t& GetGFXQueueFamilyIndex() const { return gfx_queue_idx_; }

  // Helper function
  // GetMemoryType returns the index of a memory type that has all the requested
  // property bits set.
  uint32_t GetMemoryType(uint32_t bits, vk::MemoryPropertyFlags properties);
  // GetReadableImage copys a image created with eTransferSrc and returns a
  // readable vkImage in vk::ImageLayout::eGeneral layout.
  vkImage* GetReadableImage(vkImage* src,
                            vk::ImageLayout src_layout,
                            vk::Extent2D size,
                            vk::Format format);
  // SubmitAndWait submits CommandBuffers and wait for its completion.
  void SubmitAndWait(const vk::ArrayProxy<vk::CommandBuffer> cmds);

 protected:
  // Vulkan general method.
  virtual void CreateInstance();
  virtual void ChoosePhysicalDevice();
  virtual void CreateLogicalDevice();
  virtual void CreateCommandPool();

  // Vulkan handles
  std::vector<const char*> validation_layers_{
      "VK_LAYER_LUNARG_standard_validation", "VK_LAYER_KHRONOS_validation"};
  std::vector<const char*> debug_extension_ = {"VK_EXT_debug_utils"};
  bool enable_validation_layer_ = false;

  vk::DebugUtilsMessengerEXT debug_messenger_;
  vk::Instance instance_;
  vk::PhysicalDevice physical_device_;
  vk::PhysicalDeviceMemoryProperties mem_properties_;
  vk::Device device_;
  vk::Queue gfx_queue_;
  vk::CommandPool cmd_pool_;
  uint32_t gfx_queue_idx_ = -1;

  vk::SurfaceKHR surface_;
  bool initialized_ = false;

 private:
  vkBase() {}
  static vkBase* singleton_;
  DISALLOW_COPY_AND_ASSIGN(vkBase);
};

}  // namespace vkbench
#endif  // SRC_VKBASE_H_
