// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_TESTBASE_H_
#define SRC_TESTBASE_H_

#include <vulkan/vulkan.hpp>

#include <string>
#include <vector>

#include "src/filepath.h"
#include "src/utils.h"
#include "src/vkBase.h"

namespace vkbench {
// testBase are the base class.
// Test are executed and measured in the following order
// - Initialize(), time NOT recorded
// ------------------- i-th iteration ----------------------
// - Setup(i), time NOT recorded
// - for i times
//     - Run(), time IS recorded
// - Cleanup(), time NOT recorded
// ---------------------------------------------------------
// - Destroy(), time NOT recorded
class testBase {
 public:
  virtual ~testBase() {}
  // Name of test.
  const std::string Name() const { return name_; }
  // Description of the test.
  const std::string Desp() const { return desp_; }
  // Unit for formatted measurement.
  virtual const char* Unit() const = 0;
  // Given microsecond time elapsed, format it into meaningful numbers.
  virtual double FormatMeasurement(double time) { return time; }

  // Image returns the rendered image.
  virtual Image GetImage() const {
    NOT_SUPPORT("Test doesn't implement GetImage method.");
    __builtin_unreachable();
  }

  // Test related resources allocation.
  virtual void Initialize() {}
  // Test configuration before running the test.
  virtual void Setup(int iteration) {}
  // Test body.
  virtual void Run() = 0;
  // Test cleanup after looping Run.
  virtual void Cleanup() {}
  // Free and Destroy any resources allocated during Initialize.
  virtual void Destroy() = 0;

  virtual void RunFunc(int iteration) {
    for (auto i = 0; i < iteration; i++) {
      Run();
    }
  }

  vkbench::vkBase* vk;

 protected:
  std::string name_;
  std::string desp_;
};

// commandTestBase submit a pre-recorded command buffers in its Run
// method.
class commandTestBase : public testBase {
 public:
  void Run() override { vk->SubmitAndWait(cmds_); }

  void Destroy() override {
    vk->GetDevice().freeCommandBuffers(vk->GetCommandPool(), cmds_);
  }

  // GetCommandBuffer returns a command buffer reference for submit later in Run
  // Stage.
  vk::CommandBuffer& GetCommandBuffer() {
    if (cmds_.size() == 0) {
      cmds_ = vk->GetDevice().allocateCommandBuffers(
          {vk->GetCommandPool(), vk::CommandBufferLevel::ePrimary, 1});
    }
    return cmds_[0];
  }

 private:
  std::vector<vk::CommandBuffer> cmds_;
};

// singleCommandTestBase submit a pre-recorded command buffers in its Run
// method.
// The main difference is that its Run method are only executed once in
// each iteration. Tests should record its command buffer in Setup phase by
// looping it i-th time.
//
// Test are executed and measured in the following order
// - Initialize, time NOT recorded
// ------------------- i-th iteration ----------------------
// - Setup(i), time NOT recorded
// - Run(), time IS recorded
// - Cleanup(), time NOT recorded
// ---------------------------------------------------------
// - Destroy(), time NOT recorded
class singleCommandTestBase : public commandTestBase {
 public:
  void RunFunc(int i) override { Run(); }
};

}  // namespace vkbench
#endif  // SRC_TESTBASE_H_
