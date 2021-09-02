// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./clear.h"

#include <fmt/format.h>

namespace vkbench {
namespace clear {
const char* TEST_PREFIX = "clear";

const vk::ClearColorValue GREY = std::array<float, 4>{0.1, 0.12, 0.13, 1.0};
const vk::ClearColorValue WHITE = std::array<float, 4>{1.0, 1.0, 1.0, 1.0};
const vk::ClearColorValue BLACK = std::array<float, 4>{0.0, 0.0, 0.0, 1.0};

// The number of clear inside a single submit.
const uint32_t CLEAR_CNT = 1024;

LoadOp::LoadOp(vkBase* base,
               vk::Format format,
               vk::ImageTiling tiling,
               uint32_t size,
               std::string color) {
  vk = base;

  img_format_ = format;
  img_tiling_ = tiling;
  if (color == "white") {
    clear_color_.setColor(WHITE);
  } else if (color == "grey") {
    clear_color_.setColor(GREY);
  } else if (color == "black") {
    clear_color_.setColor(BLACK);
  } else {
    RUNTIME_ERROR("Undefined color: {}", color);
  }
  img_size_ = vk::Extent2D{size, size};

  name_ = fmt::format("{}.LoadOp.{}.{}.{}.{}", TEST_PREFIX,
                      vk::to_string(format), to_string(tiling), size, color);
  desp_ = fmt::format(
      "Clear the image with "
      "VK_ATTACHMENT_LOAD_OP_CLEAR/VK_ATTACHMENT_STORE_OP_STORE with {} "
      "repetitions of an empty render pass.",
      CLEAR_CNT);
}

void LoadOp::Initialize() {
  CreateRenderPass();
  CreateFrameBuffers();

  cmds_ = vk->GetDevice().allocateCommandBuffers(
      {vk->GetCommandPool(), vk::CommandBufferLevel::ePrimary, 1});
  cmds_[0].begin(vk::CommandBufferBeginInfo());
  vk::RenderPassBeginInfo render_pass_info;
  render_pass_info.setFramebuffer(frame_buffer_)
      .setRenderPass(render_pass_)
      .setClearValueCount(1)
      .setPClearValues(&clear_color_)
      .setRenderArea(vk::Rect2D({}, img_size_));
  for (uint32_t i = 0; i < CLEAR_CNT; i++) {
    cmds_[0].beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
    cmds_[0].endRenderPass();
  }
  cmds_[0].end();
}

void LoadOp::Run() {
  vk->SubmitAndWait(cmds_);
}

void LoadOp::Destroy() {
  vk::Device device = vk->GetDevice();
  device.destroyImageView(img_view_);
  delete img_;

  device.destroyFramebuffer(frame_buffer_);
  device.destroyRenderPass(render_pass_);
  device.freeCommandBuffers(vk->GetCommandPool(), cmds_);
}

void LoadOp::CreateRenderPass() {
  vk::AttachmentDescription att_description;
  att_description.setSamples(vk::SampleCountFlagBits::e1)
      .setFormat(img_format_)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

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

void LoadOp::CreateFrameBuffers() {
  const vk::Device device = vk->GetDevice();

  img_ = new vkbench::vkImage(vk, img_size_, img_format_,
                              vk::ImageUsageFlagBits::eColorAttachment |
                                  vk::ImageUsageFlagBits::eTransferSrc,
                              vk::MemoryPropertyFlagBits::eDeviceLocal,
                              img_tiling_);
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

Image LoadOp::GetImage() const {
  const vk::Device device = vk->GetDevice();
  vkImage* dest = vk->GetReadableImage(
      img_, vk::ImageLayout::eColorAttachmentOptimal, img_size_, img_format_);
  DEFER(delete dest);
  vk::SubresourceLayout sub_resource_layout = device.getImageSubresourceLayout(
      dest->GetImage(), {vk::ImageAspectFlagBits::eColor, 0, 0});
  return Image((const unsigned char*)dest->GetMappedMemory(), img_size_,
               sub_resource_layout);
}

double LoadOp::FormatMeasurement(double time) {
  return img_size_.width * img_size_.height * CLEAR_CNT / time;
}

CmdClearImage::CmdClearImage(vkBase* base,
                             vk::Format format,
                             vk::ImageTiling tiling,
                             uint32_t size,
                             std::string color) {
  vk = base;

  img_format_ = format;
  img_tiling_ = tiling;
  if (color == "white") {
    clear_color_ = WHITE;
  } else if (color == "grey") {
    clear_color_ = GREY;
  } else if (color == "black") {
    clear_color_ = BLACK;
  } else {
    RUNTIME_ERROR("Undefined color: {}", color);
  }
  img_size_ = vk::Extent2D{size, size};

  name_ = fmt::format("{}.CmdClearImage.{}.{}.{}.{}", TEST_PREFIX,
                      vk::to_string(format), to_string(tiling), size, color);
  desp_ = fmt::format(
      "Construct a command buffer that clears the image by executing {} "
      "vkCmdClearColorImage in a single submit.",
      CLEAR_CNT);
}

void CmdClearImage::Initialize() {
  const vk::Device device = vk->GetDevice();

  img_ = new vkbench::vkImage(vk, img_size_, img_format_,
                              vk::ImageUsageFlagBits::eColorAttachment |
                                  vk::ImageUsageFlagBits::eTransferDst |
                                  vk::ImageUsageFlagBits::eTransferSrc,
                              vk::MemoryPropertyFlagBits::eDeviceLocal,
                              img_tiling_);
  std::vector<vk::CommandBuffer> cmds = vk->GetDevice().allocateCommandBuffers(
      {vk->GetCommandPool(), vk::CommandBufferLevel::ePrimary, 1});
  DEFER(vk->GetDevice().freeCommandBuffers(vk->GetCommandPool(), cmds));
  cmds[0].begin(vk::CommandBufferBeginInfo());
  img_->MoveLayout(cmds[0], vk::ImageLayout::eUndefined,
                   vk::ImageLayout::eGeneral);
  cmds[0].end();
  vk->SubmitAndWait(cmds);

  cmds_ = device.allocateCommandBuffers(
      {vk->GetCommandPool(), vk::CommandBufferLevel::ePrimary, 1});
  cmds_[0].begin(vk::CommandBufferBeginInfo());
  std::vector<vk::ImageSubresourceRange> subresource_range;
  subresource_range.push_back(
      vk::ImageSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}));
  for (uint32_t i = 0; i < CLEAR_CNT; i++) {
    cmds_[0].clearColorImage(img_->GetImage(), vk::ImageLayout::eGeneral,
                             clear_color_, subresource_range);
  }
  cmds_[0].end();
}

void CmdClearImage::Run() {
  vk->SubmitAndWait(cmds_);
}

void CmdClearImage::Destroy() {
  vk::Device device = vk->GetDevice();
  delete img_;

  device.freeCommandBuffers(vk->GetCommandPool(), cmds_);
}

Image CmdClearImage::GetImage() const {
  const vk::Device device = vk->GetDevice();
  vkImage* dest = vk->GetReadableImage(img_, vk::ImageLayout::eGeneral,
                                       img_size_, img_format_);
  DEFER(delete dest);
  vk::SubresourceLayout sub_resource_layout = device.getImageSubresourceLayout(
      dest->GetImage(), {vk::ImageAspectFlagBits::eColor, 0, 0});
  return Image((const unsigned char*)dest->GetMappedMemory(), img_size_,
               sub_resource_layout);
}

double CmdClearImage::FormatMeasurement(double time) {
  return img_size_.width * img_size_.height * CLEAR_CNT / time;
}

// GenTests generate all the clear tests.
std::vector<vkbench::testBase*> GenTests() {
  std::vector<vkbench::testBase*> tests;
  for (auto format :
       {vk::Format::eR8G8B8A8Unorm, vk::Format::eR32G32B32A32Sfloat}) {
    for (auto tiling : {vk::ImageTiling::eLinear, vk::ImageTiling::eOptimal}) {
      for (auto color : {"white", "grey"}) {
        tests.push_back(
            new LoadOp(vkBase::GetInstance(), format, tiling, 512, color));
        tests.push_back(new CmdClearImage(vkBase::GetInstance(), format, tiling,
                                          512, color));
      }
    }
  }
  return tests;
}
}  // namespace clear
}  // namespace vkbench
