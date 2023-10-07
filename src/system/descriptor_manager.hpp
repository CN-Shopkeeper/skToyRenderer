#pragma once

#include "pch.hpp"

namespace sktr {

class DescriptorSetManager final {
 private:
  struct PoolInfo {
    vk::DescriptorPool pool_;
    uint32_t remainNum_;
  };

  DescriptorSetManager(uint32_t maxFlight);

  PoolInfo bufferSetPool_;

  std::vector<PoolInfo> fulledImageSetPool_;
  std::vector<PoolInfo> avalibleImageSetPool_;

  void addImageSetPool();
  PoolInfo& getAvaliableImagePoolInfo();

  uint32_t maxFlight_;
  static std::unique_ptr<DescriptorSetManager> instance_;

 public:
  struct SetInfo {
    vk::DescriptorSet set;
    vk::DescriptorPool pool;
  };

  static void Init(uint32_t maxFlight) {
    instance_.reset(new DescriptorSetManager(maxFlight));
  }

  static void Quit() { instance_.reset(); }

  static DescriptorSetManager& GetInstance() { return *instance_; }

  ~DescriptorSetManager();

  std::vector<SetInfo> AllocBufferSets(uint32_t num);

  SetInfo AllocImageSet();

  void FreeImageSet(const SetInfo&);
};

}  // namespace sktr