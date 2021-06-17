// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_TESTBASE_H_
#define SRC_TESTBASE_H_

#include <vulkan/vulkan.hpp>

#include <vector>

#include "src/filepath.h"
#include "src/utils.h"
#include "src/vkBase.h"

namespace vkbench {
class testBase {
 public:
  virtual ~testBase() {}
  // Name of test.
  virtual const char* Name() const = 0;
  // Description of the test.
  virtual const char* Desp() const = 0;
  // Unit for formatted measurement.
  virtual const char* Unit() const = 0;
  // Given time elapse per iteration, format it into meaningful numbers.
  virtual double FormatMeasurement(double time) { return time; }

  // Image returns the rendered image.
  virtual Image GetImage() const { return vkbench::Image(); }

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
};

}  // namespace vkbench
#endif  // SRC_TESTBASE_H_
