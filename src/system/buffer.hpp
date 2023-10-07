#pragma once

#include "pch.hpp"
#include "utils/tools.hpp"

namespace sktr {
class Buffer final {
 private:
  struct MemoryInfo final {
    size_t size;
    std::optional<uint32_t> index = std::nullopt;
  };
  void createBuffer(size_t size, vk::BufferUsageFlags usage);
  void allocateMemory(MemoryInfo info);
  void bindingMem2Buf();

  MemoryInfo queryBufferMemoryInfo(vk::MemoryPropertyFlags);

 public:
  vk::Buffer buffer;
  vk::DeviceMemory memory;
  void* map;
  size_t size;

  Buffer(size_t size, vk::BufferUsageFlags usage,
         vk::MemoryPropertyFlags propertyFlags);
  ~Buffer();
};
}  // namespace sktr