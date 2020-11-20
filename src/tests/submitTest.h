// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VKBENCH_SUBMITTEST_H
#define VKBENCH_SUBMITTEST_H

#include "../testBase.h"

namespace vkbench {
class SubmitTest : public testBase {
 public:
  SubmitTest(uint64_t submitCnt, vkBase* base) {
    vk = base;
    sprintf(name_, "SubmitTest@%lu", submitCnt);
    sprintf(desp_, "Times the time used when submitting %lu empty calls.",
            submitCnt);
    smt_infos_.resize(submitCnt);
  }
  ~SubmitTest() override = default;

  static std::vector<testBase*> GenTests() {
    return std::vector<testBase*>{
        new vkbench::SubmitTest(16, vkBase::GetInstance()),
        new vkbench::SubmitTest(64, vkBase::GetInstance()),
        new vkbench::SubmitTest(256, vkBase::GetInstance()),
        new vkbench::SubmitTest(1024, vkBase::GetInstance()),
        new vkbench::SubmitTest(4096, vkBase::GetInstance()),
        new vkbench::SubmitTest(16384, vkBase::GetInstance()),
    };
  }
  const char* Name() const override { return name_; }
  const char* Desp() const override { return desp_; }
  const char* Unit() const override { return "us"; }

 protected:
  void Initialize() override;
  void Run() override;
  void Cleanup() override;
  void Destroy() override;

 private:
  char name_[100];
  char desp_[1024];
  std::vector<vk::Fence> fences_;
  std::vector<vk::SubmitInfo> smt_infos_;
  std::vector<vk::CommandBuffer> cmd_buffers_;
  DISALLOW_COPY_AND_ASSIGN(SubmitTest);
};
}  // namespace vkbench
#endif  // VKBENCH_SUBMITTEST_H
