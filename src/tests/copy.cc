// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fmt/format.h>

#include "src/tests/copy.h"

extern FilePath g_spirv_dir;

namespace vkbench {
namespace copy {
const char* TEST_PREFIX = "copy";
const std::vector<std::string> COLORS = {"noise", "white"};

ImageToImage::ImageToImage(uint64_t width, std::string color, vkBase* base) {
  vk = base;
  img_extent_.setHeight(width).setWidth(width);
  color_ = color;
  name_ = fmt::format("{}.ImageToImage.{}.{}", TEST_PREFIX, color, width);
  desp_ = fmt::format("Copy {}x{} size {} image using vkCmdCopyImage.", width,
                      width, color);
}

void ImageToImage::Initialize() {
  vkImage* temp_img = new vkbench::vkImage(
      vk, img_extent_, img_format_, vk::ImageUsageFlagBits::eTransferSrc,
      {vk::MemoryPropertyFlagBits::eHostCoherent |
       vk::MemoryPropertyFlagBits::eHostVisible},
      vk::ImageTiling::eLinear);
  DEFER(delete temp_img);
  src_img_ = new vkbench::vkImage(vk, img_extent_, img_format_,
                                  vk::ImageUsageFlagBits::eTransferSrc |
                                      vk::ImageUsageFlagBits::eTransferDst,
                                  {vk::MemoryPropertyFlagBits::eDeviceLocal},
                                  vk::ImageTiling::eOptimal);
  dest_img_ = new vkbench::vkImage(
      vk, img_extent_, img_format_, vk::ImageUsageFlagBits::eTransferDst,
      {vk::MemoryPropertyFlagBits::eDeviceLocal}, vk::ImageTiling::eOptimal);
  if (color_ == "white") {
    temp_img->FillImage([](int x, int y) {
      return std::array<uint8_t, 4>{255, 255, 255, 255};
    });
  } else if (color_ == "noise") {
    temp_img->FillImage([](int x, int y) {
      return std::array<uint8_t, 4>{static_cast<uint8_t>(randi(255)),
                                    static_cast<uint8_t>(randi(255)),
                                    static_cast<uint8_t>(randi(255)), 255};
    });
  } else {
    RUNTIME_ERROR("Unsupported colors for ImageToImage test: {}", color_);
  }

  vk::CommandBuffer cmd = GetCommandBuffer();
  cmd.begin(vk::CommandBufferBeginInfo());
  dest_img_->MoveLayout(cmd, vk::ImageLayout::ePreinitialized,
                        vk::ImageLayout::eTransferDstOptimal);
  temp_img->MoveLayout(cmd, vk::ImageLayout::ePreinitialized,
                       vk::ImageLayout::eTransferSrcOptimal);
  src_img_->MoveLayout(cmd, vk::ImageLayout::ePreinitialized,
                       vk::ImageLayout::eTransferDstOptimal);

  // Copy the generated linear tiling image to our source image.
  vk::ImageCopy copy_region;
  copy_region.srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  copy_region.dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  copy_region.extent.setWidth(img_extent_.width)
      .setHeight(img_extent_.height)
      .setDepth(1);
  cmd.copyImage(temp_img->GetImage(), vk::ImageLayout::eTransferSrcOptimal,
                src_img_->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1,
                &copy_region);
  src_img_->MoveLayout(cmd, vk::ImageLayout::eTransferDstOptimal,
                       vk::ImageLayout::eTransferSrcOptimal);
  cmd.end();
  vk->SubmitAndWait(cmd);

  // Create the command buffers to test the speed of copying.
  vk::ImageCopy img_copy_region;
  img_copy_region.srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  img_copy_region.dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setLayerCount(1);
  img_copy_region.extent.setWidth(img_extent_.width)
      .setHeight(img_extent_.height)
      .setDepth(1);
  cmd.begin(vk::CommandBufferBeginInfo());
  cmd.copyImage(src_img_->GetImage(), vk::ImageLayout::eTransferSrcOptimal,
                dest_img_->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1,
                &img_copy_region);
  cmd.end();
}

void ImageToImage::Destroy() {
  vkbench::commandTestBase::Destroy();
  delete src_img_;
  delete dest_img_;
}

Image ImageToImage::GetImage() const {
  const vk::Device device = vk->GetDevice();
  vkImage* dest =
      src_img_->GetReadableImage(vk::ImageLayout::eTransferSrcOptimal);
  DEFER(delete dest);
  vk::SubresourceLayout sub_resource_layout = device.getImageSubresourceLayout(
      dest->GetImage(), {vk::ImageAspectFlagBits::eColor, 0, 0});
  return Image((const unsigned char*)dest->GetMappedMemory(), img_extent_,
               sub_resource_layout);
}

BufferToImage::BufferToImage(uint64_t img_extent,
                             std::string color,
                             std::string buffer_memory_property,
                             vkBase* base) {
  vk = base;
  img_extent_.setHeight(img_extent).setWidth(img_extent);
  color_ = color;

  buffer_memory_requirement_.prefer_exclude =
      vk::MemoryPropertyFlagBits::eDeviceLocal;
  buffer_memory_requirement_.include = vk::MemoryPropertyFlagBits::eHostVisible;
  if (buffer_memory_property == "HostCoherent") {
    buffer_memory_requirement_.include =
        vk::MemoryPropertyFlagBits::eHostCoherent;
  } else if (buffer_memory_property == "NonHostCoherent") {
    buffer_memory_requirement_.exclude =
        vk::MemoryPropertyFlagBits::eHostCoherent;
  } else {
    NOT_SUPPORT("memory property {} is not supported yet.",
                buffer_memory_property);
  }
  name_ = fmt::format("{}.BufferToImage.{}.{}.{}", TEST_PREFIX,
                      buffer_memory_property, color, img_extent);
  desp_ = fmt::format(
      "Copy {} {} buffer to {}x{} image using vkCmdCopyBufferToImage.", color,
      buffer_memory_requirement_.String(), img_extent, img_extent);
}

void BufferToImage::Initialize() {
  // Create a buffer which capable to an image.
  uint32_t buffer_size = img_extent_.width * img_extent_.height * 4;
  buffer_ = new vkbench::vkBuffer(vk, buffer_size,
                                  vk::BufferUsageFlagBits::eTransferSrc |
                                      vk::BufferUsageFlagBits::eTransferDst,
                                  buffer_memory_requirement_);
  img_ = new vkbench::vkImage(vk, img_extent_, img_format_,
                              vk::ImageUsageFlagBits::eTransferSrc |
                                  vk::ImageUsageFlagBits::eTransferDst |
                                  vk::ImageUsageFlagBits::eColorAttachment,
                              {vk::MemoryPropertyFlagBits::eDeviceLocal},
                              vk::ImageTiling::eOptimal);

  // Fills the buffer with our data.
  auto data = (unsigned char*)buffer_->GetMappedMemory();
  for (auto i = 0; i < buffer_size; i++) {
    if (i % 4 == 3) {
      // Alpha channel, always set to 255.
      data[i] = 255;
      continue;
    }

    if (color_ == "white") {
      data[i] = 255;
    } else if (color_ == "noise") {
      data[i] = randi(255);
    } else {
      RUNTIME_ERROR("color {} not support", color_);
    }
  }

  vk::CommandBuffer cmd = GetCommandBuffer();
  cmd.begin(vk::CommandBufferBeginInfo());
  img_->MoveLayout(cmd, vk::ImageLayout::ePreinitialized,
                   vk::ImageLayout::eTransferDstOptimal);
  cmd.end();
  vk->SubmitAndWait(cmd);

  // Records the command to copy buffer to image.
  cmd.begin(vk::CommandBufferBeginInfo());
  img_->CopyFrom(cmd, buffer_->GetBuffer());
  cmd.end();
}

void BufferToImage::Destroy() {
  vkbench::commandTestBase::Destroy();
  delete buffer_;
  delete img_;
}

Image BufferToImage::GetImage() const {
  const vk::Device device = vk->GetDevice();
  vkImage* dest = img_->GetReadableImage(vk::ImageLayout::eTransferDstOptimal);
  DEFER(delete dest);
  vk::SubresourceLayout sub_resource_layout = device.getImageSubresourceLayout(
      dest->GetImage(), {vk::ImageAspectFlagBits::eColor, 0, 0});
  return Image((const unsigned char*)dest->GetMappedMemory(), img_extent_,
               sub_resource_layout);
}

BufferStream::BufferStream(vkBase* base) {
  vk = base;
  name_ = fmt::format("{}.BufferStream", TEST_PREFIX);
  desp_ = fmt::format("Copy buffers to vertex buffers in a stream.");
  img_extent_.setHeight(512).setWidth(512);
}

void BufferStream::CreateGraphicsPipeline() {
  std::string vert_shader_code = readShaderFile("bufferStream.vert");
  std::string frag_shader_code = readShaderFile("bufferStream.frag");

  vk::Device device = vk->GetDevice();
  vk::ShaderModule vert_shader_module =
      CreateShaderModule(device, vert_shader_code);
  DEFER(device.destroyShaderModule(vert_shader_module));
  vk::ShaderModule frag_shader_module =
      CreateShaderModule(device, frag_shader_code);
  DEFER(device.destroyShaderModule(frag_shader_module));
  std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {
      {{}, vk::ShaderStageFlagBits::eVertex, vert_shader_module, "main"},
      {{}, vk::ShaderStageFlagBits::eFragment, frag_shader_module, "main"}};

  // binding dst_buffer_ to vertex shader.
  vk::VertexInputBindingDescription binding_description;
  binding_description.setStride(sizeof(float) * 2)
      .setInputRate(vk::VertexInputRate::eVertex);

  vk::VertexInputAttributeDescription attribute_description;
  attribute_description.setLocation(0)
      .setFormat(vk::Format::eR32G32Sfloat)
      .setOffset(0);

  vk::PipelineVertexInputStateCreateInfo vertex_input;
  vertex_input.setVertexBindingDescriptionCount(1)
      .setVertexAttributeDescriptionCount(1)
      .setPVertexBindingDescriptions(&binding_description)
      .setPVertexAttributeDescriptions(&attribute_description);
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
      .setPStages(shader_stages.data())
      .setPVertexInputState(&vertex_input)
      .setPInputAssemblyState(&input_assembly)
      .setPViewportState(&viewport_state)
      .setPRasterizationState(&rasterizer)
      .setPMultisampleState(&multisampling)
      .setPColorBlendState(&color_info)
      .setLayout(pipeline_layout_)
      .setRenderPass(render_pass_);

  graphics_pipeline_ = device.createGraphicsPipeline({}, pipelineInfo).value;
}

void BufferStream::CreateRenderPass() {
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

void BufferStream::Initialize() {
  CreateRenderPass();
  CreateGraphicsPipeline();

  // Convert vertices_ into buffer.
  src_buffer_ = new vkbench::vkBuffer(
      vk, sizeof(vertices_), vk::BufferUsageFlagBits::eTransferSrc,
      {vk::MemoryPropertyFlagBits::eDeviceLocal |
       vk::MemoryPropertyFlagBits::eHostVisible});
  auto data = reinterpret_cast<float**>(src_buffer_->GetMappedMemory());
  memcpy(data, vertices_, sizeof(vertices_));

  // Create a vertex buffer.
  dst_buffer_ =
      new vkbench::vkBuffer(vk, vertex_buffer_size_,
                            vk::BufferUsageFlagBits::eTransferDst |
                                vk::BufferUsageFlagBits::eVertexBuffer,
                            {vk::MemoryPropertyFlagBits::eDeviceLocal});

  // Create framebuffer for drawing.
  img_ = new vkbench::vkImage(
      vk, img_extent_, img_format_, vk::ImageUsageFlagBits::eColorAttachment,
      {vk::MemoryPropertyFlagBits::eDeviceLocal}, vk::ImageTiling::eOptimal);

  frame_buffer_ = DefaultFramebuffer(vk->GetDevice(), img_, render_pass_,
                                     img_->GetDefaultImageView());

  vk::ClearValue clear_color(std::array<float, 4>{0.f, 0.f, 0.f, 1.f});
  render_pass_info_.setFramebuffer(frame_buffer_)
      .setRenderPass(render_pass_)
      .setClearValueCount(1)
      .setPClearValues(&clear_color)
      .setRenderArea(vk::Rect2D({}, img_extent_));
}

void BufferStream::Setup(int i) {
  vk::CommandBuffer cmd = GetCommandBuffer();
  cmd.begin(vk::CommandBufferBeginInfo{
      vk::CommandBufferUsageFlagBits::eSimultaneousUse});
  auto buffer_size = dst_buffer_->GetSize();
  uint32_t total_number = buffer_size / sizeof(vertices_);
  uint64_t data_size = sizeof(vertices_);
  uint64_t offset = 0;
  for (auto j = 0; j < i; j++) {
    if (offset + data_size > buffer_size) {
      offset = 0;
    }
    vk::BufferCopy buffer_copy;
    buffer_copy.setSize(data_size).setSrcOffset(0).setDstOffset(offset);
    cmd.copyBuffer(src_buffer_->GetBuffer(), dst_buffer_->GetBuffer(),
                   buffer_copy);
    offset += data_size;
  }
  cmd.end();
}

void BufferStream::Destroy() {
  delete dst_buffer_;
  delete src_buffer_;
  delete img_;

  vk::Device device = vk->GetDevice();
  device.destroyFramebuffer(frame_buffer_);
  device.destroyPipelineLayout(pipeline_layout_);
  device.destroyPipeline(graphics_pipeline_);
  device.destroyRenderPass(render_pass_);
  vkbench::singleCommandTestBase::Destroy();
}

std::vector<testBase*> GenTests() {
  std::vector<testBase*> tests;
  for (auto width : {64, 128, 512, 1024}) {
    for (auto color : COLORS) {
      tests.push_back(new ImageToImage(width, color, vkBase::GetInstance()));
    }
  }

  for (auto width : {256, 512, 1024}) {
    for (auto color : COLORS) {
      for (auto memory_type : {"HostCoherent", "NonHostCoherent"}) {
        tests.push_back(new BufferToImage(width, color, memory_type,
                                          vkBase::GetInstance()));
      }
    }
  }
  tests.push_back(new BufferStream(vkBase::GetInstance()));
  return tests;
}

}  // namespace copy
}  // namespace vkbench
