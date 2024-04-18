// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/vkBuffer.h"

namespace vkbench {
vkBuffer::vkBuffer(vkBase* vkbase,
                   uint32_t size,
                   vk::BufferUsageFlags usage,
                   vkbench::vkMemoryRequirement requirement) {
  vkbase_ = vkbase;
  size_ = size;
  mapped_memory_ = nullptr;

  vk::BufferCreateInfo create_info;
  create_info.setSize(size).setUsage(usage);

  vk::Device device = vkbase_->GetDevice();
  buffer_ = device.createBuffer(create_info);
  vk::MemoryRequirements mem_req = device.getBufferMemoryRequirements(buffer_);
  memory_ = device.allocateMemory(
      {mem_req.size,
       vkbase_->GetMemoryType(mem_req.memoryTypeBits, requirement)});
  device.bindBufferMemory(buffer_, memory_, 0);
}

void* vkBuffer::GetMappedMemory() {
  if (mapped_memory_ != nullptr) {
    return mapped_memory_;
  }
  mapped_memory_ = vkbase_->GetDevice().mapMemory(memory_, 0, VK_WHOLE_SIZE);
  return mapped_memory_;
}

void vkBuffer::UnmapMemory() {
  if (mapped_memory_ == nullptr) {
    return;
  }

  vkbase_->GetDevice().unmapMemory(memory_);
  mapped_memory_ = nullptr;
}

vkBuffer::~vkBuffer() {
  vkbase_->GetDevice().freeMemory(memory_);
  vkbase_->GetDevice().destroyBuffer(buffer_);
}

}  // namespace vkbench
