// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "submitTest.h"

namespace vkbench {
void SubmitTest::Initialize() {
  fences_.push_back(
      vk->GetDevice().createFence({vk::FenceCreateFlagBits::eSignaled}));
  // Create empty cmdBuffer
  cmd_buffers_ = vk->GetDevice().allocateCommandBuffers(
      {vk->GetCommandPool(), vk::CommandBufferLevel::ePrimary, 1});
  cmd_buffers_[0].begin({});
  cmd_buffers_[0].end();

  for (auto& t : smt_infos_) {
    t.setCommandBufferCount(1);
    t.setPCommandBuffers(&cmd_buffers_[0]);
  }
}

void SubmitTest::Run() {
  // TODO(pwang): Consider using multiple set of command buffers to minimize
  // the CPU idle time of waiting the fences.
  vk->GetDevice().waitForFences(fences_, VK_TRUE,
                                std::numeric_limits<uint64_t>::max());
  vk->GetDevice().resetFences(fences_);
  vk->GetGFXQueue().submit(smt_infos_, fences_[0], {});
}

void SubmitTest::Cleanup() {
  vk->GetDevice().waitForFences(fences_, VK_TRUE,
                                std::numeric_limits<uint64_t>::max());
}

void SubmitTest::Destroy() {
  vk->GetDevice().freeCommandBuffers(vk->GetCommandPool(), cmd_buffers_);
  for (auto& fence : fences_) {
    vk->GetDevice().destroyFence(fence);
  }
  fences_.clear();
}
}  // namespace vkbench
