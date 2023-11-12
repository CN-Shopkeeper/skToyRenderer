#include "material.hpp"

#include "sktr/core/context.hpp"

namespace sktr {

Material::Material() {
  createBuffer();
  set = DescriptorSetManager::GetInstance().AllocMaterialBufferSet();
  updateDescriptorSet();
}

Material::~Material() { buffer.reset(); }

void Material::createBuffer() {
  size_t size = sizeof(MaterialInfo);
  buffer.reset(new Buffer{size, vk::BufferUsageFlagBits::eUniformBuffer,
                          vk::MemoryPropertyFlagBits::eHostCoherent |
                              vk::MemoryPropertyFlagBits::eHostVisible});
}

void Material::updateDescriptorSet() {
  vk::WriteDescriptorSet writeInfo;

  // bind VP buffer
  vk::DescriptorBufferInfo bufferInfoVP;
  bufferInfoVP.setBuffer(buffer->buffer)
      .setOffset(0)
      .setRange(sizeof(MaterialInfo));

  writeInfo.setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setBufferInfo(bufferInfoVP)
      .setDstBinding(0)
      .setDstSet(set.set)
      .setDstArrayElement(0)
      .setDescriptorCount(1);

  Context::GetInstance().device.updateDescriptorSets(writeInfo, {});
}

}  // namespace sktr