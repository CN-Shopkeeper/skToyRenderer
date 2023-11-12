#pragma once
#include "material.hpp"
#include "sktr/pch.hpp"
#include "sktr/system/buffer.hpp"
#include "sktr/utils/common.hpp"
#include "texture.hpp"

namespace sktr {

class Model final {
 public:
  std::string name;
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::vector<MaterialInfo> materialInfos;
  glm ::mat4 modelMatrix;
  Texture* texture;
  std::unique_ptr<Material> material;

  std::unique_ptr<Buffer> vertexBuffer;
  std::unique_ptr<Buffer> indicesBuffer;

  Model(const std::string name, const std::string modelPath,
        const std::string mtlPath = "", bool normalized = false);

  void SetModelM(glm::mat4 model) { modelMatrix = model; }

  // todo: set texture

 private:
  void createVertexBuffer();
  void createIndicesBuffer();
};
}  // namespace sktr
