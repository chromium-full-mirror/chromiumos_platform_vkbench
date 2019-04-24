// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>
#include <vulkan/vulkan.hpp>
#include "testUtil.h"
#include "utils.h"

bool vkbench::IsLayersSupported(const std::vector<const char*>& kLayers) {
  std::vector<vk::LayerProperties>
      availLayers = vk::enumerateInstanceLayerProperties();
  for (const auto& layer : kLayers) {
    bool found = false;
    for (const auto availLayer : availLayers) {
      if (!strcmp(availLayer.layerName, layer)) {
        DEBUG("%s Layer is support.", layer)
        found = true;
        break;
      }
    }
    if (!found) {
      DEBUG("Layer %s is not support.", layer)
      return false;
    }
  }
  return true;
}

bool vkbench::IsExtensionSupported(
    const std::vector<const char*>& kExtensions) {
  std::vector<vk::ExtensionProperties>
      availExtensions = vk::enumerateInstanceExtensionProperties();
  for (const auto extension : kExtensions) {
    bool found = false;
    for (const auto availExtension : availExtensions) {
      if (!strcmp(availExtension.extensionName, extension)) {
        found = true;
        break;
      }
    }
    if (!found) {
      DEBUG("Extension %s is not supported.", extension)
      return false;
    }
  }
  return true;
}

vk::Result vkbench::CreateDebugUtilsMessengerEXT(
    vk::Instance instance,
    const vk::DebugUtilsMessengerCreateInfoEXT* kPcreateInfo,
    const vk::AllocationCallbacks* kPallocator,
    vk::DebugUtilsMessengerEXT* pdebug_messengeer) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) instance.getProcAddr(
      "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return static_cast<vk::Result>(func(
        instance,
        reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(
            kPcreateInfo),
        reinterpret_cast<const VkAllocationCallbacks*>(kPallocator),
        reinterpret_cast<VkDebugUtilsMessengerEXT*>(pdebug_messengeer)));
  } else {
    return vk::Result::eErrorExtensionNotPresent;
  }
}

void vkbench::DestroyDebugUtilsMessengerEXT(
    vk::Instance instance,
    vk::DebugUtilsMessengerEXT debug_messengeer,
    const vk::AllocationCallbacks* kPAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) instance.getProcAddr(
      "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance,
         static_cast<VkDebugUtilsMessengerEXT>(debug_messengeer),
         reinterpret_cast<const VkAllocationCallbacks*>(
             static_cast<const vk::AllocationCallbacks*>(kPAllocator)));
  }
}

uint32_t vkbench::ChooseGFXQueueFamilies(
    const vk::PhysicalDevice& kPhysical_device) {
  uint32_t gfxQueueIdx = UINT32_MAX;
  std::vector<vk::QueueFamilyProperties>
      props = kPhysical_device.getQueueFamilyProperties();
  for (uint32_t i = 0; i < props.size(); i++) {
    if (props[i].queueCount <= 0)
      continue;
    if (props[i].queueFlags & vk::QueueFlagBits::eGraphics) {
      gfxQueueIdx = i;
      break;
    }
  }
  return gfxQueueIdx;
}