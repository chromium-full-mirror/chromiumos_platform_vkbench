// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __CONSTNAT_H__
#define __CONSTANT_H__

// ERROR_TYPE indicates various error type for vkbench itself.
enum ERROR_TYPE {
  TEST_PASS,
  TEST_FAIL,
  TEST_NA,
  ENVIROMENT_FAIL,
  VULKAN_INITIALIZATION_FAIL,
  TEST_INITIALIZATION_FAIL
};

#endif
