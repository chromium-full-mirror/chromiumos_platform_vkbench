// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_VKBUFFER_H_
#define SRC_VKBUFFER_H_

#include <vulkan/vulkan.hpp>

#include <vector>

#include "src/vkBase.h"

namespace vkbench {
class vkBuffer {
 public:
  vkBuffer(vkBase* vkbase,
           uint32_t size,
           vk::BufferUsageFlags usage,
           vkbench::vkMemoryRequirement requirement);
  ~vkBuffer();

  vk::Buffer GetBuffer() const { return buffer_; }
  uint32_t GetSize() const { return size_; }
  void* GetMappedMemory();
  void UnmapMemory();

 private:
  vkBase* vkbase_;

  vk::Buffer buffer_;
  uint32_t size_;
  vk::DeviceMemory memory_;
  void* mapped_memory_;

  DISALLOW_COPY_AND_ASSIGN(vkBuffer);
};

}  // namespace vkbench
#endif  // SRC_VKBUFFER_H_
