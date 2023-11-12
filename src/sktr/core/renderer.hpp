#pragma once
#include "model.hpp"
#include "sktr/pch.hpp"
#include "sktr/system/buffer.hpp"
#include "sktr/system/descriptor_manager.hpp"
#include "sktr/utils/math.hpp"
#include "texture.hpp"

namespace sktr {
class Renderer final {
 public:
  Renderer(int width, int height, int maxFlightCount = 2);
  ~Renderer();

  void SetProjection(float fov, float aspect, float near, float far);
  void SetView(const glm::vec3 eye, const glm::vec3 center, const glm::vec3 up);
  void SetLight(glm::vec3 lightPos, glm::float32 lightIntensity);
  void SetDrawColor(const Color& color);

  // 开启render pass 并绑定渲染管线
  bool StartRender();
  // 结束render pass 并提交命令
  void EndRender();

  // 可以绘制多个图片
  void DrawTexture(const Rect& rect, Texture& texture);
  void DrawLine(const Vec2& p1, const Vec2& p2);
  void DrawModel(const Model& model);

  void GetInstance();

 private:
  int maxFlightCount_;
  int curFrame_;
  uint32_t imageIndex_;

  ViewProjectMatrices vpMatrices_;
  LightInfo lightMatrices_;

  std::vector<vk::CommandBuffer> cmdBuffs_;

  std::vector<vk::Semaphore> imageAvaliableSems_;
  std::vector<vk::Semaphore> renderFinishSems_;
  std::vector<vk::Fence> fences_;

  // std::unique_ptr<Buffer> hostRectVertexBuffer_;
  // std::unique_ptr<Buffer> deviceRectVertexBuffer_;
  // std::unique_ptr<Buffer> hostRectIndicesBuffer_;
  // std::unique_ptr<Buffer> deviceRectIndicesBuffer_;
  // std::unique_ptr<Buffer> hostLineVertexBuffer_;
  // std::unique_ptr<Buffer> deviceLineVertexBuffer_;

  // std::vector<std::unique_ptr<Buffer>> hostColorBuffers_;
  // std::vector<std::unique_ptr<Buffer>> deviceColorBuffers_;
  std::vector<std::unique_ptr<Buffer>> uniformVPBuffers;
  std::vector<std::unique_ptr<Buffer>> uniformLightBuffers;

  std::vector<DescriptorSetManager::SetInfo> worldUniformDescriptorSets_;
  Texture* whiteTexture;
  Color drawColor_ = {1, 1, 1};

  void allocCmdBuffers();
  void createSemaphores();
  void createFences();

  void createBuffers();
  void createUniformBuffers();

  void bufferVPData();
  void bufferLightData();

  void initMats();

  void updateDescriptorSets();

  void copyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size,
                  size_t srcOffset, size_t dstOffset);

  void createWhiteTexture();
};

}  // namespace sktr