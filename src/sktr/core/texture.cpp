#include "texture.hpp"

#include "context.hpp"
#include "sktr/system/buffer.hpp"

namespace sktr {

void ImageResource::createImage(uint32_t w, uint32_t h, uint32_t mipLevels,
                                vk::SampleCountFlagBits numSamples,
                                vk::Format format, vk::ImageTiling tiling,
                                vk::ImageUsageFlags usage) {
  vk::ImageCreateInfo imageInfo;
  imageInfo.setImageType(vk::ImageType::e2D)
      .setArrayLayers(1)
      .setMipLevels(mipLevels)
      .setExtent({w, h, 1})
      .setFormat(format)
      // 像素存储方式，linear是按行存储，optimal的优化方式和gpu厂商相关，例如按块存储
      .setTiling(tiling)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setUsage(usage)
      //   对其周围多少个点进行采样
      .setSamples(numSamples);
  image = Context::GetInstance().device.createImage(imageInfo);
}

void ImageResource::allocMemory(vk::MemoryPropertyFlags properties) {
  auto& device = Context::GetInstance().device;
  vk::MemoryAllocateInfo allocateInfo;

  auto requirements = device.getImageMemoryRequirements(image);
  allocateInfo.setAllocationSize(requirements.size);

  auto index = QueryMemoryTypeIndex(requirements.memoryTypeBits, properties);
  allocateInfo.setMemoryTypeIndex(index);

  memory = device.allocateMemory(allocateInfo);
}

void ImageResource::createImageView(vk::Format format,
                                    vk::ImageAspectFlags aspectFlags,
                                    uint32_t mipLevels) {
  vk::ImageViewCreateInfo imageViewInfo;
  vk::ComponentMapping mapping;
  vk::ImageSubresourceRange range;
  range.setAspectMask(aspectFlags)
      .setBaseArrayLayer(0)
      .setLayerCount(1)
      .setLevelCount(mipLevels)
      .setBaseMipLevel(0);
  imageViewInfo.setImage(image)
      .setViewType(vk::ImageViewType::e2D)
      .setComponents(mapping)
      .setFormat(format)
      .setSubresourceRange(range);
  view = Context::GetInstance().device.createImageView(imageViewInfo);
}

ImageResource ImageResource::CreateColorResource(
    uint32_t w, uint32_t h, uint32_t mipLevels,
    vk::SampleCountFlagBits numSamples, vk::Format format,
    vk::ImageTiling tiling, vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties) {
  ImageResource resource;
  resource.createImage(w, h, mipLevels, numSamples, format, tiling, usage);
  resource.allocMemory(properties);
  Context::GetInstance().device.bindImageMemory(resource.image, resource.memory,
                                                0);
  resource.createImageView(format, vk::ImageAspectFlagBits::eColor, mipLevels);
  return resource;
}

ImageResource ImageResource::CreateDepthResource(
    uint32_t w, uint32_t h, uint32_t mipLevels,
    vk::SampleCountFlagBits numSamples, vk::Format format,
    vk::ImageTiling tiling, vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties) {
  ImageResource resource;
  resource.createImage(w, h, mipLevels, numSamples, format, tiling, usage);
  resource.allocMemory(properties);
  Context::GetInstance().device.bindImageMemory(resource.image, resource.memory,
                                                0);
  resource.createImageView(format, vk::ImageAspectFlagBits::eDepth, mipLevels);
  resource.transitionImageLayout(
      format, vk::ImageLayout::eUndefined,
      vk::ImageLayout::eDepthStencilAttachmentOptimal, mipLevels);
  return resource;
}

void ImageResource::transitionImageLayout(vk::Format format,
                                          vk::ImageLayout oldLayout,
                                          vk::ImageLayout newLayout,
                                          uint32_t mipLevels) {
  Context::GetInstance().commandManager->ExecuteCmd(
      Context::GetInstance().graphicsQueue, [&](vk::CommandBuffer cmdBUff) {
        // 用于gpu同步，等待layout的转化
        vk::ImageMemoryBarrier barrier;
        vk::ImageSubresourceRange range;
        range.setLayerCount(1)
            .setBaseArrayLayer(0)
            .setLevelCount(mipLevels)
            .setBaseMipLevel(0);
        barrier.setOldLayout(oldLayout);
        barrier.setNewLayout(newLayout);
        if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
          range.aspectMask = vk::ImageAspectFlagBits::eDepth;
          if (hasStencilComponent(format)) {
            range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
          }
        } else {
          range.setAspectMask(vk::ImageAspectFlagBits::eColor);
        }

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;

        // 转换完成后的权限
        if (oldLayout == vk::ImageLayout::eUndefined &&
            newLayout == vk::ImageLayout::eTransferDstOptimal) {
          barrier.srcAccessMask = vk::AccessFlagBits::eNone;
          barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

          // 在传输开始前就进行barrier
          sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
          // 直到数据传输时
          destinationStage = vk::PipelineStageFlagBits::eTransfer;
        } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
                   newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
          barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
          barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

          sourceStage = vk::PipelineStageFlagBits::eTransfer;
          destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        } else if (oldLayout == vk::ImageLayout::eUndefined &&
                   newLayout ==
                       vk::ImageLayout::eDepthStencilAttachmentOptimal) {
          barrier.srcAccessMask = vk::AccessFlagBits::eNone;
          barrier.dstAccessMask =
              vk::AccessFlagBits::eDepthStencilAttachmentRead |
              vk::AccessFlagBits::eDepthStencilAttachmentWrite;

          sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
          destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        } else {
          throw std::invalid_argument("unsupported layout transition!");
        }

        barrier
            .setImage(image)
            // 没有queueFamily的依赖关系
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setSubresourceRange(range);
        cmdBUff.pipelineBarrier(sourceStage, destinationStage, {},
                                // 使用裸内存，不绑定在buffer或image上
                                {},
                                // 需要对buffer进行等待
                                nullptr,
                                // 需要对image等待
                                barrier);
      });
}

ImageResource::~ImageResource() {
  auto& device = Context::GetInstance().device;
  device.destroyImageView(view);
  device.freeMemory(memory);
  device.destroyImage(image);
}

Texture::Texture(std::string_view filename) {
  auto surface = SDL_ConvertSurfaceFormat(IMG_Load(filename.data()),
                                          SDL_PIXELFORMAT_RGBA32, 0);
  if (!surface) {
    SDL_Log("load %s failed", filename);
  }
  auto w = surface->w;
  auto h = surface->h;
  mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(w, h)))) + 1;

  init(surface->pixels, w, h, mipLevels_, vk::SampleCountFlagBits::e1,
       vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
       vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled |
           vk::ImageUsageFlagBits::eTransferSrc,
       vk::MemoryPropertyFlagBits::eDeviceLocal);

  SDL_FreeSurface(surface);
}

Texture::Texture(void* data, uint32_t w, uint32_t h, uint32_t mipLevels,
                 vk::SampleCountFlagBits numSamples, vk::Format format,
                 vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                 vk::MemoryPropertyFlags properties) {
  init(data, w, h, mipLevels, numSamples, format, tiling, usage, properties);
}

void Texture::init(void* data, uint32_t w, uint32_t h, uint32_t mipLevels,
                   vk::SampleCountFlagBits numSamples, vk::Format format,
                   vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                   vk::MemoryPropertyFlags properties) {
  const uint32_t size = w * h * 4;

  std::unique_ptr<Buffer> buffer(
      new Buffer(size, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostCoherent |
                     vk::MemoryPropertyFlagBits::eHostVisible));
  memcpy(buffer->map, data, size);

  createImage(w, h, mipLevels, numSamples, format, tiling, usage);
  allocMemory(properties);
  Context::GetInstance().device.bindImageMemory(image, memory, 0);

  transitionImageLayoutFromUndefine2Dst();
  transformData2Image(*buffer, w, h);
  transitionImageLayoutFromDst2Optimal();

  generateMipmaps(w, h);

  createImageView(format, vk::ImageAspectFlagBits::eColor, mipLevels_);

  set = DescriptorSetManager::GetInstance().AllocImageSet();
  updateDescriptorSet();
}

Texture::~Texture() {
  auto& device = Context::GetInstance().device;
  DescriptorSetManager::GetInstance().FreeImageSet(set);
  ImageResource::~ImageResource();
}

void Texture::updateDescriptorSet() {
  vk::WriteDescriptorSet writer;
  vk::DescriptorImageInfo imageInfo;
  imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
      .setImageView(view)
      .setSampler(Context::GetInstance().sampler.sampler);
  writer.setImageInfo(imageInfo)
      .setDstBinding(0)
      .setDstArrayElement(0)
      .setDstSet(set.set)
      .setDescriptorCount(1)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
  Context::GetInstance().device.updateDescriptorSets(writer, {});
}

void Texture::transitionImageLayoutFromUndefine2Dst() {
  transitionImageLayout(vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal, 1);
}

void Texture::transitionImageLayoutFromDst2Optimal() {
  transitionImageLayout(vk::Format::eR8G8B8A8Srgb,
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eShaderReadOnlyOptimal, 1);
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

void Texture::generateMipmaps(int32_t texWidth, int32_t texHeight) {
  vk::FormatProperties formatProperties =
      Context::GetInstance().phyDevice.getFormatProperties(
          vk::Format::eR8G8B8A8Srgb);
  if (formatProperties.optimalTilingFeatures &
      vk::FormatFeatureFlagBits::eSampledImageFilterLinear) {
    throw std::runtime_error(
        "texture image format does not support linear blitting!");
  }

  Context::GetInstance().commandManager->ExecuteCmd(
      Context::GetInstance().graphicsQueue, [&](vk::CommandBuffer cmdBuff) {
        // 用于gpu同步，等待layout的转化
        vk::ImageMemoryBarrier barrier;
        vk::ImageSubresourceRange range;
        range.setLayerCount(1)
            .setBaseArrayLayer(0)
            .setLevelCount(1)
            .setBaseMipLevel(0);
        barrier
            .setImage(image)
            // 没有queueFamily的依赖关系
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels_; i++) {
          range.setBaseMipLevel(i - 1);
          barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
              .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
              .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
              .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
              .setSubresourceRange(range);

          cmdBuff.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                  vk::PipelineStageFlagBits::eTransfer, {},
                                  // 使用裸内存，不绑定在buffer或image上
                                  {},
                                  // 需要对buffer进行等待
                                  nullptr,
                                  // 需要对image等待
                                  barrier);

          vk::ImageBlit bilt{};
          vk::ImageSubresourceLayers srcLayer{};
          vk::ImageSubresourceLayers dstLayer{};
          srcLayer.setAspectMask(vk::ImageAspectFlagBits::eColor)
              .setMipLevel(i - 1)
              .setBaseArrayLayer(0)
              .setLayerCount(1);
          dstLayer.setAspectMask(vk::ImageAspectFlagBits::eColor)
              .setMipLevel(i)
              .setBaseArrayLayer(0)
              .setLayerCount(1);
          bilt.setSrcOffsets(
                  {vk::Offset3D{0, 0, 0}, vk::Offset3D{mipWidth, mipHeight, 1}})
              .setSrcSubresource(srcLayer)
              .setDstOffsets(
                  {vk::Offset3D{0, 0, 0},
                   vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1,
                                mipHeight > 1 ? mipHeight / 2 : 1, 1}})
              .setDstSubresource(dstLayer);

          cmdBuff.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
                            vk::ImageLayout::eTransferDstOptimal, bilt,
                            vk::Filter::eLinear);
          if (mipWidth > 1) mipWidth /= 2;
          if (mipHeight > 1) mipHeight /= 2;
        }
        range.setBaseMipLevel(mipLevels_);
        barrier.setSubresourceRange(range)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        cmdBuff.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                vk::PipelineStageFlagBits::eFragmentShader, {},
                                // 使用裸内存，不绑定在buffer或image上
                                {},
                                // 需要对buffer进行等待
                                nullptr,
                                // 需要对image等待
                                barrier);
      });
}

void Texture::createSampler() {
  vk::SamplerCreateInfo samplerInfo{};
  samplerInfo.setMagFilter(vk::Filter::eLinear)
      .setMinFilter(vk::Filter::eLinear)
      .setAddressModeU(vk::SamplerAddressMode::eRepeat)
      .setAddressModeV(vk::SamplerAddressMode::eRepeat)
      .setAddressModeW(vk::SamplerAddressMode::eRepeat);

  vk::PhysicalDeviceProperties properties =
      Context::GetInstance().phyDevice.getProperties();

  samplerInfo.setAnisotropyEnable(vk::True)
      .setMaxAnisotropy(properties.limits.maxSamplerAnisotropy)
      .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
      .setUnnormalizedCoordinates(vk::False)
      .setCompareEnable(vk::False)
      .setCompareOp(vk::CompareOp::eAlways)
      .setMipmapMode(vk::SamplerMipmapMode::eLinear)
      .setMipLodBias(0.f)
      .setMinLod(0.f)
      .setMaxLod(static_cast<float>(mipLevels_));

  sampler = Context::GetInstance().device.createSampler(samplerInfo);
}

// TextureManager
std::unique_ptr<TextureManager> TextureManager::instance_ = nullptr;

Texture* TextureManager::Load(const std::string& filename) {
  datas_.push_back(std::unique_ptr<Texture>(new Texture{filename}));
  return datas_.back().get();
}

Texture* TextureManager::Create(void* data, uint32_t w, uint32_t h,
                                uint32_t mipLevels,
                                vk::SampleCountFlagBits numSamples,
                                vk::Format format, vk::ImageTiling tiling,
                                vk::ImageUsageFlags usage,
                                vk::MemoryPropertyFlags properties) {
  datas_.push_back(std::unique_ptr<Texture>(new Texture(
      data, w, h, mipLevels, numSamples, format, tiling, usage, properties)));
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