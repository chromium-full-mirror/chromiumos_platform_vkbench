// Copyright 2020 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <unistd.h>
#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <limits>
#include <map>
#include <string>

#include "src/utils.h"
#include "src/vkBase.h"

extern int g_vlayer;

namespace vkbench {
std::string vkMemoryRequirement::String() {
  return fmt::format(
      "include: {}, exclude: {}, prefer_include: {}, prefer_exclude: {}",
      vk::to_string(include), vk::to_string(exclude),
      vk::to_string(prefer_include), vk::to_string(prefer_exclude));
}

bool IsLayerSupported(const char* layer) {
  std::vector<vk::LayerProperties> availLayers =
      vk::enumerateInstanceLayerProperties();
  for (const auto& availLayer : availLayers) {
    if (!strcmp(layer, availLayer.layerName)) {
      return true;
    }
  }
  LOG("Layer {} is not support.", layer);
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
  DEBUG("Extension {} is not supported.", ext);
  return false;
}

void CreateDebugUtilsMessengerEXT(
    vk::Instance instance,
    const vk::DebugUtilsMessengerCreateInfoEXT* kPcreateInfo,
    const vk::AllocationCallbacks* kPallocator,
    vk::DebugUtilsMessengerEXT* pdebug_messengeer) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)instance.getProcAddr(
      "vkCreateDebugUtilsMessengerEXT");
  if (func == nullptr)
    RUNTIME_ERROR("can't locate vkCreateDebugUtilsMessengerEXT");
  vk::Result result = static_cast<vk::Result>(func(
      (VkInstance)instance,
      reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(kPcreateInfo),
      reinterpret_cast<const VkAllocationCallbacks*>(kPallocator),
      reinterpret_cast<VkDebugUtilsMessengerEXT*>(pdebug_messengeer)));
  if (result != vk::Result::eSuccess) {
    RUNTIME_ERROR("vkCreateDebugUtilsMessengerEXT failed: {}",
                  vk::to_string(result));
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

vkBase* vkBase::singleton_ = nullptr;
vkBase* vkBase::GetInstance() {
  if (singleton_ == nullptr)
    singleton_ = new vkBase();
  return singleton_;
}

uint32_t ChooseGFXQueueFamilies(const vk::PhysicalDevice& physical_device) {
  uint32_t gfx_queue_idx = UINT32_MAX;
  std::vector<vk::QueueFamilyProperties> props =
      physical_device.getQueueFamilyProperties();
  for (uint32_t i = 0; i < props.size(); i++) {
    if (props[i].queueCount <= 0)
      continue;
    if (props[i].queueFlags & vk::QueueFlagBits::eGraphics) {
      gfx_queue_idx = i;
      break;
    }
  }
  return gfx_queue_idx;
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
      DEBUG("{}", kPcallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      LOG("{}", kPcallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      ERROR("{}", kPcallbackData->pMessage);
      break;
    default:
      RUNTIME_ERROR("{}", kPcallbackData->pMessage);
  }
  return VK_FALSE;
}

void vkBase::Initialize() {
  CreateInstance();
  ChoosePhysicalDevice();
  CreateLogicalDevice();
  CreateCommandPool();
  initialized_ = true;
}

bool vkBase::IsInitialized() const {
  return initialized_;
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
      LOG("Validation layer is not supported. Less log will be printed.");
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

void vkBase::ChoosePhysicalDevice() {
  std::vector<vk::PhysicalDevice> physical_devices =
      instance_.enumeratePhysicalDevices();
  if (physical_devices.size() == 0) {
    RUNTIME_ERROR("enumeratePhysicalDevices returns 0 devices.");
  }

  auto isSuitable = [&physical_devices](vk::MemoryPropertyFlags flags) -> int {
    int target = -1;
    for (auto index = 0; index < physical_devices.size(); index++) {
      vk::PhysicalDeviceMemoryProperties mem_properties =
          physical_devices[index].getMemoryProperties();

      for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((mem_properties.memoryTypes[i].propertyFlags & flags) == flags) {
          target = index;
        }
      }
    }
    return target;
  };
  int index = isSuitable(vk::MemoryPropertyFlagBits::eDeviceLocal |
                         vk::MemoryPropertyFlagBits::eHostVisible |
                         vk::MemoryPropertyFlagBits::eHostCoherent);
  if (index == -1) {
    index = isSuitable(vk::MemoryPropertyFlagBits::eDeviceLocal);
  }
  physical_device_ = physical_devices[index];
  gfx_queue_idx_ = ChooseGFXQueueFamilies(physical_device_);
  mem_properties_ = physical_device_.getMemoryProperties();
}

void vkBase::CreateLogicalDevice() {
  float tmp = 1.0f;
  vk::DeviceQueueCreateInfo queue_create_info;
  queue_create_info.setQueueCount(1)
      .setQueueFamilyIndex(gfx_queue_idx_)
      .setPQueuePriorities(&tmp);
  vk::DeviceCreateInfo device_create_info;
  device_create_info.setQueueCreateInfoCount(1).setPQueueCreateInfos(
      &queue_create_info);
  if (enable_validation_layer_) {
    device_create_info.setEnabledLayerCount(validation_layers_.size())
        .setPpEnabledLayerNames(validation_layers_.data());
  }
  device_ = physical_device_.createDevice(device_create_info);
  gfx_queue_ = device_.getQueue(gfx_queue_idx_, /* Queue Index */ 0);
}

void vkBase::CreateCommandPool() {
  cmd_pool_ = device_.createCommandPool(
      {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, gfx_queue_idx_});
}

void vkBase::Destroy() {
  if (cmd_pool_)
    device_.destroy(cmd_pool_, nullptr);
  device_.destroy();
  if (enable_validation_layer_) {
    DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
  }
  instance_.destroy();
  initialized_ = false;
}

void vkBase::SubmitAndWait(const vk::ArrayProxy<vk::CommandBuffer> cmds) {
  Submit(cmds);
  gfx_queue_.waitIdle();
}

void vkBase::Submit(const vk::ArrayProxy<vk::CommandBuffer> cmds) {
  vk::SubmitInfo info;
  info.setCommandBufferCount(cmds.size()).setPCommandBuffers(cmds.data());
  gfx_queue_.submit(info, {});
}

uint32_t vkBase::GetMemoryType(uint32_t bits,
                               vkbench::vkMemoryRequirement requirement) {
  int maxScore, candidate;
  bool found = false;
  for (auto i = 0; i < mem_properties_.memoryTypeCount; i++) {
    if (!(bits & (1 << i))) {
      continue;
    }
    vk::MemoryPropertyFlags device_flags =
        mem_properties_.memoryTypes[i].propertyFlags;
    if ((device_flags & requirement.include) != requirement.include) {
      continue;
    }
    if ((device_flags & requirement.exclude) != vk::MemoryPropertyFlagBits{}) {
      continue;
    }
    // Add 1 for every prefer_include matches.
    int score = __builtin_popcount(
        static_cast<uint32_t>(device_flags & requirement.prefer_include));
    // Substract 1 for every prefer_exclude matches.
    score -= __builtin_popcount(
        static_cast<uint32_t>(device_flags & requirement.prefer_exclude));

    if (found == false || score > maxScore) {
      found = true;
      maxScore = score;
      candidate = i;
    }
  }

  if (found == false) {
    NOT_SUPPORT("No matching memory type found for {}", requirement.String());
  }
  return candidate;
}

}  // namespace vkbench
