#include "texture.hpp"

#include "context.hpp"
#include "system/buffer.hpp"

namespace sktr {

Texture::Texture(std::string_view filename) {
  auto surface = SDL_ConvertSurfaceFormat(IMG_Load(filename.data()),
                                          SDL_PIXELFORMAT_RGBA32, 0);
  if (!surface) {
    SDL_Log("load %s failed", filename);
  }
  auto w = surface->w;
  auto h = surface->h;

  init(surface->pixels, w, h);

  SDL_FreeSurface(surface);
}

Texture::Texture(void* data, uint32_t w, uint32_t h) { init(data, w, h); }

void Texture::init(void* data, uint32_t w, uint32_t h) {
  const uint32_t size = w * h * 4;

  std::unique_ptr<Buffer> buffer(
      new Buffer(size, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostCoherent |
                     vk::MemoryPropertyFlagBits::eHostVisible));
  memcpy(buffer->map, data, size);

  createImage(w, h);
  allocMemory();
  Context::GetInstance().device.bindImageMemory(image, memory, 0);

  transitionImageLayoutFromUndefine2Dst();
  transformData2Image(*buffer, w, h);
  transitionImageLayoutFromDst2Optimal();

  createImageView();

  set = DescriptorSetManager::GetInstance().AllocImageSet();
  updateDescriptorSet();
}

Texture::~Texture() {
  auto& device = Context::GetInstance().device;
  DescriptorSetManager::GetInstance().FreeImageSet(set);
  device.destroyImageView(view);
  device.freeMemory(memory);
  device.destroyImage(image);
}

void Texture::updateDescriptorSet() {
  vk::WriteDescriptorSet writer;
  vk::DescriptorImageInfo imageInfo;
  imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
      .setImageView(view)
      .setSampler(Context::GetInstance().sampler);
  writer.setImageInfo(imageInfo)
      .setDstBinding(0)
      .setDstArrayElement(0)
      .setDstSet(set.set)
      .setDescriptorCount(1)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
  Context::GetInstance().device.updateDescriptorSets(writer, {});
}

void Texture::createImage(uint32_t w, uint32_t h) {
  vk::ImageCreateInfo imageInfo;
  imageInfo.setImageType(vk::ImageType::e2D)
      .setArrayLayers(1)
      .setMipLevels(1)
      .setExtent({w, h, 1})
      .setFormat(vk::Format::eR8G8B8A8Srgb)
      // 像素存储方式，linear是按行存储，optimal的优化方式和gpu厂商相关，例如按块存储
      .setTiling(vk::ImageTiling::eOptimal)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setUsage(vk::ImageUsageFlagBits::eTransferDst |
                vk::ImageUsageFlagBits::eSampled)
      //   对其周围多少个点进行采样
      .setSamples(vk::SampleCountFlagBits::e1);
  image = Context::GetInstance().device.createImage(imageInfo);
}

void Texture::allocMemory() {
  auto& device = Context::GetInstance().device;
  vk::MemoryAllocateInfo allocateInfo;

  auto requirements = device.getImageMemoryRequirements(image);
  allocateInfo.setAllocationSize(requirements.size);

  auto index = QueryMemoryTypeIndex(requirements.memoryTypeBits,
                                    vk::MemoryPropertyFlagBits::eDeviceLocal);
  allocateInfo.setMemoryTypeIndex(index);

  memory = device.allocateMemory(allocateInfo);
}

void Texture::createImageView() {
  vk::ImageViewCreateInfo imageViewInfo;
  vk::ComponentMapping mapping;
  vk::ImageSubresourceRange range;
  range.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setBaseArrayLayer(0)
      .setLayerCount(1)
      .setLevelCount(1)
      .setBaseMipLevel(0);
  imageViewInfo.setImage(image)
      .setViewType(vk::ImageViewType::e2D)
      .setComponents(mapping)
      .setFormat(vk::Format::eR8G8B8A8Srgb)
      .setSubresourceRange(range);
  view = Context::GetInstance().device.createImageView(imageViewInfo);
}

void Texture::transitionImageLayoutFromUndefine2Dst() {
  Context::GetInstance().commandManager->ExecuteCmd(
      Context::GetInstance().graphicsQueue, [&](vk::CommandBuffer cmdBUff) {
        // 用于gpu同步，等待layout的转化
        vk::ImageMemoryBarrier barrier;
        vk::ImageSubresourceRange range;
        range.setLayerCount(1)
            .setBaseArrayLayer(0)
            .setLevelCount(1)
            .setBaseMipLevel(0)
            .setAspectMask(vk::ImageAspectFlagBits::eColor);
        barrier.setImage(image)
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            // 没有queueFamily的依赖关系
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            // 转换完成后的权限
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setSubresourceRange(range);
        cmdBUff.pipelineBarrier(
            // 在传输开始前就进行barrier
            vk::PipelineStageFlagBits::eTopOfPipe,
            // 直到数据传输时
            vk::PipelineStageFlagBits::eTransfer, {},
            // 使用裸内存，不绑定在buffer或image上
            {},
            // 需要对buffer进行等待
            nullptr,
            // 需要对image等待
            barrier);
      });
}

void Texture::transitionImageLayoutFromDst2Optimal() {
  Context::GetInstance().commandManager->ExecuteCmd(
      Context::GetInstance().graphicsQueue, [&](vk::CommandBuffer cmdBuf) {
        vk::ImageMemoryBarrier barrier;
        vk::ImageSubresourceRange range;
        range.setLayerCount(1)
            .setBaseArrayLayer(0)
            .setLevelCount(1)
            .setBaseMipLevel(0)
            .setAspectMask(vk::ImageAspectFlagBits::eColor);
        barrier.setImage(image)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setSrcAccessMask((vk::AccessFlagBits::eTransferWrite))
            .setDstAccessMask((vk::AccessFlagBits::eShaderRead))
            .setSubresourceRange(range);
        cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eFragmentShader, {},
                               {}, nullptr, barrier);
      });
}

void Texture::transformData2Image(Buffer& buffer, uint32_t w, uint32_t h) {
  Context::GetInstance().commandManager->ExecuteCmd(
      Context::GetInstance().graphicsQueue, [&](vk::CommandBuffer cmdBuff) {
        vk::BufferImageCopy region;
        vk::ImageSubresourceLayers subSource;
        subSource.setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setMipLevel(0)
            .setLayerCount(1);
        region
            // padding:方便存储，内存对齐
            // 0：告诉vulkan图像就是紧凑存储
            .setBufferImageHeight(0)
            // buffer中image的偏移量
            .setBufferOffset(0)
            .setImageOffset(0)
            .setImageExtent({w, h, 1})
            // 也是为了计算padding
            .setBufferRowLength(0)
            .setImageSubresource(subSource);
        cmdBuff.copyBufferToImage(buffer.buffer, image,
                                  vk::ImageLayout::eTransferDstOptimal, region);
      });
}

// TextureManager
std::unique_ptr<TextureManager> TextureManager::instance_ = nullptr;

Texture* TextureManager::Load(const std::string& filename) {
  datas_.push_back(std::unique_ptr<Texture>(new Texture{filename}));
  return datas_.back().get();
}

Texture* TextureManager::Create(void* data, uint32_t w, uint32_t h) {
  datas_.push_back(std::unique_ptr<Texture>(new Texture(data, w, h)));
  return datas_.back().get();
}

void TextureManager::Clear() { datas_.clear(); }

void TextureManager::Destroy(Texture* texture) {
  auto it = std::find_if(
      datas_.begin(), datas_.end(),
      [&](const std::unique_ptr<Texture>& t) { return t.get() == texture; });
  if (it != datas_.end()) {
    Context::GetInstance().device.waitIdle();
    datas_.erase(it);
    return;
  }
}
}  // namespace sktr