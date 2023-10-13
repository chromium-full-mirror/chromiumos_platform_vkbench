// Copyright 2020 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_VKBASE_H_
#define SRC_VKBASE_H_

#include <vulkan/vulkan.hpp>

#include <string>
#include <vector>

#include "src/utils.h"

namespace vkbench {

class vkMemoryRequirement {
 public:
  // include contains the requirements that the memory must have.
  vk::MemoryPropertyFlags include;
  // exclude contains the requirements that the memory must not have.
  vk::MemoryPropertyFlags exclude;
  // prefer_include contains the requirements that the memory prefer to have.
  vk::MemoryPropertyFlags prefer_include;
  // prefer_exclude contains the requirements that the memory prefer to not
  // have.
  vk::MemoryPropertyFlags prefer_exclude;

  // String returns strings representation for logging.
  std::string String();
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
  uint32_t GetMemoryType(uint32_t bits, vkMemoryRequirement requirement);
  // Submit submits the CommandBuffers.
  void Submit(const vk::ArrayProxy<vk::CommandBuffer> cmds);
  // SubmitAndWait submits CommandBuffers and wait for graphics pipe to become
  // idle.
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
