// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "submitTest.h"
#include "testUtil.h"

namespace vkbench {
void SubmitTest::TestInitialization() {
  fences_.push_back(device_.createFence({vk::FenceCreateFlagBits::eSignaled}));
  // Create empty cmdBuffer
  cmd_buffers_ = device_.allocateCommandBuffers(
      {cmd_pool_, vk::CommandBufferLevel::ePrimary, 1});
  cmd_buffers_[0].begin({vk::CommandBufferUsageFlagBits::eSimultaneousUse});
  cmd_buffers_[0].end();

  for (auto& t : smt_infos_) {
    t.setCommandBufferCount(1);
    t.setPCommandBuffers(&cmd_buffers_[0]);
  }
}

bool SubmitTest::TestRun() {
  device_.waitForFences(fences_, VK_TRUE, std::numeric_limits<uint64_t>::max());
  device_.resetFences(fences_);
  gfx_queue_.submit(smt_infos_, fences_[0], {});
  return true;
}

void SubmitTest::TestCleanup() {
  device_.waitForFences(fences_, VK_TRUE, std::numeric_limits<uint64_t>::max());
}

void SubmitTest::TestDestroy() {
  if (!device_)
    return;

  if (cmd_pool_)
    device_.freeCommandBuffers(cmd_pool_, cmd_buffers_);
  for (auto &fence : fences_) {
    device_.destroyFence(fence);
  }
}
}  // namespace vkbench
