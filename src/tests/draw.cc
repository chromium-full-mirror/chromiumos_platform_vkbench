// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fmt/format.h>
#include <utility>

#include "src/tests/draw.h"

extern FilePath g_spirv_dir;

namespace vkbench {
namespace draw {
const char* TEST_PREFIX = "draw";

DrawSize::DrawSize(uint64_t width, vkBase* base) {
  vk = base;
  img_extent_.setHeight(width).setWidth(width);
  name_ = fmt::format("{}.DrawSize.{}", TEST_PREFIX, width);
  desp_ = fmt::format("Draws {}x{} size image with solid color.", width, width);
}
void DrawSize::Initialize() {
  CreateRenderPass();
  CreateGraphicsPipeline();
  CreateFrameBuffers();
  // Create a cmdBuffer with draw call.
  vk::CommandBuffer cmd = GetCommandBuffer();
  cmd.begin(vk::CommandBufferBeginInfo());
  vk::ClearValue clear_color(std::array<float, 4>{0.f, 0.f, 0.f, 1.f});
  vk::RenderPassBeginInfo render_pass_info;
  render_pass_info.setFramebuffer(frame_buffer_)
      .setRenderPass(render_pass_)
      .setClearValueCount(1)
      .setPClearValues(&clear_color)
      .setRenderArea(vk::Rect2D({}, img_extent_));
  cmd.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline_);
  cmd.draw(3, 1, 0, 0);
  cmd.endRenderPass();
  cmd.end();
}

void DrawSize::Destroy() {
  vkbench::commandTestBase::Destroy();

  vk::Device device = vk->GetDevice();
  device.destroyImageView(img_view_);
  delete img_;

  device.destroyFramebuffer(frame_buffer_);
  device.destroyRenderPass(render_pass_);
  device.destroyPipeline(graphics_pipeline_);
  device.destroyPipelineLayout(pipeline_layout_);
}

void DrawSize::CreateRenderPass() {
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

void DrawSize::CreateGraphicsPipeline() {
  std::string vert_shader_code = readShaderFile("drawSizeTest.vert.spv");
  std::string frag_shader_code = readShaderFile("drawSizeTest.frag.spv");
  vk::Device device = vk->GetDevice();

  vk::ShaderModule vert_shader_module =
      CreateShaderModule(device, vert_shader_code);
  DEFER(device.destroyShaderModule(vert_shader_module));
  vk::ShaderModule frag_shader_module =
      CreateShaderModule(device, frag_shader_code);
  DEFER(device.destroyShaderModule(frag_shader_module));
  vk::PipelineShaderStageCreateInfo shader_stages[] = {
      {{}, vk::ShaderStageFlagBits::eVertex, vert_shader_module, "main"},
      {{}, vk::ShaderStageFlagBits::eFragment, frag_shader_module, "main"}};

  vk::PipelineVertexInputStateCreateInfo vertex_input;
  vk::PipelineInputAssemblyStateCreateInfo input_assembly;
  input_assembly.setTopology(vk::PrimitiveTopology::eTriangleList);

  vk::Viewport viewport;
  viewport.setWidth(img_extent_.width)
      .setHeight(img_extent_.height)
      .setMaxDepth(1);
  vk::Rect2D scissor;
  scissor.setOffset({0, 0}).setExtent(img_extent_);
  vk::PipelineViewportStateCreateInfo viewport_state;
  viewport_state.setViewportCount(1)
      .setPViewports(&viewport)
      .setScissorCount(1)
      .setPScissors(&scissor);

  vk::PipelineRasterizationStateCreateInfo rasterizer;
  rasterizer.setLineWidth(1);
  vk::PipelineMultisampleStateCreateInfo multisampling;
  multisampling.setRasterizationSamples(vk::SampleCountFlagBits::e1);
  vk::PipelineColorBlendAttachmentState color_attachment;
  color_attachment.setBlendEnable(false).setColorWriteMask(
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
  vk::PipelineColorBlendStateCreateInfo color_info;
  color_info.setAttachmentCount(1).setPAttachments(&color_attachment);

  vk::PipelineLayoutCreateInfo layout_info;
  pipeline_layout_ = device.createPipelineLayout(layout_info);

  vk::GraphicsPipelineCreateInfo pipelineInfo;
  pipelineInfo.setStageCount(2)
      .setPStages(shader_stages)
      .setPVertexInputState(&vertex_input)
      .setPInputAssemblyState(&input_assembly)
      .setPViewportState(&viewport_state)
      .setPRasterizationState(&rasterizer)
      .setPMultisampleState(&multisampling)
      .setPColorBlendState(&color_info)
      .setLayout(pipeline_layout_)
      .setRenderPass(render_pass_);

  graphics_pipeline_ =
      std::move(device.createGraphicsPipeline({}, pipelineInfo).value);
}

void DrawSize::CreateFrameBuffers() {
  const vk::Device device = vk->GetDevice();

  img_ = new vkbench::vkImage(vk, img_extent_, img_format_,
                              vk::ImageUsageFlagBits::eColorAttachment |
                                  vk::ImageUsageFlagBits::eTransferSrc,
                              {vk::MemoryPropertyFlagBits::eDeviceLocal});

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
      .setWidth(img_extent_.width)
      .setHeight(img_extent_.height)
      .setLayers(1);
  frame_buffer_ = device.createFramebuffer(frame_buffer_info);
}

Image DrawSize::GetImage() const {
  const vk::Device device = vk->GetDevice();
  vkImage* dest = img_->GetReadableImage(vk::ImageLayout::eGeneral, img_extent_,
                                         img_format_);
  DEFER(delete dest);
  vk::SubresourceLayout sub_resource_layout = device.getImageSubresourceLayout(
      dest->GetImage(), {vk::ImageAspectFlagBits::eColor, 0, 0});
  return Image((const unsigned char*)dest->GetMappedMemory(), img_extent_,
               sub_resource_layout);
}

std::vector<testBase*> GenTests() {
  return std::vector<testBase*>{
      new DrawSize(16, vkBase::GetInstance()),
      new DrawSize(64, vkBase::GetInstance()),
      new DrawSize(128, vkBase::GetInstance()),
      new DrawSize(512, vkBase::GetInstance()),
      new DrawSize(1024, vkBase::GetInstance()),
  };
}

}  // namespace draw
}  // namespace vkbench
