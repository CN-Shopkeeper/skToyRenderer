#pragma once
#include "pch.hpp"
#include "system/buffer.hpp"
#include "system/descriptor_manager.hpp"
#include "texture.hpp"
#include "utils/math.hpp"

namespace sktr {
struct Matrices {
  glm::mat4 project;
  glm::mat4 view;
};
class Renderer final {
 public:
  Renderer(int width, int height, int maxFlightCount = 2);
  ~Renderer();

  void SetProjection(int left, int right, int bottom, int top, int near,
                     int far);
  void SetDrawColor(const Color& color);

  // 开启render pass 并绑定渲染管线
  void StartRender();
  // 结束render pass 并提交命令
  void EndRender();

  // 可以绘制多个图片
  void DrawTexture(const Rect& rect, Texture& texture);
  void DrawLine(const Vec2& p1, const Vec2& p2);

  void GetInstance();

 private:
  int maxFlightCount_;
  int curFrame_;
  uint32_t imageIndex_;

  Matrices matrices;

  std::vector<vk::CommandBuffer> cmdBuffs_;

  std::vector<vk::Semaphore> imageAvaliableSems_;
  std::vector<vk::Semaphore> renderFinishSems_;
  std::vector<vk::Fence> fences_;

  std::unique_ptr<Buffer> hostRectVertexBuffer_;
  std::unique_ptr<Buffer> deviceRectVertexBuffer_;
  std::unique_ptr<Buffer> hostRectIndicesBuffer_;
  std::unique_ptr<Buffer> deviceRectIndicesBuffer_;
  std::unique_ptr<Buffer> hostLineVertexBuffer_;
  std::unique_ptr<Buffer> deviceLineVertexBuffer_;

  // std::vector<std::unique_ptr<Buffer>> hostColorBuffers_;
  // std::vector<std::unique_ptr<Buffer>> deviceColorBuffers_;
  std::vector<std::unique_ptr<Buffer>> hostUniformBuffers_;
  std::vector<std::unique_ptr<Buffer>> deviceUniformBuffers_;

  std::vector<DescriptorSetManager::SetInfo> descriptorSets_;
  Texture* whiteTexture;
  Color drawColor_ = {1, 1, 1};

  void allocCmdBuffers();
  void createSemaphores();
  void createFences();

  void createBuffers();
  void createVertexBuffer();
  void createIndicesBuffer();
  void createUniformBuffers();

  void bufferRectData();
  void bufferRectVertexData();
  void bufferRectIndicesData();

  void bufferLineData(const Vec2& p1, const Vec2& p2);

  void bufferMVPData();

  void initMats();

  void updateDescriptorSets();

  void copyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size,
                  size_t srcOffset, size_t dstOffset);

  void createWhiteTexture();
};

}  // namespace sktr