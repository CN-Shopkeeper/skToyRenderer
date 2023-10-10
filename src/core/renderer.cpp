#include "renderer.hpp"

#include "context.hpp"

namespace sktr {

const std::array<Vertex, 4> vertices = {
    Vertex{Vec{-0.5, -0.5}, Vec{0, 0}},
    Vertex{Vec{0.5, -0.5}, Vec{1, 0}},
    Vertex{Vec{0.5, 0.5}, Vec{1, 1}},
    Vertex{Vec{-0.5, 0.5}, Vec{0, 1}},
};

const std::uint32_t indices[] = {0, 1, 3, 1, 2, 3};

Renderer::Renderer(int width, int height, int maxFlightCount)
    : maxFlightCount_(maxFlightCount), curFrame_(0) {
  allocCmdBuffers();
  createSemaphores();
  createFences();

  createBuffers();

  initMats();
  SetProjection(0, width, 0, height, 1, -1);

  descriptorSets_ =
      DescriptorSetManager::GetInstance().AllocBufferSets(maxFlightCount);
  updateDescriptorSets();

  createWhiteTexture();
  SetDrawColor(Color{1, 1, 1});
}

Renderer::~Renderer() {
  auto& device = Context::GetInstance().device;
  // 在textureManager中删除了
  // texture.reset();
  // ! 应当手动调用Texture Manager的clear，
  // ! 因为单例的析构函数在最后，会导致内部的texture析构时device以及为空
  TextureManager::GetInstance().Clear();
  hostRectIndicesBuffer_.reset();
  deviceRectIndicesBuffer_.reset();
  // ? 释放line
  hostLineVertexBuffer_.reset();
  deviceLineVertexBuffer_.reset();
  hostLineVertexBuffer_.reset();
  deviceLineVertexBuffer_.reset();
  hostUniformBuffers_.clear();
  deviceUniformBuffers_.clear();
  hostRectVertexBuffer_.reset();
  deviceRectVertexBuffer_.reset();
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

void Renderer::StartRender() {
  auto& device = Context::GetInstance().device;
  auto& renderProcess = Context::GetInstance().renderProcess;
  auto& swapchain = Context::GetInstance().swapchain;

  auto& fence = fences_[curFrame_];

  if (device.waitForFences(fence, true,
                           std::numeric_limits<std::uint64_t>::max()) !=
      vk::Result::eSuccess) {
    throw std::runtime_error("wait for fence failed");
  }
  device.resetFences(fence);

  /*
   * Semaphore - GPU internal; Queue - Queue
   * Fence - CPU - GPU
   */

  auto& imageAvaliableSem = imageAvaliableSems_[curFrame_];
  auto& renderFinishSem = renderFinishSems_[curFrame_];

  auto result = device.acquireNextImageKHR(swapchain->swapchain,
                                           std::numeric_limits<uint64_t>::max(),
                                           imageAvaliableSem);
  if (result.result != vk::Result::eSuccess) {
    std::cout << "acquire next image fialed" << std::endl;
  }

  imageIndex_ = result.value;

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
  vk::ClearValue clearValue;
  clearValue.color =
      vk::ClearColorValue(std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.0f});
  area.setOffset({0, 0}).setExtent(swapchain->info.imageExtent);
  renderPassBegin.setRenderPass(renderProcess->renderPass)
      .setRenderArea(area)
      .setFramebuffer(swapchain->framebuffers[imageIndex_])
      .setClearValues(clearValue);

  cmdBuff.beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);
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

void Renderer::DrawTexture(const Rect& rect, Texture& texture) {
  auto& ctx = Context::GetInstance();
  auto& device = ctx.device;
  auto& cmdBuff = cmdBuffs_[curFrame_];
  auto& renderProcess = Context::GetInstance().renderProcess;
  vk::DeviceSize offset = 0;
  bufferRectData();

  cmdBuff.bindPipeline(vk::PipelineBindPoint::eGraphics,
                       renderProcess->graphicsPipelineWithTriangleTopology);
  cmdBuff.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics, renderProcess->pipelineLayout, 0,
      {descriptorSets_[curFrame_].set, texture.set.set}, {});
  cmdBuff.bindVertexBuffers(0, deviceRectVertexBuffer_->buffer, offset);
  cmdBuff.bindIndexBuffer(deviceRectIndicesBuffer_->buffer, 0,
                          vk::IndexType::eUint32);
  auto model =
      Mat4::CreateTranslate(rect.position).Mul(Mat4::CreateScale(rect.size));
  cmdBuff.pushConstants(renderProcess->pipelineLayout, vk::ShaderStageFlagBits::eVertex,
                        0, sizeof(Mat4), model.GetData());
  cmdBuff.pushConstants(renderProcess->pipelineLayout,
                        vk::ShaderStageFlagBits::eFragment, sizeof(Mat4),
                        sizeof(Color), &drawColor_);
  cmdBuff.drawIndexed(6, 1, 0, 0, 0);
}

void Renderer::DrawLine(const Vec& p1, const Vec& p2) {
  auto& ctx = Context::GetInstance();
  auto& device = ctx.device;
  auto& cmd = cmdBuffs_[curFrame_];
  vk::DeviceSize offset = 0;

  bufferLineData(p1, p2);

  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                   ctx.renderProcess->graphicsPipelineWithLineTopology);
  cmd.bindVertexBuffers(0, deviceLineVertexBuffer_->buffer, offset);

  auto& layout = Context::GetInstance().renderProcess->pipelineLayout;
  cmd.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics, layout, 0,
      {descriptorSets_[curFrame_].set, whiteTexture->set.set}, {});
  auto model = Mat4::CreateIdentity();
  cmd.pushConstants(layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Mat4),
                    model.GetData());
  cmd.pushConstants(layout, vk::ShaderStageFlagBits::eFragment, sizeof(Mat4),
                    sizeof(Color), &drawColor_);
  cmd.draw(2, 1, 0, 0);
}

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

void Renderer::createVertexBuffer() {
  hostRectVertexBuffer_.reset(new Buffer{
      sizeof(vertices),
      // 数据传输的起点
      vk::BufferUsageFlagBits::eTransferSrc,
      //    eHostVisible: cpu可以访问这个内存
      //    eHostCoherent：cpu和gpu共享内存,但是对于GPU这块内存不是最优
      vk::MemoryPropertyFlagBits::eHostVisible |
          vk::MemoryPropertyFlagBits::eHostCoherent});
  deviceRectVertexBuffer_.reset(
      new Buffer{sizeof(vertices),
                 vk::BufferUsageFlagBits::eVertexBuffer |
                     vk::BufferUsageFlagBits::eTransferDst,
                 //   gpu本地的内存
                 vk::MemoryPropertyFlagBits::eDeviceLocal});

  hostLineVertexBuffer_.reset(
      new Buffer{sizeof(Vertex) * 2, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent});
  deviceLineVertexBuffer_.reset(
      new Buffer{sizeof(Vertex) * 2,
                 vk::BufferUsageFlagBits::eVertexBuffer |
                     vk::BufferUsageFlagBits::eTransferDst,
                 vk::MemoryPropertyFlagBits::eDeviceLocal});
}

void Renderer::createIndicesBuffer() {
  size_t size = sizeof(indices);
  hostRectIndicesBuffer_.reset(
      new Buffer{size, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent});
  deviceRectIndicesBuffer_.reset(
      new Buffer{size,
                 vk::BufferUsageFlagBits::eTransferDst |
                     vk::BufferUsageFlagBits::eIndexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal});
}

void Renderer::createUniformBuffers() {
  hostUniformBuffers_.resize(maxFlightCount_);
  deviceUniformBuffers_.resize(maxFlightCount_);

  size_t size = sizeof(matrices);

  for (auto& buffer : hostUniformBuffers_) {
    buffer.reset(new Buffer{size, vk::BufferUsageFlagBits::eTransferSrc,
                            vk::MemoryPropertyFlagBits::eHostCoherent |
                                vk::MemoryPropertyFlagBits::eHostVisible});
  }

  for (auto& buffer : deviceUniformBuffers_) {
    buffer.reset(new Buffer{size,
                            vk::BufferUsageFlagBits::eTransferDst |
                                vk::BufferUsageFlagBits::eUniformBuffer,
                            vk::MemoryPropertyFlagBits::eDeviceLocal});
  }
}

void Renderer::bufferRectVertexData() {
  memcpy(hostRectVertexBuffer_->map, vertices.data(), sizeof(vertices));

  copyBuffer(hostRectVertexBuffer_->buffer, deviceRectVertexBuffer_->buffer,
             hostRectVertexBuffer_->size, 0, 0);
}

void Renderer::bufferRectIndicesData() {
  memcpy(hostRectIndicesBuffer_->map, indices, sizeof(indices));

  copyBuffer(hostRectIndicesBuffer_->buffer, deviceRectIndicesBuffer_->buffer,
             hostRectIndicesBuffer_->size, 0, 0);
}

void Renderer::bufferLineData(const Vec& p1, const Vec& p2) {
  Vertex vertices[]{{p1, Vec{0, 0}}, {p2, Vec{0, 0}}};
  memcpy(hostLineVertexBuffer_->map, vertices, sizeof(vertices));
  copyBuffer(hostLineVertexBuffer_->buffer, deviceLineVertexBuffer_->buffer,
             hostLineVertexBuffer_->size, 0, 0);
}

void Renderer::bufferMVPData() {
  for (int i = 0; i < hostUniformBuffers_.size(); i++) {
    auto& buffer = hostUniformBuffers_[i];
    memcpy(buffer->map, (void*)&matrices, sizeof(matrices));
    copyBuffer(buffer->buffer, deviceUniformBuffers_[i]->buffer, buffer->size,
               0, 0);
  }
}

void Renderer::SetDrawColor(const Color& color) { drawColor_ = color; }

void Renderer::createBuffers() {
  createVertexBuffer();
  createIndicesBuffer();
  createUniformBuffers();
}

void Renderer::bufferRectData() {
  bufferRectVertexData();
  bufferRectIndicesData();
}

void Renderer::initMats() {
  matrices.project = Mat4::CreateIdentity();
  matrices.view = Mat4::CreateIdentity();
}

// 创建一个只有一个像素的图像，用于绘制线段
void Renderer::createWhiteTexture() {
  unsigned char data[] = {0xFF, 0xFF, 0xFF, 0xFF};
  whiteTexture = TextureManager::GetInstance().Create((void*)data, 1, 1);
}

void Renderer::SetProjection(int left, int right, int bottom, int top, int near,
                             int far) {
  matrices.project = Mat4::CreateOrtho(left, right, bottom, top, near, far);
  bufferMVPData();
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
  for (int i = 0; i < descriptorSets_.size(); i++) {
    std::vector<vk::WriteDescriptorSet> writeInfos(1);
    auto& set = descriptorSets_[i];
    // bind MVP buffer
    vk::DescriptorBufferInfo bufferInfoMVP;
    bufferInfoMVP.setBuffer(deviceUniformBuffers_[i]->buffer)
        .setOffset(0)
        .setRange(sizeof(matrices));

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