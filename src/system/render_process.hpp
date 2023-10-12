#pragma once
#include "pch.hpp"
namespace sktr {

class RenderProcess final {
 public:
  // 管线只负责渲染的具体的步骤，不关心要渲染什么
  vk::Pipeline graphicsPipelineWithTriangleTopology;
  vk::Pipeline graphicsPipelineWithLineTopology;

  // 传递数据（例如Uniform）在shader中的布局
  vk::PipelineLayout pipelineLayout;
  vk::RenderPass renderPass;

  RenderProcess(int w, int h);
  ~RenderProcess();

 private:
  vk::PipelineCache pipelineCache_ = nullptr;
  vk::PipelineCache createPipelineCache();

  void initPipeline(int width, int height);
  vk::Pipeline createPipeline(int width, int height,
                              vk::PrimitiveTopology topology);
  void initPipelineLayout();
  void initRenderPass();
};

}  // namespace sktr