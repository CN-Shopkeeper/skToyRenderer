#pragma once

#include "sktr/pch.hpp"
#include "sktr/utils/singlton.hpp"

namespace sktr {

class Shader final : public Singlton<Shader> {
 public:
  vk::ShaderModule vertexModule;
  vk::ShaderModule fragmentModule;
  std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;

  /**
   * @brief
   * @note
   * @param  &verteSource:定点着色器的源代码
   * @param  &fragSource:片段着色器的源代码
   * @retval None
   */
  Shader(const std::string &verteSource, const std::string &fragSource);
  ~Shader();

  std::vector<vk::PipelineShaderStageCreateInfo> GetStages();

  std::vector<vk::PushConstantRange> GetPushConstantRange() const;

 private:
  std::vector<vk::PipelineShaderStageCreateInfo> stages_;

  void initStage();

  void initDescriptorSetLayouts();
};
}  // namespace sktr