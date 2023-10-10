#include "tools.hpp"

#include "core/context.hpp"

namespace sktr {
std::string ReadWholeFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);

  if (!file.is_open()) {
    std::cout << "read " << filename << " failed" << std::endl;
    return std::string();
  }

  auto size = file.tellg();
  std::string content;
  content.resize(size);
  file.seekg(0);

  file.read(content.data(), content.size());
  file.close();

  return content;
}

uint32_t QueryMemoryTypeIndex(uint32_t memoryTypeBits,
                              vk::MemoryPropertyFlags propertyFlags) {
  std::optional<uint32_t> typeIndex;
  // 寻找内存类型
  auto properties = Context::GetInstance().phyDevice.getMemoryProperties();

  for (int i = 0; i < properties.memoryTypeCount; i++) {
    if ((1 << i) & memoryTypeBits &&
        properties.memoryTypes[i].propertyFlags & propertyFlags) {
      typeIndex = i;
      break;
    }
  }
  if (!typeIndex.has_value()) {
    std::cout << "MemoryInfo.index not find" << std::endl;
  }
  return typeIndex.value();
}
}  // namespace sktr