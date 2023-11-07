#include "model.hpp"
#define TINYOBJLOADER_IMPLEMENTATION

#include <tiny_obj_loader.h>

#include "context.hpp"
namespace sktr {
Model::Model(const std::string name, const std::string modelPath,
             const std::string mtlPath, bool normalized)
    : name(name), modelMatrix(glm::identity<glm::mat4>()) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn;
  std::string err;

  const char* mtlPathC =
      mtlPath.empty() ? (const char*)__null : mtlPath.c_str();
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str(),
                        mtlPathC)) {
    throw std::runtime_error(warn + err);
  }
  std::cout << materials.size() << std::endl;

  std::unordered_map<Vertex, uint32_t> uniqueVertices{};

  float maxX = -std::numeric_limits<float>::max(),
        maxY = -std::numeric_limits<float>::max(),
        maxZ = -std::numeric_limits<float>::max(),
        minX = std::numeric_limits<float>::max(),
        minY = std::numeric_limits<float>::max(),
        minZ = std::numeric_limits<float>::max();

  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Vertex vertex{};
      vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};

      vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                       attrib.normals[3 * index.normal_index + 1],
                       attrib.normals[3 * index.normal_index + 2]};

      vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                         1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

      vertex.color = {1.0f, 1.0f, 1.0f};
      if (uniqueVertices.count(vertex) == 0) {
        maxX = std::max(maxX, vertex.pos.x);
        maxY = std::max(maxY, vertex.pos.y);
        maxZ = std::max(maxZ, vertex.pos.z);
        minX = std::min(minX, vertex.pos.x);
        minY = std::min(minY, vertex.pos.y);
        minZ = std::min(minZ, vertex.pos.z);
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }

      indices.push_back(uniqueVertices[vertex]);
    }
  }
  if (normalized) {
    auto scale_factor =
        2.0 / std::max(maxX - minX, std::max(maxY - minY, maxZ - minZ));
    for (auto& vertex : vertices) {
      vertex.pos.x = (vertex.pos.x - minX) * scale_factor - 1.0;
      vertex.pos.y = (vertex.pos.y - minY) * scale_factor - 1.0;
      vertex.pos.z = (vertex.pos.z - minZ) * scale_factor;
    }
  }

  createVertexBuffer();
  createIndicesBuffer();
}

void Model::createVertexBuffer() {
  auto size = sizeof(vertices[0]) * vertices.size();
  Buffer stagingBuffer = Buffer{size, vk::BufferUsageFlagBits::eTransferSrc,
                                vk::MemoryPropertyFlagBits::eHostVisible |
                                    vk::MemoryPropertyFlagBits::eHostCoherent};
  memcpy(stagingBuffer.map, vertices.data(), size);
  vertexBuffer.reset(new Buffer{size,
                                vk::BufferUsageFlagBits::eVertexBuffer |
                                    vk::BufferUsageFlagBits::eTransferDst,
                                vk::MemoryPropertyFlagBits::eDeviceLocal});
  auto& ctx = Context::GetInstance();
  ctx.commandManager->ExecuteCmd(
      ctx.graphicsQueue, [&](vk::CommandBuffer& cmdBuff) {
        vk::BufferCopy region;
        region.setSize(stagingBuffer.size).setSrcOffset(0).setDstOffset(0);
        cmdBuff.copyBuffer(stagingBuffer.buffer, vertexBuffer->buffer, region);
      });
}

void Model::createIndicesBuffer() {
  auto size = sizeof(indices[0]) * indices.size();
  Buffer stagingBuffer = Buffer{size, vk::BufferUsageFlagBits::eTransferSrc,
                                vk::MemoryPropertyFlagBits::eHostVisible |
                                    vk::MemoryPropertyFlagBits::eHostCoherent};
  memcpy(stagingBuffer.map, indices.data(), size);
  indicesBuffer.reset(new Buffer{size,
                                 vk::BufferUsageFlagBits::eTransferDst |
                                     vk::BufferUsageFlagBits::eIndexBuffer,
                                 vk::MemoryPropertyFlagBits::eDeviceLocal});
  auto& ctx = Context::GetInstance();
  ctx.commandManager->ExecuteCmd(
      ctx.graphicsQueue, [&](vk::CommandBuffer& cmdBuff) {
        vk::BufferCopy region;
        region.setSize(stagingBuffer.size).setSrcOffset(0).setDstOffset(0);
        cmdBuff.copyBuffer(stagingBuffer.buffer, indicesBuffer->buffer, region);
      });
}

}  // namespace sktr