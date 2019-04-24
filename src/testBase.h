// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __VKBENCH_TESTBASE_H__
#define __VKBENCH_TESTBASE_H__

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "utils.h"

namespace vkbench {
class testBase {
 public:
  virtual ~testBase() = default;
  // Name of test.
  virtual const char* Name() const = 0;
  // Description of the test.
  virtual const char* Desp() const = 0;
  // Unit for formatted measurement.
  virtual const char* Unit() const = 0;
  // Given time elapse per iteration, format it into meaningful numbers.
  virtual double FormatMeasurement(double time) { return time; }

  // Vulkan initialization for allocating universal resources. Must be called
  // before TestInitialization.
  virtual void VkInitialization();
  // Test related resources allocation. The resources allocated here would be
  // shared by all TestRun iterations. Time spent here will not be recorded.
  virtual void TestInitialization(){}
  // Test configuration before running the test. This will run once before
  // each TestRun loop. Time spent here will be recorded.
  virtual void TestSetup(){}
  // Test body. Time spent will be recorded.
  virtual bool TestRun() = 0;
  // Test cleanup after looping TestRun. Time spent here will be recorded.
  virtual void TestCleanup() = 0;
  // Free and Destroy any resources allocated during TestInitialization. Time
  // spent here will not be recorded.
  virtual void TestDestroy() = 0;
  // Free and Destroy any general vulkan resources
  virtual void VkDestroy();

 protected:
  // Vulkan general method.
  virtual bool CreateInstance();
  virtual bool ChoosePhysical_device_();
  virtual bool CreateLogicalDevice();
  virtual bool CreateCommandPool();

  // Vulkan handles
  const std::vector<const char*> kValidation_layers_ = {
      "VK_LAYER_LUNARG_standard_validation", "VK_LAYER_GOOGLE_threading"};
  bool enable_validation_layer_ = false;

  vk::DebugUtilsMessengerEXT debug_messenger_;
  vk::Instance instance_;
  vk::PhysicalDevice physical_device_;
  vk::Device device_;
  vk::Queue gfx_queue_;
  vk::CommandPool cmd_pool_;
  uint32_t gfx_queue_idx_ = -1;
};

}  // namespace vkbench
#endif
