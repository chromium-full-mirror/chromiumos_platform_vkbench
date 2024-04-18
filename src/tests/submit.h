// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_TESTS_SUBMIT_H_
#define SRC_TESTS_SUBMIT_H_

#include <string>
#include <vector>

#include "src/testBase.h"

namespace vkbench {
namespace submit {

// BulkSubmit submits multiple calls in a submit calls.
class BulkSubmit : public testBase {
 public:
  BulkSubmit(uint64_t submitCnt, vkBase* base);
  ~BulkSubmit() override = default;

  const char* Unit() const override { return "us"; }

 protected:
  void Initialize() override;
  void Run() override;
  void Destroy() override;

 private:
  std::vector<vk::SubmitInfo> smt_infos_;
  std::vector<vk::CommandBuffer> cmd_buffers_;
  DISALLOW_COPY_AND_ASSIGN(BulkSubmit);
};

// MultiSubmitWaitIdle calls vkSubmit multiple times and then wait idle once.
class MultiSubmitWaitIdle : public testBase {
 public:
  MultiSubmitWaitIdle(uint64_t submitCnt, vkBase* base);
  ~MultiSubmitWaitIdle() override = default;

  const char* Unit() const override { return "us"; }

 protected:
  void Initialize() override;
  void Run() override;
  void Destroy() override;

 private:
  std::vector<vk::Fence> fences_;
  std::vector<vk::SubmitInfo> smt_infos_;
  std::vector<vk::CommandBuffer> cmd_buffers_;
  DISALLOW_COPY_AND_ASSIGN(MultiSubmitWaitIdle);
};

// MultiSubmitMultiWaitForFence calls vkSubmits multiple times with multiple
// fences.
class MultiSubmitMultiWaitForFence : public testBase {
 public:
  MultiSubmitMultiWaitForFence(uint64_t submitCnt, vkBase* base);
  ~MultiSubmitMultiWaitForFence() override = default;
  const char* Unit() const override { return "us"; }

 protected:
  void Initialize() override;
  void Run() override;
  void Destroy() override;

 private:
  std::vector<vk::Fence> fences_;
  std::vector<vk::SubmitInfo> smt_infos_;
  std::vector<vk::CommandBuffer> cmd_buffers_;
  DISALLOW_COPY_AND_ASSIGN(MultiSubmitMultiWaitForFence);
};

// MultiSubmitBulkWaitForFence calls vkSubmits multiple times with multiple
// fences.
class MultiSubmitBulkWaitForFence : public testBase {
 public:
  MultiSubmitBulkWaitForFence(uint64_t submitCnt, vkBase* base);
  ~MultiSubmitBulkWaitForFence() override = default;
  const char* Unit() const override { return "us"; }

 protected:
  void Initialize() override;
  void Run() override;
  void Destroy() override;

 private:
  std::vector<vk::Fence> fences_;
  std::vector<vk::SubmitInfo> smt_infos_;
  std::vector<vk::CommandBuffer> cmd_buffers_;
  DISALLOW_COPY_AND_ASSIGN(MultiSubmitBulkWaitForFence);
};
// GenTests generate all the submit tests.
std::vector<vkbench::testBase*> GenTests();

}  // namespace submit
}  // namespace vkbench
#endif  // SRC_TESTS_SUBMIT_H_
