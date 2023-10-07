#pragma once

#include "pch.hpp"

namespace sktr {

class Shader final {
 private:
  static std::unique_ptr<Shader> instance_;
  std::vector<vk::PipelineShaderStageCreateInfo> stages_;

  Shader(const std::string &verteSource, const std::string &fragSource);
  void initStage();

  void initDescriptorSetLayouts();

 public:
  /**
   * @brief
   * @note
   * @param  &verteSource:定点着色器的源代码
   * @param  &fragSource:片段着色器的源代码
   * @retval None
   */
  static void Init(const std::string &vertexSource,
                   const std::string &fragSource);
  static void Quit();
  static Shader &GetInstance();

  vk::ShaderModule vertexModule;
  vk::ShaderModule fragmentModule;
  std::vector<vk::DescriptorSetLayout> setLayouts;

  ~Shader();

  std::vector<vk::PipelineShaderStageCreateInfo> GetStages();

  std::vector<vk::PushConstantRange> GetPushConstantRange() const;
};
}  // namespace sktr