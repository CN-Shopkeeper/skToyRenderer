#pragma once
#include "sktr/pch.hpp"
#include "sktr/system/buffer.hpp"
#include "sktr/system/descriptor_manager.hpp"

namespace sktr {

class ImageResource {
 public:
  vk::Image image;
  vk::DeviceMemory memory;
  vk::ImageView view;
  ~ImageResource();

  static ImageResource CreateColorResource(uint32_t w, uint32_t h,
                                           uint32_t mipLevels,
                                           vk::SampleCountFlagBits numSamples,
                                           vk::Format format,
                                           vk::ImageTiling tiling,
                                           vk::ImageUsageFlags usage,
                                           vk::MemoryPropertyFlags properties);
  static ImageResource CreateDepthResource(uint32_t w, uint32_t h,
                                           uint32_t mipLevels,
                                           vk::SampleCountFlagBits numSamples,
                                           vk::Format format,
                                           vk::ImageTiling tiling,
                                           vk::ImageUsageFlags usage,
                                           vk::MemoryPropertyFlags properties);

 protected:
  void createImage(uint32_t w, uint32_t h, uint32_t mipLevels,
                   vk::SampleCountFlagBits numSamples, vk::Format format,
                   vk::ImageTiling tiling, vk::ImageUsageFlags usage);
  void createImageView(vk::Format format, vk::ImageAspectFlags aspectFlags,
                       uint32_t mipLevels);
  void allocMemory(vk::MemoryPropertyFlags properties);
  void transitionImageLayout(vk::Format format, vk::ImageLayout oldLayout,
                             vk::ImageLayout newLayout, uint32_t mipLevels);
  bool hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint ||
           format == vk::Format::eD24UnormS8Uint;
  }
};

class TextureManager;

class Texture final : public ImageResource {
 public:
  friend class TextureManager;
  ~Texture();

  // 记录DescriptorSet信息，用于在一帧内绘制多个图片时，不需要重置描述符集
  DescriptorSetManager::SetInfo set;

 private:
  uint32_t mipLevels_;
  vk::Sampler sampler;

  Texture(std::string_view filename);
  Texture(void* data, uint32_t w, uint32_t h, uint32_t mipLevels,
          vk::SampleCountFlagBits numSamples, vk::Format format,
          vk::ImageTiling tiling, vk::ImageUsageFlags usage,
          vk::MemoryPropertyFlags properties);

  // 从undefined转化为dst，防止①无法操作；②性能损失
  void transitionImageLayoutFromUndefine2Dst();
  // CPU传到GPU
  void transformData2Image(Buffer&, uint32_t w, uint32_t h);
  // 接收数据的布局->shader只读的布局
  void transitionImageLayoutFromDst2Optimal();
  void updateDescriptorSet();
  void init(void* data, uint32_t w, uint32_t h, uint32_t mipLevels,
            vk::SampleCountFlagBits numSamples, vk::Format format,
            vk::ImageTiling tiling, vk::ImageUsageFlags usage,
            vk::MemoryPropertyFlags properties);
  void generateMipmaps(int32_t texWidth, int32_t texHeight);
  void createSampler();
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
  Texture* Create(void* data, uint32_t w, uint32_t h, uint32_t mipLevels,
                  vk::SampleCountFlagBits numSamples, vk::Format format,
                  vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                  vk::MemoryPropertyFlags properties);
  void Destroy(Texture*);
  void Clear();
  void updateDescriptorSet();

  Texture* CreateWhiteTexture();

 private:
  static std::unique_ptr<TextureManager> instance_;

  std::vector<std::unique_ptr<Texture>> datas_;
};
}  // namespace sktr