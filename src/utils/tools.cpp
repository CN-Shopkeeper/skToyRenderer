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

vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates,
                               vk::ImageTiling tiling,
                               vk::FormatFeatureFlags features) {
  for (vk::Format format : candidates) {
    vk::FormatProperties props =
        Context::GetInstance().phyDevice.getFormatProperties(format);
    if (tiling == vk::ImageTiling::eLinear &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == vk::ImageTiling::eOptimal &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("failed to find supported format!");
}

vk::Format findDepthFormat() {
  return findSupportedFormat(
      {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
       vk::Format::eD24UnormS8Uint},
      vk::ImageTiling::eOptimal,
      vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}
}  // namespace sktr