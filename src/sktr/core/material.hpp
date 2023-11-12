#pragma once

#include "sktr/pch.hpp"
#include "sktr/system/buffer.hpp"
#include "sktr/system/descriptor_manager.hpp"
#include "sktr/utils/common.hpp"

namespace sktr {
class Material {
 public:
  Material();
  ~Material();
  DescriptorSetManager::SetInfo set;
  std::unique_ptr<Buffer> buffer;

 private:
  void createBuffer();
  void updateDescriptorSet();
};
}  // namespace sktr