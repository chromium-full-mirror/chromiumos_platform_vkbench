// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vulkan/vulkan.hpp>

#include <unistd.h>
#include <algorithm>
#include <limits>

#include "./utils.h"
#include "./vkBase.h"

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

vkImage::vkImage(vkBase* vkbase,
                 vk::Extent2D img_size,
                 vk::Format img_format,
                 vk::ImageUsageFlags usage,
                 vk::MemoryPropertyFlags memory_properties,
                 vk::ImageTiling tiling) {
  vkbase_ = vkbase;
  size_ = img_size;
  format_ = img_format;
  tiling_ = tiling;
  mapped_memory_ = nullptr;

  vk::ImageCreateInfo img_create_info;
  img_create_info.setFormat(format_)
      .setImageType(vk::ImageType::e2D)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setUsage(usage)
      .setMipLevels(1)
      .setArrayLayers(1)
      .setExtent(vk::Extent3D(size_, 1))
      .setQueueFamilyIndexCount(1)
      .setPQueueFamilyIndices(&vkbase_->GetGFXQueueFamilyIndex())
      .setInitialLayout(vk::ImageLayout::ePreinitialized)
      .setTiling(tiling_);
  vk::Device device = vkbase_->GetDevice();
  img_ = device.createImage(img_create_info);
  vk::MemoryRequirements mem_req = device.getImageMemoryRequirements(img_);
  memory_ = device.allocateMemory(
      {mem_req.size,
       vkbase_->GetMemoryType(mem_req.memoryTypeBits, memory_properties)});
  device.bindImageMemory(img_, memory_, 0);
}

void* vkImage::GetMappedMemory() {
  if (mapped_memory_ != nullptr) {
    return mapped_memory_;
  }
  mapped_memory_ = vkbase_->GetDevice().mapMemory(memory_, 0, VK_WHOLE_SIZE);
  return mapped_memory_;
}

void vkImage::UnmapMemory() {
  if (mapped_memory_ == nullptr) {
    return;
  }

  vkbase_->GetDevice().unmapMemory(memory_);
  mapped_memory_ = nullptr;
}

void vkImage::MoveLayout(vk::CommandBuffer cmd,
                         vk::ImageLayout src,
                         vk::ImageLayout dst) {
  vk::ImageMemoryBarrier img_memory_barrier;
  // Transition image layout to target layout.
  img_memory_barrier.setOldLayout(src)
      .setNewLayout(dst)
      .setImage(GetImage())
      .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                      vk::PipelineStageFlagBits::eTransfer, {}, 0, nullptr, 0,
                      nullptr, 1, &img_memory_barrier);
}

void vkImage::FillImage(std::array<uint8_t, 4> (*fill_function)(int x, int y)) {
  vk::Device device = vkbase_->GetDevice();
  if (tiling_ != vk::ImageTiling::eLinear) {
    RUNTIME_ERROR("vkGetImageSubresourceLayout needs linear tiling: %s",
                  vk::to_string(tiling_).c_str());
  }
  if (format_ != vk::Format::eR8G8B8A8Unorm) {
    RUNTIME_ERROR("Unsupported format with fill_function: %s",
                  vk::to_string(format_).c_str());
  }

  vk::SubresourceLayout resource_layout = device.getImageSubresourceLayout(
      GetImage(), {vk::ImageAspectFlagBits::eColor, 0, 0});
  auto data = (unsigned char*)GetMappedMemory();
  auto row_ptr = data + resource_layout.offset;
  for (uint32_t y = 0; y < size_.height; y++) {
    auto row = row_ptr;
    for (uint32_t x = 0; x < size_.width; x++) {
      std::array<uint8_t, 4> pixel = fill_function(y, x);
      memcpy(row, pixel.data(), sizeof(char) * 4);
      row += 4;
    }
    row_ptr += resource_layout.rowPitch;
  }
}

vkImage::~vkImage() {
  vkbase_->GetDevice().freeMemory(memory_);
  vkbase_->GetDevice().destroyImage(img_);
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
    RUNTIME_ERROR("vkCreateDebugUtilsMessengerEXT failed: %s",
                  vk::to_string(result).c_str());
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
      RUNTIME_ERROR("%s", kPcallbackData->pMessage);
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
  cmd_pool_ =
      device_.createCommandPool({vk::CommandPoolCreateFlags(), gfx_queue_idx_});
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

// GetReadableImage copys a image created with eTransferSrc and returns a
// readable vkImage in vk::ImageLayout::eGeneral layout.
vkImage* vkBase::GetReadableImage(vkImage* src,
                                  vk::ImageLayout src_layout,
                                  vk::Extent2D size,
                                  vk::Format format) {
  std::vector<vk::CommandBuffer> cmds = device_.allocateCommandBuffers(
      {cmd_pool_, vk::CommandBufferLevel::ePrimary, 1});
  DEFER(device_.freeCommandBuffers(cmd_pool_, cmds));

  vk::CommandBuffer command = cmds[0];

  vkImage* dest =
      new vkImage(this, size, format, vk::ImageUsageFlagBits::eTransferDst,
                  vk::MemoryPropertyFlagBits::eHostCoherent |
                      vk::MemoryPropertyFlagBits::eHostVisible,
                  vk::ImageTiling::eLinear);
  command.begin(vk::CommandBufferBeginInfo());
  src->MoveLayout(command, src_layout, vk::ImageLayout::eTransferSrcOptimal);
  dest->MoveLayout(command, vk::ImageLayout::eUndefined,
                   vk::ImageLayout::eTransferDstOptimal);
  // Copy the image from src to dest.
  vk::ImageCopy img_copy_region;
  img_copy_region.srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  img_copy_region.dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  img_copy_region.extent.setWidth(size.width)
      .setHeight(size.height)
      .setDepth(1);
  command.copyImage(src->GetImage(), vk::ImageLayout::eTransferSrcOptimal,
                    dest->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1,
                    &img_copy_region);
  src->MoveLayout(command, vk::ImageLayout::eTransferSrcOptimal, src_layout);
  dest->MoveLayout(command, vk::ImageLayout::eTransferDstOptimal,
                   vk::ImageLayout::eGeneral);
  command.end();

  std::vector<vk::SubmitInfo> submit_infos(1);
  submit_infos[0].setCommandBufferCount(1).setPCommandBuffers(cmds.data());
  gfx_queue_.submit(submit_infos, {});
  gfx_queue_.waitIdle();
  return dest;
}

// Get the index of a memory type that has all the requested property bits set
uint32_t vkBase::GetMemoryType(uint32_t bits,
                               vk::MemoryPropertyFlags properties) {
  for (uint32_t i = 0; i < mem_properties_.memoryTypeCount; i++) {
    if ((bits & 1) == 1) {
      if (!properties)
        return i;
      if ((mem_properties_.memoryTypes[i].propertyFlags & properties) ==
          properties) {
        return i;
      }
    }
    bits >>= 1;
  }
  RUNTIME_ERROR("could not find a matching memory type");
  return 0;
}

}  // namespace vkbench
