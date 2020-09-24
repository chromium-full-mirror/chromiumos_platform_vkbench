// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <unistd.h>
#include <algorithm>
#include <limits>
#include <vulkan/vulkan.hpp>

#include "constant.h"
#include "utils.h"
#include "vkBase.h"

extern int g_vlayer;

namespace vkbench {
bool IsLayerSupported(const char* layer) {
  std::vector<vk::LayerProperties> availLayers =
      vk::enumerateInstanceLayerProperties();
  for (const auto& availLayer : availLayers) {
    if (!strcmp(layer, availLayer.layerName)) {
      return true;
    }
  }
  LOG("Layer %s is not support.", layer);
  return false;
}

bool IsExtensionSupported(const char* ext) {
  std::vector<vk::ExtensionProperties> availExtensions =
      vk::enumerateInstanceExtensionProperties();
  for (const auto& availExtension : availExtensions) {
    if (!strcmp(ext, availExtension.extensionName)) {
      return true;
    }
  }
  DEBUG("Extension %s is not supported.", ext);
  return false;
}

vk::Result CreateDebugUtilsMessengerEXT(
    vk::Instance instance,
    const vk::DebugUtilsMessengerCreateInfoEXT* kPcreateInfo,
    const vk::AllocationCallbacks* kPallocator,
    vk::DebugUtilsMessengerEXT* pdebug_messengeer) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)instance.getProcAddr(
      "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return static_cast<vk::Result>(
        func((VkInstance)instance,
             reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(
                 kPcreateInfo),
             reinterpret_cast<const VkAllocationCallbacks*>(kPallocator),
             reinterpret_cast<VkDebugUtilsMessengerEXT*>(pdebug_messengeer)));
  } else {
    return vk::Result::eErrorExtensionNotPresent;
  }
}

void DestroyDebugUtilsMessengerEXT(vk::Instance instance,
                                   vk::DebugUtilsMessengerEXT debug_messengeer,
                                   const vk::AllocationCallbacks* kPAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)instance.getProcAddr(
      "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func((VkInstance)instance,
         static_cast<VkDebugUtilsMessengerEXT>(debug_messengeer),
         reinterpret_cast<const VkAllocationCallbacks*>(
             static_cast<const vk::AllocationCallbacks*>(kPAllocator)));
  }
}

uint32_t ChooseGFXQueueFamilies(const vk::PhysicalDevice& kPhysical_device) {
  uint32_t gfxQueueIdx = UINT32_MAX;
  std::vector<vk::QueueFamilyProperties> props =
      kPhysical_device.getQueueFamilyProperties();
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

void vkBase::Initialize() {
  if (initialized)
    return;
  CreateInstance();
  ChoosePhysical_device_();
  CreateLogicalDevice();
  CreateCommandPool();
  initialized = true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
ValidationCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                   VkDebugUtilsMessageTypeFlagsEXT messageType,
                   const VkDebugUtilsMessengerCallbackDataEXT* kPcallbackData,
                   void* pUserData) {
  UNUSED(messageType);
  UNUSED(pUserData);
  switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      DEBUG("%s", kPcallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      LOG("%s", kPcallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      ERROR("%s", kPcallbackData->pMessage);
      break;
    default:
      ABORT(1, "%s", kPcallbackData->pMessage);
  }
  return VK_FALSE;
}

void vkBase::CreateInstance() {
  vk::ApplicationInfo appInfo("vkbench", 1, "Vulkan.hpp", 1,
                              VK_API_VERSION_1_1);
  vk::InstanceCreateInfo createInfo({}, &appInfo);

  if (g_vlayer) {
    validation_layers_.erase(
        std::remove_if(
            validation_layers_.begin(), validation_layers_.end(),
            [](const char* layer) { return !IsLayerSupported(layer); }),
        validation_layers_.end());
    debug_extension_.erase(
        std::remove_if(
            debug_extension_.begin(), debug_extension_.end(),
            [](const char* ext) { return !IsExtensionSupported(ext); }),
        debug_extension_.end());
    if (validation_layers_.size() == 0 || debug_extension_.size() == 0) {
      LOG("Validation layer is not supported. Less log will be printed.")
      enable_validation_layer_ = false;
    } else {
      createInfo.setEnabledExtensionCount(debug_extension_.size())
          .setPpEnabledExtensionNames(debug_extension_.data())
          .setEnabledLayerCount(validation_layers_.size())
          .setPpEnabledLayerNames(validation_layers_.data());
      enable_validation_layer_ = true;
    }
  }
  instance_ = vk::createInstance(createInfo);
  if (enable_validation_layer_) {
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    debugCreateInfo
        .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
        .setPfnUserCallback(ValidationCallback);
    CreateDebugUtilsMessengerEXT(instance_, &debugCreateInfo, nullptr,
                                 &debug_messenger_);
  }
}

void vkBase::ChoosePhysical_device_() {
  std::vector<vk::PhysicalDevice> physical_devices =
      instance_.enumeratePhysicalDevices();
  if (physical_devices.size() == 0) {
    ERROR("enumeratePhysicalDevices returns 0 devices.")
    throw VULKAN_INITIALIZATION_FAIL;
  }
  // Choose the first device we found.
  physical_device_ = physical_devices[0];
  gfx_queue_idx_ = ChooseGFXQueueFamilies(physical_device_);
}

void vkBase::CreateLogicalDevice() {
  float tmp = 1.0f;
  vk::DeviceQueueCreateInfo queueCreateInfo({}, gfx_queue_idx_, 1, &tmp);
  vk::DeviceCreateInfo deviceCreateInfo({}, 1, &queueCreateInfo);
  if (enable_validation_layer_) {
    deviceCreateInfo.setEnabledLayerCount(validation_layers_.size());
    deviceCreateInfo.setPpEnabledLayerNames(validation_layers_.data());
  }
  device_ = physical_device_.createDevice(deviceCreateInfo);
  gfx_queue_ = device_.getQueue(gfx_queue_idx_, /* Queue Index */ 0);
}

void vkBase::CreateCommandPool() {
  cmd_pool_ =
      device_.createCommandPool({vk::CommandPoolCreateFlags(), gfx_queue_idx_});
}

void vkBase::Destroy() {
  if (!initialized) {
    ERROR("Can't destroy any resource without initialization.")
    return;
  }
  if (cmd_pool_)
    device_.destroy(cmd_pool_, nullptr);
  device_.destroy();
  if (enable_validation_layer_) {
    DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
  }
  instance_.destroy();
  initialized = false;
}

}  // namespace vkbench
