#pragma once
#include "pch.hpp"
#include "system/buffer.hpp"
#include "utils/common.hpp"

namespace sktr {

class Model final {
 public:
  std::string name;
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  MVPMatrix mvpMatrix;

  std::unique_ptr<Buffer> vertexBuffer;
  std::unique_ptr<Buffer> indicesBuffer;
  std::vector<std::unique_ptr<Buffer>> uniformBuffers;

  Model(const std::string, const std::string, const std::string mtlPath = "");
  void BufferUniformData(int nowFlight);

  void SetModelM(glm::mat4 model) { mvpMatrix.model = model; }
  void SetViewlM(glm::mat4 view) { mvpMatrix.view = view; }
  void SetProjM(glm::mat4 proj) { mvpMatrix.proj = proj; }

 private:
  void createVertexBuffer();
  void createIndicesBuffer();
  void createUniformBuffers(int maxFlight);
};
}  // namespace sktr
