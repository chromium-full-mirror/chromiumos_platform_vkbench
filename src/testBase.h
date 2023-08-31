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

  // Test related resources allocation. The resources allocated here would be
  // shared by all Run iterations. Time spent here will not be recorded.
  virtual void Initialize() {}
  // Test configuration before running the test. This will run once before
  // each Run loop. Time spent here will be recorded.
  virtual void Setup() {}
  // Test body. Time spent will be recorded.
  virtual void Run() = 0;
  // Test cleanup after looping Run. Time spent here will be recorded.
  virtual void Cleanup() {}
  // Free and Destroy any resources allocated during Initialize. Time
  // spent here will not be recorded.
  virtual void Destroy() = 0;

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

}  // namespace vkbench
#endif  // SRC_TESTBASE_H_
