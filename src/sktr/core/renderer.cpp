#include "renderer.hpp"

#include "context.hpp"

namespace sktr {

Renderer::Renderer(int width, int height, int maxFlightCount)
    : maxFlightCount_(maxFlightCount), curFrame_(0) {
  allocCmdBuffers();
  createSemaphores();
  createFences();

  createUniformBuffers();

  initMats();
  SetProjection(glm::radians(45.0f), width / (float)height, 0.1f, 10.0f);

  worldUniformDescriptorSets_ =
      DescriptorSetManager::GetInstance().AllocBufferSets(maxFlightCount);
  updateDescriptorSets();

  // createWhiteTexture();
  SetDrawColor(Color{1, 1, 1});
}

Renderer::~Renderer() {
  auto& device = Context::GetInstance().device;
  // 在textureManager中删除了
  // texture.reset();
  // ! 应当手动调用Texture Manager的clear，
  // ! 因为单例的析构函数在最后，会导致内部的texture析构时device以及为空
  TextureManager::GetInstance().Clear();
  uniformWorldBuffers.clear();
  for (auto& sem : imageAvaliableSems_) {
    device.destroySemaphore(sem);
  }
  for (auto& sem : renderFinishSems_) {
    device.destroySemaphore(sem);
  }
  for (auto& fence : fences_) {
    device.destroyFence(fence);
  }

  Context::GetInstance().commandManager->FreeCommandBuffers(cmdBuffs_);
}

void Renderer::allocCmdBuffers() {
  cmdBuffs_.resize(maxFlightCount_);
  for (auto& cmdBuff : cmdBuffs_) {
    cmdBuff = Context::GetInstance().commandManager->CreateOneCommandBuffer();
  }
}

bool Renderer::StartRender() {
  auto& device = Context::GetInstance().device;
  auto& renderProcess = Context::GetInstance().renderProcess;
  auto& swapchain = Context::GetInstance().swapchain;

  /*
   * Semaphore - GPU internal; Queue - Queue
   * Fence - CPU - GPU
   */

  auto& imageAvaliableSem = imageAvaliableSems_[curFrame_];
  auto& renderFinishSem = renderFinishSems_[curFrame_];
  auto& fence = fences_[curFrame_];

  if (device.waitForFences(fence, true,
                           std::numeric_limits<std::uint64_t>::max()) !=
      vk::Result::eSuccess) {
    throw std::runtime_error("wait for fence failed");
  }
  auto result = device.acquireNextImageKHR(swapchain->swapchain,
                                           std::numeric_limits<uint64_t>::max(),
                                           imageAvaliableSem);
  if (result.result == vk::Result::eErrorOutOfDateKHR) {
    swapchain->Recreate();
    return false;
  } else if (result.result != vk::Result::eSuccess &&
             result.result != vk::Result::eSuboptimalKHR) {
    std::cout << "acquire next image fialed" << std::endl;
  }

  imageIndex_ = result.value;
  device.resetFences(fence);

  auto& cmdBuff = cmdBuffs_[curFrame_];
  cmdBuff.reset();

  vk::CommandBufferBeginInfo beginInfo;
  // OneTimeSubmit: 提交一次之后失效
  // RenderPassContinue: 在渲染流程中生命周期内都有效
  // SimultaneousUse: 可以一直重复使用
  beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  cmdBuff.begin(beginInfo);

  vk::RenderPassBeginInfo renderPassBegin;
  vk::Rect2D area;
  std::array<vk::ClearValue, 2> clearValues{};
  clearValues[0].color =
      vk::ClearColorValue(std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.0f});
  clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

  area.setOffset({0, 0}).setExtent(swapchain->info.imageExtent);
  renderPassBegin.setRenderPass(renderProcess->renderPass)
      .setRenderArea(area)
      .setFramebuffer(swapchain->framebuffers[imageIndex_])
      .setClearValues(clearValues);

  cmdBuff.beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);
  return true;
}

void Renderer::EndRender() {
  auto& swapchain = Context::GetInstance().swapchain;
  auto& cmdBuff = cmdBuffs_[curFrame_];
  auto& imageAvaliableSem = imageAvaliableSems_[curFrame_];
  auto& renderFinishSem = renderFinishSems_[curFrame_];
  auto& fence = fences_[curFrame_];

  cmdBuff.endRenderPass();

  cmdBuff.end();

  vk::SubmitInfo submitInfo;
  vk::PipelineStageFlags flags =
      vk::PipelineStageFlagBits::eColorAttachmentOutput;
  submitInfo.setCommandBuffers(cmdBuff)
      .setWaitSemaphores(imageAvaliableSem)
      .setSignalSemaphores(renderFinishSem)
      .setWaitDstStageMask(flags);
  Context::GetInstance().graphicsQueue.submit(submitInfo, fence);

  vk::PresentInfoKHR present;
  present.setImageIndices(imageIndex_)
      .setSwapchains(swapchain->swapchain)
      .setWaitSemaphores(renderFinishSem);

  if (Context::GetInstance().presentQueue.presentKHR(present) !=
      vk::Result::eSuccess) {
    std::cout << "image present failed" << std::endl;
  }

  curFrame_ = (curFrame_ + 1) % maxFlightCount_;
}

void Renderer::DrawModel(const Model& model) {
  auto& ctx = Context::GetInstance();
  auto& device = ctx.device;
  auto& cmdBuff = cmdBuffs_[curFrame_];
  auto& renderProcess = Context::GetInstance().renderProcess;
  vk::DeviceSize offset = 0;

  cmdBuff.bindPipeline(vk::PipelineBindPoint::eGraphics,
                       renderProcess->graphicsPipelineWithTriangleTopology);
  cmdBuff.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                             renderProcess->pipelineLayout, 0,
                             worldUniformDescriptorSets_[curFrame_].set, {});
  cmdBuff.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                             renderProcess->pipelineLayout, 1,
                             model.texture->set.set, {});
  cmdBuff.bindVertexBuffers(0, model.vertexBuffer->buffer, offset);
  cmdBuff.bindIndexBuffer(model.indicesBuffer->buffer, 0,
                          vk::IndexType::eUint32);

  cmdBuff.pushConstants(renderProcess->pipelineLayout,
                        vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4),
                        &model.modelMatrix);
  cmdBuff.pushConstants(renderProcess->pipelineLayout,
                        vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4),
                        sizeof(Color), &drawColor_);
  cmdBuff.drawIndexed(model.indices.size(), 1, 0, 0, 0);
}

// void Renderer::DrawTexture(const Rect& rect, Texture& texture) {
//   auto& ctx = Context::GetInstance();
//   auto& device = ctx.device;
//   auto& cmdBuff = cmdBuffs_[curFrame_];
//   auto& renderProcess = Context::GetInstance().renderProcess;
//   vk::DeviceSize offset = 0;
//   bufferRectData();

//   cmdBuff.bindPipeline(vk::PipelineBindPoint::eGraphics,
//                        renderProcess->graphicsPipelineWithTriangleTopology);
//   cmdBuff.bindDescriptorSets(
//       vk::PipelineBindPoint::eGraphics, renderProcess->pipelineLayout, 0,
//       {descriptorSets_[curFrame_].set, texture.set.set}, {});
//   cmdBuff.bindVertexBuffers(0, deviceRectVertexBuffer_->buffer, offset);
//   cmdBuff.bindIndexBuffer(deviceRectIndicesBuffer_->buffer, 0,
//                           vk::IndexType::eUint32);
//   auto model =
//       Mat4::CreateTranslate(rect.position).Mul(Mat4::CreateScale(rect.size));
//   cmdBuff.pushConstants(renderProcess->pipelineLayout,
//                         vk::ShaderStageFlagBits::eVertex, 0, sizeof(Mat4),
//                         model.GetData());
//   cmdBuff.pushConstants(renderProcess->pipelineLayout,
//                         vk::ShaderStageFlagBits::eFragment, sizeof(Mat4),
//                         sizeof(Color), &drawColor_);
//   cmdBuff.drawIndexed(6, 1, 0, 0, 0);
// }

// void Renderer::DrawLine(const Vec& p1, const Vec& p2) {
//   auto& ctx = Context::GetInstance();
//   auto& device = ctx.device;
//   auto& cmd = cmdBuffs_[curFrame_];
//   vk::DeviceSize offset = 0;

//   bufferLineData(p1, p2);

//   cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
//                    ctx.renderProcess->graphicsPipelineWithLineTopology);
//   cmd.bindVertexBuffers(0, deviceLineVertexBuffer_->buffer, offset);

//   auto& layout = Context::GetInstance().renderProcess->pipelineLayout;
//   cmd.bindDescriptorSets(
//       vk::PipelineBindPoint::eGraphics, layout, 0,
//       {descriptorSets_[curFrame_].set, whiteTexture->set.set}, {});
//   auto model = Mat4::CreateIdentity();
//   cmd.pushConstants(layout, vk::ShaderStageFlagBits::eVertex, 0,
//   sizeof(Mat4),
//                     model.GetData());
//   cmd.pushConstants(layout, vk::ShaderStageFlagBits::eFragment, sizeof(Mat4),
//                     sizeof(Color), &drawColor_);
//   cmd.draw(2, 1, 0, 0);
// }

void Renderer::createWhiteTexture() {}

void Renderer::createSemaphores() {
  auto& device = Context::GetInstance().device;
  vk::SemaphoreCreateInfo semaphoreInfo;

  imageAvaliableSems_.resize(maxFlightCount_);
  renderFinishSems_.resize(maxFlightCount_);

  for (auto& sem : imageAvaliableSems_) {
    sem = device.createSemaphore(semaphoreInfo);
  }

  for (auto& sem : renderFinishSems_) {
    sem = device.createSemaphore(semaphoreInfo);
  }
}

void Renderer::createFences() {
  fences_.resize(maxFlightCount_, nullptr);
  for (auto& fence : fences_) {
    vk::FenceCreateInfo fenceInfo;
    fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
    fence = Context::GetInstance().device.createFence(fenceInfo);
  }
}

void Renderer::createUniformBuffers() {
  size_t size = sizeof(WorldMatrices);
  uniformWorldBuffers.resize(maxFlightCount_);

  for (size_t i = 0; i < maxFlightCount_; i++) {
    uniformWorldBuffers[i].reset(
        new Buffer{size, vk::BufferUsageFlagBits::eUniformBuffer,
                   vk::MemoryPropertyFlagBits::eHostCoherent |
                       vk::MemoryPropertyFlagBits::eHostVisible});
  }
}

void Renderer::bufferWorldData() {
  for (int i = 0; i < uniformWorldBuffers.size(); i++) {
    memcpy(uniformWorldBuffers[i]->map, &worldMatrices_,
           sizeof(worldMatrices_));
  }
}

void Renderer::SetDrawColor(const Color& color) { drawColor_ = color; }

void Renderer::initMats() {
  worldMatrices_.proj = glm::identity<glm::mat4>();
  worldMatrices_.view = glm::identity<glm::mat4>();
}

// 创建一个只有一个像素的图像，用于绘制线段
// void Renderer::createWhiteTexture() {
//   unsigned char data[] = {0xFF, 0xFF, 0xFF, 0xFF};
//   whiteTexture = TextureManager::GetInstance().Create((void*)data, 1, 1);
// }

void Renderer::SetProjection(float fov, float aspect, float near, float far) {
  worldMatrices_.proj = glm::perspective(fov, aspect, near, far);
  worldMatrices_.proj *= -1;
  bufferWorldData();
}

void Renderer::copyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size,
                          size_t srcOffset, size_t dstOffset) {
  auto& ctx = Context::GetInstance();
  ctx.commandManager->ExecuteCmd(
      ctx.graphicsQueue, [&](vk::CommandBuffer& cmdBuff) {
        vk::BufferCopy region;
        region.setSize(size).setSrcOffset(srcOffset).setDstOffset(dstOffset);
        cmdBuff.copyBuffer(src, dst, region);
      });
}

void Renderer::updateDescriptorSets() {
  for (int i = 0; i < worldUniformDescriptorSets_.size(); i++) {
    std::vector<vk::WriteDescriptorSet> writeInfos(1);
    auto& set = worldUniformDescriptorSets_[i];
    // bind MVP buffer
    vk::DescriptorBufferInfo bufferInfoMVP;
    bufferInfoMVP.setBuffer(uniformWorldBuffers[i]->buffer)
        .setOffset(0)
        .setRange(sizeof(worldMatrices_));

    writeInfos[0]
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setBufferInfo(bufferInfoMVP)
        .setDstBinding(0)
        .setDstSet(set.set)
        .setDstArrayElement(0)
        .setDescriptorCount(1);

    Context::GetInstance().device.updateDescriptorSets(writeInfos, {});
  }
}

}  // namespace sktr