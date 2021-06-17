// Copyright 2021 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fmt/format.h>
#include <limits>
#include <vector>

#include "src/tests/submit.h"

namespace vkbench {
namespace submit {

const char* TEST_PREFIX = "submit";

BulkSubmit::BulkSubmit(uint64_t submitCnt, vkBase* base) {
  vk = base;
  name_ = fmt::format("{}.{}.{}", TEST_PREFIX, "BulkSubmit", submitCnt);
  desp_ = fmt::format(
      "Call vkQueueSubmit with {} different empty command buffers each time. "
      "Then vkQueueWaitIdle.",
      submitCnt);
  smt_infos_.resize(submitCnt);
}
void BulkSubmit::Initialize() {
  cmd_buffers_ = vk->GetDevice().allocateCommandBuffers(
      {vk->GetCommandPool(), vk::CommandBufferLevel::ePrimary,
       uint32_t(smt_infos_.size())});

  for (uint32_t i = 0; i < smt_infos_.size(); i++) {
    // Create empty cmdBuffer
    cmd_buffers_[i].begin(vk::CommandBufferBeginInfo());
    cmd_buffers_[i].end();

    smt_infos_[i].setCommandBufferCount(1);
    smt_infos_[i].setPCommandBuffers(&cmd_buffers_[i]);
  }
}

void BulkSubmit::Run() {
  vk->GetGFXQueue().submit(smt_infos_, {});
  vk->GetGFXQueue().waitIdle();
}

void BulkSubmit::Destroy() {
  vk->GetDevice().freeCommandBuffers(vk->GetCommandPool(), cmd_buffers_);
}

MultiSubmitWaitIdle::MultiSubmitWaitIdle(uint64_t submitCnt, vkBase* base) {
  vk = base;

  name_ =
      fmt::format("{}.{}.{}", TEST_PREFIX, "MultiSubmitWaitIdle", submitCnt);
  desp_ = fmt::format(
      "Calls vkQueueSubmit {} times, with the different command buffer each "
      "time. Then vkQueueWaitIdle.",
      submitCnt);
  smt_infos_.resize(submitCnt);
}

void MultiSubmitWaitIdle::Initialize() {
  cmd_buffers_ = vk->GetDevice().allocateCommandBuffers(
      {vk->GetCommandPool(), vk::CommandBufferLevel::ePrimary,
       uint32_t(smt_infos_.size())});
  for (uint32_t i = 0; i < smt_infos_.size(); i++) {
    // Create empty cmdBuffer
    cmd_buffers_[i].begin(vk::CommandBufferBeginInfo());
    cmd_buffers_[i].end();

    smt_infos_[i].setCommandBufferCount(1);
    smt_infos_[i].setPCommandBuffers(&cmd_buffers_[i]);
  }
}

void MultiSubmitWaitIdle::Run() {
  for (auto& smt_info : smt_infos_) {
    vk->GetGFXQueue().submit(smt_info, {});
  }
  vk->GetGFXQueue().waitIdle();
}

void MultiSubmitWaitIdle::Destroy() {
  vk->GetDevice().freeCommandBuffers(vk->GetCommandPool(), cmd_buffers_);
}

MultiSubmitMultiWaitForFence::MultiSubmitMultiWaitForFence(uint64_t submitCnt,
                                                           vkBase* base) {
  vk = base;
  name_ = fmt::format("{}.{}.{}", TEST_PREFIX, "MultiSubmitMultiWaitForFence",
                      submitCnt);
  desp_ = fmt::format(
      "Calls vkQueueSubmit {} times, with one fence per submission, and with "
      "different empty command buffer each time. Then call vkWaitForFences "
      "for each fence.",
      submitCnt);
  smt_infos_.resize(submitCnt);
  fences_.resize(submitCnt);
}

void MultiSubmitMultiWaitForFence::Initialize() {
  cmd_buffers_ = vk->GetDevice().allocateCommandBuffers(
      {vk->GetCommandPool(), vk::CommandBufferLevel::ePrimary,
       uint32_t(smt_infos_.size())});
  for (uint32_t i = 0; i < smt_infos_.size(); i++) {
    // Create empty cmdBuffer
    cmd_buffers_[i].begin(vk::CommandBufferBeginInfo());
    cmd_buffers_[i].end();

    smt_infos_[i].setCommandBufferCount(1);
    smt_infos_[i].setPCommandBuffers(&cmd_buffers_[i]);
    // Create fence for each submit.
    fences_[i] = vk->GetDevice().createFence({});
  }
}

void MultiSubmitMultiWaitForFence::Run() {
  vk::Device device = vk->GetDevice();
  for (uint64_t i = 0; i < smt_infos_.size(); i++) {
    vk->GetGFXQueue().submit(smt_infos_[i], fences_[i], {});
  }
  for (uint64_t i = 0; i < smt_infos_.size(); i++) {
    device.waitForFences(fences_[i], VK_TRUE,
                         std::numeric_limits<uint64_t>::max());
    device.resetFences(fences_[i]);
  }
}

void MultiSubmitMultiWaitForFence::Destroy() {
  vk->GetDevice().freeCommandBuffers(vk->GetCommandPool(), cmd_buffers_);
  for (auto& fence : fences_) {
    vk->GetDevice().destroyFence(fence);
  }
}

MultiSubmitBulkWaitForFence::MultiSubmitBulkWaitForFence(uint64_t submitCnt,
                                                         vkBase* base) {
  vk = base;
  name_ = fmt::format("{}.{}.{}", TEST_PREFIX, "MultiSubmitBulkWaitForFence",
                      submitCnt);
  desp_ =
      "MultiSubmitBulkWaitForFence is similar to MultiSubmitMultiWaitForFence "
      "but call vkWaitForFences once with all fences.",
  smt_infos_.resize(submitCnt);
  fences_.resize(submitCnt);
}

void MultiSubmitBulkWaitForFence::Initialize() {
  cmd_buffers_ = vk->GetDevice().allocateCommandBuffers(
      {vk->GetCommandPool(), vk::CommandBufferLevel::ePrimary,
       uint32_t(smt_infos_.size())});
  for (uint32_t i = 0; i < smt_infos_.size(); i++) {
    // Create empty cmdBuffer
    cmd_buffers_[i].begin(vk::CommandBufferBeginInfo());
    cmd_buffers_[i].end();

    smt_infos_[i].setCommandBufferCount(1);
    smt_infos_[i].setPCommandBuffers(&cmd_buffers_[i]);
    // Create fence for each submit.
    fences_[i] = vk->GetDevice().createFence({});
  }
}

void MultiSubmitBulkWaitForFence::Run() {
  vk::Device device = vk->GetDevice();
  for (uint64_t i = 0; i < smt_infos_.size(); i++) {
    vk->GetGFXQueue().submit(smt_infos_[i], fences_[i], {});
  }
  device.waitForFences(fences_, VK_TRUE, std::numeric_limits<uint64_t>::max());
  device.resetFences(fences_);
}

void MultiSubmitBulkWaitForFence::Destroy() {
  vk->GetDevice().freeCommandBuffers(vk->GetCommandPool(), cmd_buffers_);
  for (auto& fence : fences_) {
    vk->GetDevice().destroyFence(fence);
  }
}

// GenTests generate all the submit tests.
std::vector<vkbench::testBase*> GenTests() {
  return std::vector<vkbench::testBase*>{
      new BulkSubmit(16, vkBase::GetInstance()),
      new BulkSubmit(64, vkBase::GetInstance()),
      new BulkSubmit(256, vkBase::GetInstance()),
      new BulkSubmit(1024, vkBase::GetInstance()),
      new MultiSubmitMultiWaitForFence(16, vkBase::GetInstance()),
      new MultiSubmitMultiWaitForFence(64, vkBase::GetInstance()),
      new MultiSubmitMultiWaitForFence(256, vkBase::GetInstance()),
      new MultiSubmitMultiWaitForFence(1024, vkBase::GetInstance()),
      new MultiSubmitBulkWaitForFence(16, vkBase::GetInstance()),
      new MultiSubmitBulkWaitForFence(64, vkBase::GetInstance()),
      new MultiSubmitBulkWaitForFence(256, vkBase::GetInstance()),
      new MultiSubmitBulkWaitForFence(1024, vkBase::GetInstance()),
      new MultiSubmitWaitIdle(16, vkBase::GetInstance()),
      new MultiSubmitWaitIdle(64, vkBase::GetInstance()),
      new MultiSubmitWaitIdle(256, vkBase::GetInstance()),
      new MultiSubmitWaitIdle(1024, vkBase::GetInstance()),
  };
}

}  // namespace submit
}  // namespace vkbench
