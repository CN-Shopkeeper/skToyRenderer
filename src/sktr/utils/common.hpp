#pragma once
#include "sktr/pch.hpp"

namespace sktr {

// 不同硬件的命令队列不一样，能做的事情也不一样
struct QueueFamilyIndices {
  // 家族队列可能不存在

  // 支持图像操作的队列
  std::optional<uint32_t> graphicsQueue;
  // 支持surface的队列
  std::optional<uint32_t> presentQueue;

  operator bool() const {
    return graphicsQueue.has_value() && presentQueue.has_value();
  }
};

struct SwapChainSupportDetails {
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> presentModes;
};

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;
  glm::vec2 texCoord;

  static vk::VertexInputBindingDescription GetBindingDescriptions() {
    vk::VertexInputBindingDescription bindingDescription{};

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;
    return bindingDescription;
  }

  static std::array<vk::VertexInputAttributeDescription, 4>
  GetAttributeDescriptions() {
    std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[2].offset = offsetof(Vertex, normal);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = vk::Format::eR32G32Sfloat;
    attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
  }

  bool operator==(const Vertex& other) const {
    return pos == other.pos && color == other.color &&
           texCoord == other.texCoord;
  }
};

struct ViewProjectMatrices {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct LightInfo {
  alignas(16) glm::vec3 cameraPosition;
  alignas(16) glm::vec3 position;
  alignas(4) glm::float32 intensity;
};

struct MaterialInfo {
  alignas(16) glm::vec3 diffuse = {0.800000, 0.800000, 0.800000};
  alignas(16) glm::vec3 specular{0.500000, 0.500000, 0.500000};
};
}  // namespace sktr

namespace std {
template <>
struct hash<sktr::Vertex> {
  size_t operator()(sktr::Vertex const& vertex) const {
    return ((hash<glm::vec3>()(vertex.pos) ^
             (hash<glm::vec3>()(vertex.color) << 1)) >>
            1) ^
           (hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};
}  // namespace std
