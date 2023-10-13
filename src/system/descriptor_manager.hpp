#pragma once

#include "pch.hpp"
#include "utils/singlton.hpp"
namespace sktr {

class DescriptorSetManager : public Singlton<DescriptorSetManager> {
 public:
  struct SetInfo {
    vk::DescriptorSet set;
    vk::DescriptorPool pool;
  };

  ~DescriptorSetManager();

  std::vector<SetInfo> AllocBufferSets(uint32_t num);

  SetInfo AllocImageSet();

  void FreeImageSet(const SetInfo&);

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
  void createBufferSetPool();
  PoolInfo& getAvaliableImagePoolInfo();

  uint32_t maxFlight_;
};

}  // namespace sktr