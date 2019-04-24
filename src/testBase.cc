// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testBase.h"
#include <unistd.h>
#include <limits>
#include <vulkan/vulkan.hpp>
#include "utils.h"
#include "testUtil.h"

extern int g_vlayer;

namespace vkbench {
void testBase::VkInitialization() {
  CreateInstance();
  ChoosePhysical_device_();
  CreateLogicalDevice();
  CreateCommandPool();
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

bool testBase::CreateInstance() {
  const std::vector<const char*> kDebugExtension = {"VK_EXT_debug_utils"};
  vk::ApplicationInfo
      appInfo("vkbench", 1, "Vulkan.hpp", 1, VK_API_VERSION_1_1);
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
        .setMessageSeverity(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
        .setPfnUserCallback(ValidationCallback);
    CreateDebugUtilsMessengerEXT(instance_,
                                 &debugCreateInfo,
                                 nullptr,
                                 &debug_messenger_);
  }
  return true;
}

bool testBase::ChoosePhysical_device_() {
  if (!instance_)
    return false;
  std::vector<vk::PhysicalDevice>
      physical_devices = instance_.enumeratePhysicalDevices();
  if (physical_devices.size() == 0) {
    ERROR("No available Vk physical_device_.")
    return false;
  }
  // Choose the first device we found.
  physical_device_ = physical_devices[0];
  gfx_queue_idx_ = ChooseGFXQueueFamilies(physical_device_);
  return true;
}

bool testBase::CreateLogicalDevice() {
  float tmp = 1.0f;
  vk::DeviceQueueCreateInfo queueCreateInfo({}, gfx_queue_idx_, 1, &tmp);
  vk::DeviceCreateInfo deviceCreateInfo({}, 1, &queueCreateInfo);
  if (enable_validation_layer_) {
    deviceCreateInfo.setEnabledLayerCount(kValidation_layers_.size());
    deviceCreateInfo.setPpEnabledLayerNames(kValidation_layers_.data());
  }
  device_ = physical_device_.createDevice(deviceCreateInfo);
  gfx_queue_ = device_.getQueue(gfx_queue_idx_, /* Queue Index */ 0);
  return true;
}

bool testBase::CreateCommandPool() {
  cmd_pool_ =
      device_.createCommandPool({vk::CommandPoolCreateFlags(), gfx_queue_idx_});
  return true;
}

void testBase::VkDestroy() {
  if (!instance_) {
    ERROR("Can't destroy any resource without a valid vulkan instance.")
    return;
  }
  if (device_) {
    if (cmd_pool_) {
      device_.destroy(cmd_pool_, nullptr);
    }
    device_.destroy();
  }
  if (enable_validation_layer_) {
    DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
  }
  instance_.destroy();
}

}  // namespace vkbench