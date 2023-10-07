#include "command_manager.hpp"

#include "core/context.hpp"

namespace sktr {
CommandManager::CommandManager() { pool_ = createCommandPool(); }

CommandManager::~CommandManager() {
  auto& ctx = Context::GetInstance();
  ctx.device.destroyCommandPool(pool_);
}

void CommandManager::ResetCmds() {
  Context::GetInstance().device.resetCommandPool(pool_);
}

vk::CommandPool CommandManager::createCommandPool() {
  auto& ctx = Context::GetInstance();
  vk::CommandPoolCreateInfo commandPoolInfo;
  commandPoolInfo
      // eTransient：瞬间的，可以在一帧内反复使用的
      // eResetCommandBuffer：可以单独reset
      .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
  return ctx.device.createCommandPool(commandPoolInfo);
}

std::vector<vk::CommandBuffer> CommandManager::CreateCommandBuffers(
    std::uint32_t count) {
  auto& ctx = Context::GetInstance();

  vk::CommandBufferAllocateInfo allocateInfo;
  allocateInfo.setCommandPool(pool_)
      .setCommandBufferCount(1)
      // primary 主要的，可以被cpu直接执行的
      .setLevel(vk::CommandBufferLevel::ePrimary);

  return ctx.device.allocateCommandBuffers(allocateInfo);
}

vk::CommandBuffer CommandManager::CreateOneCommandBuffer() {
  return CreateCommandBuffers(1)[0];
}

void CommandManager::FreeOneCommandBuffer(vk::CommandBuffer& cmdBuff) {
  auto& device = Context::GetInstance().device;
  device.freeCommandBuffers(pool_, cmdBuff);
}

void CommandManager::FreeCommandBuffers(
    std::vector<vk::CommandBuffer>& cmdBuffs) {
  auto& device = Context::GetInstance().device;
  device.freeCommandBuffers(pool_, cmdBuffs);
}

void CommandManager::ExecuteCmd(vk::Queue queue, RecordCmdFunc func) {
  auto cmdBuff = CreateOneCommandBuffer();

  vk::CommandBufferBeginInfo beginInfo;
  beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  cmdBuff.begin(beginInfo);
  if (func) {
    func(cmdBuff);
  }
  cmdBuff.end();

  vk::SubmitInfo submitInfo;
  submitInfo.setCommandBuffers(cmdBuff);
  queue.submit(submitInfo);
  queue.waitIdle();
  Context::GetInstance().device.waitIdle();
  FreeOneCommandBuffer(cmdBuff);
}

}  // namespace sktr
