#include "descriptor_manager.hpp"

#include "core/context.hpp"

namespace sktr {
std::unique_ptr<DescriptorSetManager> DescriptorSetManager::instance_ = nullptr;

DescriptorSetManager::DescriptorSetManager(uint32_t maxFlight)
    : maxFlight_(maxFlight) {
  vk::DescriptorPoolSize size;
  size.setType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(2 * maxFlight);
  vk::DescriptorPoolCreateInfo descriptorPoolInfo;
  descriptorPoolInfo.setMaxSets(maxFlight).setPoolSizes(size);
  auto pool =
      Context::GetInstance().device.createDescriptorPool(descriptorPoolInfo);
  bufferSetPool_.pool_ = pool;
  bufferSetPool_.remainNum_ = maxFlight;
}

DescriptorSetManager::~DescriptorSetManager() {
  auto& device = Context::GetInstance().device;

  device.destroyDescriptorPool(bufferSetPool_.pool_);
  for (auto pool : fulledImageSetPool_) {
    device.destroyDescriptorPool(pool.pool_);
  }
  for (auto pool : avalibleImageSetPool_) {
    device.destroyDescriptorPool(pool.pool_);
  }
}

void DescriptorSetManager::addImageSetPool() {
  constexpr uint32_t MaxSetNum = 10;
  vk::DescriptorPoolSize size;
  size.setType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(MaxSetNum);
  vk::DescriptorPoolCreateInfo createInfo;
  // ? 为什么我这里必须×2
  // ? 为什么都统一设置为MaxSetNum
  createInfo.setMaxSets(MaxSetNum).setPoolSizes(size).setFlags(
      // 可以单独销毁的描述符集，而非必须跟随pool一起销毁
      vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
  auto pool = Context::GetInstance().device.createDescriptorPool(createInfo);
  avalibleImageSetPool_.push_back({pool, MaxSetNum});
}

std::vector<DescriptorSetManager::SetInfo>
DescriptorSetManager::AllocBufferSets(uint32_t num) {
  // ? 这里的layouts前的数量不能去掉
  std::vector<vk::DescriptorSetLayout> layouts(
      maxFlight_, Shader::GetInstance().setLayouts[0]);
  vk::DescriptorSetAllocateInfo allocInfo;
  allocInfo.setDescriptorPool(bufferSetPool_.pool_)
      .setDescriptorSetCount(num)
      .setSetLayouts(layouts);
  auto sets = Context::GetInstance().device.allocateDescriptorSets(allocInfo);

  std::vector<SetInfo> result(num);

  for (int i = 0; i < num; i++) {
    result[i].set = sets[i];
    result[i].pool = bufferSetPool_.pool_;
  }

  return result;
}

DescriptorSetManager::SetInfo DescriptorSetManager::AllocImageSet() {
  // ? 这个函数我不是很懂
  auto& poolInfo = getAvaliableImagePoolInfo();
  std::vector<vk::DescriptorSetLayout> layouts(
      maxFlight_, Shader::GetInstance().setLayouts[1]);
  vk::DescriptorSetAllocateInfo allocInfo;
  allocInfo.setDescriptorPool(poolInfo.pool_)
      .setDescriptorSetCount(1)
      .setSetLayouts(layouts);
  auto sets = Context::GetInstance().device.allocateDescriptorSets(allocInfo);

  SetInfo result;
  result.pool = poolInfo.pool_;
  result.set = sets[0];

  // ? 是否公式有误？
  poolInfo.remainNum_ =
      std::max(0, static_cast<int>(poolInfo.remainNum_ - sets.size()));
  if (poolInfo.remainNum_ == 0) {
    fulledImageSetPool_.push_back(poolInfo);
    avalibleImageSetPool_.pop_back();
  }

  return result;
}

void DescriptorSetManager::FreeImageSet(const SetInfo& info) {
  auto it = std::find_if(
      fulledImageSetPool_.begin(), fulledImageSetPool_.end(),
      [&](const PoolInfo& poolInfo) { return poolInfo.pool_ == info.pool; });
  if (it != fulledImageSetPool_.end()) {
    it->remainNum_++;
    avalibleImageSetPool_.push_back(*it);
    fulledImageSetPool_.erase(it);
    return;
  }

  it = std::find_if(
      avalibleImageSetPool_.begin(), avalibleImageSetPool_.end(),
      [&](const PoolInfo& poolInfo) { return poolInfo.pool_ == info.pool; });
  if (it != avalibleImageSetPool_.end()) {
    it->remainNum_++;
    return;
  }
  // 如果都找不到，就说明这个setInfo已经失效
  Context::GetInstance().device.freeDescriptorSets(info.pool, info.set);
}

DescriptorSetManager::PoolInfo&
DescriptorSetManager::getAvaliableImagePoolInfo() {
  if (avalibleImageSetPool_.empty()) {
    addImageSetPool();
    return avalibleImageSetPool_.back();
  }
  return avalibleImageSetPool_.back();
}

}  // namespace sktr