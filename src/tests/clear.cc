// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./clear.h"

#include <fmt/format.h>

namespace vkbench {
namespace clear {
const char* TEST_PREFIX = "clear";

Clear::Clear(vkBase* base) {
  vk = base;
  clear_color_.setColor({std::array<float, 4>{0.f, 0.f, 1.f, 1.f}});  // Blue
  name_ = fmt::format("{}.Clear", TEST_PREFIX);
  desp_ = "Clears the window to specific color.";
}
void Clear::Initialize() {
  CreateRenderPass();
  CreateFrameBuffers();

  // Create a cmdBuffer with draw call.
  cmd_buffers_ = vk->GetDevice().allocateCommandBuffers(
      {vk->GetCommandPool(), vk::CommandBufferLevel::ePrimary, 1});
  for (const auto& buffer : cmd_buffers_) {
    buffer.begin(vk::CommandBufferBeginInfo());
    vk::RenderPassBeginInfo render_pass_info;
    render_pass_info.setFramebuffer(frame_buffer_)
        .setRenderPass(render_pass_)
        .setClearValueCount(1)
        .setPClearValues(&clear_color_)
        .setRenderArea(vk::Rect2D({}, img_size_));
    buffer.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
    buffer.endRenderPass();
    buffer.end();
  }

  vk::SubmitInfo info;
  info.setCommandBufferCount(1).setPCommandBuffers(cmd_buffers_.data());
  smt_infos_.push_back(info);
}

void Clear::Run() {
  vk->GetGFXQueue().submit(smt_infos_, {});
  vk->GetGFXQueue().waitIdle();
}

void Clear::Destroy() {
  vk::Device device = vk->GetDevice();
  device.destroyImageView(img_view_);
  delete img_;

  device.destroyFramebuffer(frame_buffer_);
  device.destroyRenderPass(render_pass_);
  device.freeCommandBuffers(vk->GetCommandPool(), cmd_buffers_);
  smt_infos_.clear();
}

void Clear::CreateRenderPass() {
  vk::AttachmentDescription att_description;
  att_description.setSamples(vk::SampleCountFlagBits::e1)
      .setFormat(img_format_)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::eGeneral);

  vk::AttachmentReference att_reference;
  att_reference.setAttachment(0).setLayout(
      vk::ImageLayout::eColorAttachmentOptimal);
  vk::SubpassDescription subpass;
  subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
      .setColorAttachmentCount(1)
      .setPColorAttachments(&att_reference);

  vk::RenderPassCreateInfo render_pass_info;
  render_pass_info.setAttachmentCount(1)
      .setPAttachments(&att_description)
      .setSubpassCount(1)
      .setPSubpasses(&subpass);
  render_pass_ = vk->GetDevice().createRenderPass(render_pass_info);
}

void Clear::CreateFrameBuffers() {
  const vk::Device device = vk->GetDevice();

  img_ = vk->AllocateImage(img_size_, img_format_,
                           vk::ImageUsageFlagBits::eColorAttachment |
                               vk::ImageUsageFlagBits::eTransferSrc,
                           vk::MemoryPropertyFlagBits::eDeviceLocal);

  vk::ImageViewCreateInfo img_view_create_info;
  img_view_create_info.setViewType(vk::ImageViewType::e2D)
      .setFormat(img_format_)
      .setImage(img_->GetImage());
  img_view_create_info.subresourceRange
      .setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setBaseMipLevel(0)
      .setLevelCount(1)
      .setBaseArrayLayer(0)
      .setLayerCount(1);
  img_view_ = device.createImageView(img_view_create_info);

  vk::FramebufferCreateInfo frame_buffer_info;
  frame_buffer_info.setRenderPass(render_pass_)
      .setAttachmentCount(1)
      .setPAttachments(&img_view_)
      .setWidth(img_size_.width)
      .setHeight(img_size_.height)
      .setLayers(1);
  frame_buffer_ = device.createFramebuffer(frame_buffer_info);
}

Image Clear::GetImage() const {
  const vk::Device device = vk->GetDevice();
  vkImage* dest = vk->GetReadableImage(img_, img_size_, img_format_);
  DEFER(delete dest);
  vk::SubresourceLayout sub_resource_layout = device.getImageSubresourceLayout(
      dest->GetImage(), {vk::ImageAspectFlagBits::eColor, 0, 0});
  const unsigned char* data = (const unsigned char*)device.mapMemory(
      dest->GetMemory(), 0, VK_WHOLE_SIZE);
  return Image(data, img_size_, sub_resource_layout);
}

// GenTests generate all the clear tests.
std::vector<vkbench::testBase*> GenTests() {
  return std::vector<testBase*>{new Clear(vkBase::GetInstance())};
}
}  // namespace clear
}  // namespace vkbench
