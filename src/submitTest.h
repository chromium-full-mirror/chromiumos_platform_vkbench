// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VKBENCH_SUBMITTEST_H
#define VKBENCH_SUBMITTEST_H

#include "testBase.h"

namespace vkbench {
class SubmitTest : public testBase {
 public:
  SubmitTest(uint64_t submitCnt){
    sprintf(name_, "SubmitTest@%lu", submitCnt);
    sprintf(desp_, "Times the time used when submitting %lu empty calls.",
            submitCnt);
    smt_infos_.resize(submitCnt);
  }
  const char* Name() const override { return name_; }
  const char* Desp() const override { return desp_; }
  ~SubmitTest() override = default;
  const char* Unit() const override { return "us"; }

 protected:
  void TestInitialization() override;
  bool TestRun() override;
  void TestCleanup() override;
  void TestDestroy() override;

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
