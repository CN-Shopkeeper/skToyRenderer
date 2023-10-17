#include "buffer.hpp"

#include "sktr/core/context.hpp"

namespace sktr {

Buffer::Buffer(size_t size, vk::BufferUsageFlags usage,
               vk::MemoryPropertyFlags propertyFlags)
    : size(size) {
  createBuffer(size, usage);
  auto info = queryBufferMemoryInfo(propertyFlags);
  allocateMemory(info);
  bindingMem2Buf();

  if (propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) {
    map = Context::GetInstance().device.mapMemory(memory, 0, size);
  } else {
    map = nullptr;
  }
}
Buffer::~Buffer() {
  auto& device = Context::GetInstance().device;
  if (map) {
    device.unmapMemory(memory);
  }
  device.freeMemory(memory);
  device.destroyBuffer(buffer);
}

void Buffer::createBuffer(size_t size, vk::BufferUsageFlags usage) {
  vk::BufferCreateInfo bufferInfo;
  bufferInfo.setSize(size).setUsage(usage).setSharingMode(
      vk::SharingMode::eExclusive);

  buffer = Context::GetInstance().device.createBuffer(bufferInfo);
}

void Buffer::allocateMemory(MemoryInfo info) {
  vk::MemoryAllocateInfo allocInfo;
  allocInfo.setMemoryTypeIndex(info.index.value()).setAllocationSize(info.size);
  memory = Context::GetInstance().device.allocateMemory(allocInfo);
}

void Buffer::bindingMem2Buf() {
  Context::GetInstance().device.bindBufferMemory(buffer, memory, 0);
}

Buffer::MemoryInfo Buffer::queryBufferMemoryInfo(
    vk::MemoryPropertyFlags propertyFlags) {
  MemoryInfo info;
  auto requirements =
      Context::GetInstance().device.getBufferMemoryRequirements(buffer);
  // * 不能直接用buffer的size，因为可能存在内存对齐的问题
  info.size = requirements.size;

  info.index = QueryMemoryTypeIndex(requirements.memoryTypeBits, propertyFlags);
  return info;
}

}  // namespace sktr