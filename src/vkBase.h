// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __VKBENCH_VKBASE_H__
#define __VKBENCH_VKBASE_H__

#include <vector>
#include <vulkan/vulkan.hpp>

#include "utils.h"

namespace vkbench {
class vkBase;

class vkImage {
 public:
  vkImage(vkBase* vkbase,
          vk::Extent2D img_size,
          vk::Format img_format,
          vk::ImageUsageFlags usage,
          vk::MemoryPropertyFlags memory_properties,
          vk::ImageTiling tiling);
  ~vkImage();

  vk::Image GetImage() const { return img_; }
  vk::DeviceMemory GetMemory() const { return memory_; }

  vkBase* vkbase_;
  vk::Image img_;
  vk::DeviceMemory memory_;
};

class vkBase {
 public:
  virtual ~vkBase() = default;
  virtual void Initialize();
  virtual void Destroy();

  const vk::Device& GetDevice() const { return device_; }
  const vk::Queue& GetGFXQueue() const { return gfx_queue_; }
  const vk::CommandPool& GetCommandPool() const { return cmd_pool_; }
  const uint32_t& GetGFXQueueFamilyIndex() const { return gfx_queue_idx_; }

  vkImage* AllocateImage(vk::Extent2D img_size,
                         vk::Format img_format,
                         vk::ImageUsageFlags usage,
                         vk::MemoryPropertyFlags memory_properties = {},
                         vk::ImageTiling tiling = vk::ImageTiling::eOptimal);
  // Helper function
  uint32_t GetMemoryType(uint32_t bits, vk::MemoryPropertyFlags properties);
  vkImage* GetReadableImage(vkImage* src, vk::Extent2D size, vk::Format format);

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
};

}  // namespace vkbench
#endif
