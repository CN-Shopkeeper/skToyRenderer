#include "shader.hpp"

#include "core/context.hpp"

namespace sktr {
std::unique_ptr<Shader> Shader::instance_ = nullptr;

void Shader::Init(const std::string &verteSource,
                  const std::string &fragSource) {
  instance_.reset(new Shader{verteSource, fragSource});
}

void Shader::Quit() { instance_.reset(); }

Shader &Shader::GetInstance() { return *instance_; }

Shader::Shader(const std::string &vertexSource, const std::string &fragSource) {
  vk::ShaderModuleCreateInfo shaderModuleInfo;
  // 读入二进制文件时，一般都是读成char，所以用这种方式而不用↓
  // shaderModuleInfo.setCode()
  shaderModuleInfo.codeSize = vertexSource.size();
  shaderModuleInfo.pCode = (uint32_t *)vertexSource.data();
  vertexModule =
      Context::GetInstance().device.createShaderModule(shaderModuleInfo);

  shaderModuleInfo.codeSize = fragSource.size();
  shaderModuleInfo.pCode = (uint32_t *)fragSource.data();
  fragmentModule =
      Context::GetInstance().device.createShaderModule(shaderModuleInfo);

  initStage();
  initDescriptorSetLayouts();
}

Shader::~Shader() {
  auto &device = Context::GetInstance().device;
  for (auto &layout : setLayouts) {
    device.destroyDescriptorSetLayout(layout);
  }
  setLayouts.clear();
  device.destroyShaderModule(vertexModule);
  device.destroyShaderModule(fragmentModule);
}

std::vector<vk::PipelineShaderStageCreateInfo> Shader::GetStages() {
  return stages_;
}

void Shader::initStage() {
  stages_.resize(2);
  stages_[0]
      // 当前着色器代码需要用在哪个着色器上
      .setStage(vk::ShaderStageFlagBits::eVertex)
      // 设置着色器代码
      .setModule(vertexModule)
      // 设置函数入口（着色器代码）
      .setPName("main");
  stages_[1]
      .setStage(vk::ShaderStageFlagBits::eFragment)
      .setModule(fragmentModule)
      .setPName("main");
}

void Shader::initDescriptorSetLayouts() {
  auto &device = Context::GetInstance().device;
  // createSetLayout, 对应.frag中layout的set，binding对应bingding
  vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
  // mvp and color
  std::vector<vk::DescriptorSetLayoutBinding> bindings(1);
  bindings[0]
      .setBinding(0)
      .setDescriptorCount(1)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setStageFlags(vk::ShaderStageFlagBits::eVertex);
  setLayoutInfo.setBindings(bindings);
  setLayouts.push_back(device.createDescriptorSetLayout(setLayoutInfo));

  // sampler
  bindings.resize(1);
  bindings[0]
      .setBinding(0)
      .setDescriptorCount(1)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setStageFlags(vk::ShaderStageFlagBits::eFragment);
  setLayoutInfo.setBindings(bindings);

  setLayouts.push_back(device.createDescriptorSetLayout(setLayoutInfo));
}

std::vector<vk::PushConstantRange> Shader::GetPushConstantRange() const {
  std::vector<vk::PushConstantRange> ranges(2);
  ranges[0]
      .setOffset(0)
      .setSize(sizeof(Mat4))
      .setStageFlags(vk::ShaderStageFlagBits::eVertex);
  ranges[1]
      .setOffset(sizeof(Mat4))
      .setSize(sizeof(Color))
      .setStageFlags(vk::ShaderStageFlagBits::eFragment);
  return ranges;
}

}  // namespace sktr