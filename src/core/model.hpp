#pragma once
#include "pch.hpp"
#include "system/buffer.hpp"
#include "texture.hpp"
#include "utils/common.hpp"

namespace sktr {

class Model final {
 public:
  std::string name;
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  glm ::mat4 modelMatrix;
  Texture* texture;

  std::unique_ptr<Buffer> vertexBuffer;
  std::unique_ptr<Buffer> indicesBuffer;

  Model(const std::string, const std::string, const std::string mtlPath = "");
  void BufferUniformData(int nowFlight);

  void SetModelM(glm::mat4 model) { modelMatrix = model; }

 private:
  void createVertexBuffer();
  void createIndicesBuffer();
};
}  // namespace sktr
