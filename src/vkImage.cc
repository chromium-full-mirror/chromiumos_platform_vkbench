// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/vkImage.h"

namespace vkbench {
vkImage::vkImage(vkBase* vkbase,
                 vk::Extent2D extent,
                 vk::Format format,
                 vk::ImageUsageFlags usage,
                 vkbench::vkMemoryRequirement requirement,
                 vk::ImageTiling tiling) {
  vkbase_ = vkbase;
  extent_ = extent;
  format_ = format;
  tiling_ = tiling;
  mapped_memory_ = nullptr;

  if (format_ != vk::Format::eR8G8B8A8Unorm &&
      format_ != vk::Format::eR32G32B32A32Sfloat) {
    throw not_supported_exception(
        fmt::format("{} has not yet suppported", vk::to_string(format_)));
  }

  vk::ImageCreateInfo img_create_info;
  img_create_info.setFormat(format_)
      .setImageType(vk::ImageType::e2D)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setUsage(usage)
      .setMipLevels(1)
      .setArrayLayers(1)
      .setExtent(vk::Extent3D(extent_, 1))
      .setQueueFamilyIndexCount(1)
      .setPQueueFamilyIndices(&vkbase_->GetGFXQueueFamilyIndex())
      .setInitialLayout(vk::ImageLayout::ePreinitialized)
      .setTiling(tiling_);
  vk::Device device = vkbase_->GetDevice();
  img_ = device.createImage(img_create_info);
  vk::MemoryRequirements mem_req = device.getImageMemoryRequirements(img_);
  memory_ = device.allocateMemory(
      {mem_req.size,
       vkbase_->GetMemoryType(mem_req.memoryTypeBits, requirement)});
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

const vk::Extent2D& vkImage::GetExtent2D() const {
  return extent_;
}

const vk::Format& vkImage::GetFormat() const {
  return format_;
}

vk::Image vkImage::GetImage() const {
  return img_;
}

vk::ImageView vkImage::GetDefaultImageView() {
  if (!img_view_) {
    vk::ImageViewCreateInfo info;
    info.setViewType(vk::ImageViewType::e2D)
        .setFormat(GetFormat())
        .setImage(GetImage());
    vk::ImageSubresourceRange subresource_range;
    subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1);
    info.setSubresourceRange(subresource_range);
    img_view_ = vkbase_->GetDevice().createImageView(info);
  }
  return img_view_;
}

void vkImage::CopyFrom(vk::CommandBuffer cmd, vk::Buffer buffer) {
  vk::BufferImageCopy image_copy;
  // BufferRowLength=0 or BufferImageHeight=0 means buffer tightly packed
  // according to the imageExtent
  image_copy.setBufferOffset(0).setBufferRowLength(0).setBufferImageHeight(0);
  image_copy.setImageOffset(0)
      .setImageExtent(vk::Extent3D(GetExtent2D(), 1))
      .setImageSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
  cmd.copyBufferToImage(buffer, GetImage(),
                        vk::ImageLayout::eTransferDstOptimal, image_copy);
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
    RUNTIME_ERROR("vkGetImageSubresourceLayout needs linear tiling: {}",
                  vk::to_string(tiling_));
  }
  if (format_ != vk::Format::eR8G8B8A8Unorm) {
    RUNTIME_ERROR("Unsupported format with fill_function: {}",
                  vk::to_string(format_));
  }

  vk::SubresourceLayout resource_layout = device.getImageSubresourceLayout(
      GetImage(), {vk::ImageAspectFlagBits::eColor, 0, 0});
  auto data = (unsigned char*)GetMappedMemory();
  auto row_ptr = data + resource_layout.offset;
  for (uint32_t y = 0; y < extent_.height; y++) {
    auto row = row_ptr;
    for (uint32_t x = 0; x < extent_.width; x++) {
      std::array<uint8_t, 4> pixel = fill_function(y, x);
      memcpy(row, pixel.data(), sizeof(uint8_t) * 4);
      row += 4;
    }
    row_ptr += resource_layout.rowPitch;
  }
}

vkImage* vkImage::GetReadableImage(vk::ImageLayout src_layout) {
  std::vector<vk::CommandBuffer> cmds =
      vkbase_->GetDevice().allocateCommandBuffers(
          {vkbase_->GetCommandPool(), vk::CommandBufferLevel::ePrimary, 1});
  DEFER(
      vkbase_->GetDevice().freeCommandBuffers(vkbase_->GetCommandPool(), cmds));

  vk::CommandBuffer command = cmds[0];

  vkImage* dest = new vkbench::vkImage(
      vkbase_, extent_, format_, vk::ImageUsageFlagBits::eTransferDst,
      {vk::MemoryPropertyFlagBits::eHostCoherent |
       vk::MemoryPropertyFlagBits::eHostVisible},
      vk::ImageTiling::eLinear);
  command.begin(vk::CommandBufferBeginInfo());
  this->MoveLayout(command, vk::ImageLayout::eUndefined,
                   vk::ImageLayout::eTransferSrcOptimal);
  dest->MoveLayout(command, vk::ImageLayout::eUndefined,
                   vk::ImageLayout::eTransferDstOptimal);
  // Copy the image from src to dest.
  vk::ImageCopy img_copy_region;
  img_copy_region.srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  img_copy_region.dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  img_copy_region.extent.setWidth(extent_.width)
      .setHeight(extent_.height)
      .setDepth(1);
  command.copyImage(this->GetImage(), vk::ImageLayout::eTransferSrcOptimal,
                    dest->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1,
                    &img_copy_region);
  this->MoveLayout(command, vk::ImageLayout::eUndefined, src_layout);
  dest->MoveLayout(command, vk::ImageLayout::eUndefined,
                   vk::ImageLayout::eGeneral);
  command.end();

  vkbase_->SubmitAndWait(cmds);
  return dest;
}

vkImage::~vkImage() {
  if (img_view_) {
    vkbase_->GetDevice().destroyImageView(img_view_);
  }
  vkbase_->GetDevice().freeMemory(memory_);
  vkbase_->GetDevice().destroyImage(img_);
}

vk::Framebuffer DefaultFramebuffer(vk::Device device,
                                   vkImage* img,
                                   vk::RenderPass render_pass,
                                   vk::ImageView img_view) {
  vk::FramebufferCreateInfo info;
  info.setRenderPass(render_pass)
      .setAttachmentCount(1)
      .setPAttachments(&img_view)
      .setWidth(img->GetExtent2D().width)
      .setHeight(img->GetExtent2D().height)
      .setLayers(1);
  return device.createFramebuffer(info);
}

}  // namespace vkbench
