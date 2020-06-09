// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <unistd.h>
#include <limits>
#include <vulkan/vulkan.hpp>

#include "constant.h"
#include "utils.h"
#include "vkBase.h"

extern int g_vlayer;

namespace vkbench {
bool IsLayersSupported(const std::vector<const char*>& kLayers) {
  std::vector<vk::LayerProperties> availLayers =
      vk::enumerateInstanceLayerProperties();
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

bool IsExtensionSupported(const std::vector<const char*>& kExtensions) {
  std::vector<vk::ExtensionProperties> availExtensions =
      vk::enumerateInstanceExtensionProperties();
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
  if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
    DEBUG("%s", kPcallbackData->pMessage)
  } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT ||
             messageSeverity ==
                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    LOG("%s", kPcallbackData->pMessage)
  } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    ERROR("%s", kPcallbackData->pMessage)
  } else {
    ABORT(1, "%s", kPcallbackData->pMessage)
  }
  return VK_FALSE;
}

void vkBase::CreateInstance() {
  const std::vector<const char*> kDebugExtension = {"VK_EXT_debug_utils"};
  vk::ApplicationInfo appInfo("vkbench", 1, "Vulkan.hpp", 1,
                              VK_API_VERSION_1_1);
  vk::InstanceCreateInfo createInfo({}, &appInfo);

  if (g_vlayer) {
    if (!IsLayersSupported(kValidation_layers_) ||
        !IsExtensionSupported(kDebugExtension)) {
      DEBUG("Validation layer is not supported. Less log will be printed.")
      enable_validation_layer_ = false;
    } else {
      createInfo.setEnabledExtensionCount(kDebugExtension.size())
          .setPpEnabledExtensionNames(kDebugExtension.data())
          .setEnabledLayerCount(kValidation_layers_.size())
          .setPpEnabledLayerNames(kValidation_layers_.data());
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
    deviceCreateInfo.setEnabledLayerCount(kValidation_layers_.size());
    deviceCreateInfo.setPpEnabledLayerNames(kValidation_layers_.data());
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
