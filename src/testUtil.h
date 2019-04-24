// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __VKBENCH_TESTUTIL_H__
#define __VKBENCH_TESTUTIL_H__
#include <string>
#include <vulkan/vulkan.hpp>

namespace vkbench {
bool IsLayersSupported(const std::vector<const char*>& kLayers);
bool IsExtensionSupported(const std::vector<const char*>& kExtensions);
uint32_t ChooseGFXQueueFamilies(const vk::PhysicalDevice& kPhysical_device);
vk::Result CreateDebugUtilsMessengerEXT(
    vk::Instance instance,
    const vk::DebugUtilsMessengerCreateInfoEXT* kPcreateInfo,
    const vk::AllocationCallbacks* kPallocator,
    vk::DebugUtilsMessengerEXT* pdebug_messengeer);
void DestroyDebugUtilsMessengerEXT(vk::Instance instance,
                                   vk::DebugUtilsMessengerEXT debug_messengeer,
                                   const vk::AllocationCallbacks* kPAllocator);
}  // namespace vkbench
#endif
