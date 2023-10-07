#pragma once
#include "pch.hpp"
#include "system/buffer.hpp"
#include "system/descriptor_manager.hpp"

namespace sktr {

class TextureManager;

class Texture final {
 public:
  friend class TextureManager;
  ~Texture();

  vk::Image image;
  vk::DeviceMemory memory;
  vk::ImageView view;

  // 记录DescriptorSet信息，用于在一帧内绘制多个图片时，不需要重置描述符集
  DescriptorSetManager::SetInfo set;

 private:
  Texture(std::string_view filename);
  Texture(void* data, uint32_t w, uint32_t h);

  void createImage(uint32_t w, uint32_t h);
  void createImageView();
  void allocMemory();
  // 从undefined转化为dst，防止①无法操作；②性能损失
  void transitionImageLayoutFromUndefine2Dst();
  // CPU传到GPU
  void transformData2Image(Buffer&, uint32_t w, uint32_t h);
  // 接收数据的布局->shader只读的布局
  void transitionImageLayoutFromDst2Optimal();
  void updateDescriptorSet();

  void init(void* data, uint32_t w, uint32_t h);
};

class TextureManager final {
 public:
  static TextureManager& GetInstance() {
    if (!instance_) {
      instance_.reset(new TextureManager);
    }
    return *instance_;
  }

  Texture* Load(const std::string& filename);

  // * data must be a RGBA8888 format data
  Texture* Create(void* data, uint32_t w, uint32_t h);
  void Destroy(Texture*);
  void Clear();
  void updateDescriptorSet();

 private:
  static std::unique_ptr<TextureManager> instance_;

  std::vector<std::unique_ptr<Texture>> datas_;
};
}  // namespace sktr