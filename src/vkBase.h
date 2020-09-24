// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __VKBENCH_VKBASE_H__
#define __VKBENCH_VKBASE_H__

#include <vector>
#include <vulkan/vulkan.hpp>

#include "utils.h"

namespace vkbench {
class vkBase {
 public:
  virtual ~vkBase() = default;
  virtual void Initialize();
  virtual void Destroy();

  const vk::Device& GetDevice() { return device_; }
  const vk::Queue& GetGFXQueue() { return gfx_queue_; }
  const vk::CommandPool& GetCommandPool() { return cmd_pool_; }

 protected:
  // Vulkan general method.
  virtual void CreateInstance();
  virtual void ChoosePhysical_device_();
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
  vk::Device device_;
  vk::Queue gfx_queue_;
  vk::CommandPool cmd_pool_;
  uint32_t gfx_queue_idx_ = -1;

 private:
  bool initialized = false;
};

}  // namespace vkbench
#endif
