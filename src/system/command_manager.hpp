#pragma once

#include "pch.hpp"

namespace sktr {
class CommandManager final {
 private:
  vk::CommandPool pool_;

  vk::CommandPool createCommandPool();

 public:
  CommandManager();
  ~CommandManager();

  vk::CommandBuffer CreateOneCommandBuffer();
  std::vector<vk::CommandBuffer> CreateCommandBuffers(std::uint32_t count);
  void ResetCmds();

  void FreeCommandBuffers(std::vector<vk::CommandBuffer>& cmdBuffs);

  void FreeOneCommandBuffer(vk::CommandBuffer& cmdBuff);

  using RecordCmdFunc = std::function<void(vk::CommandBuffer&)>;
  void ExecuteCmd(vk::Queue, RecordCmdFunc);
};
}  // namespace sktr