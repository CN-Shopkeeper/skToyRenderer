#include "swapchain.hpp"

#include "core/context.hpp"

namespace sktr {

Swapchain::Swapchain(int w, int h) {
  queryInfo(w, h);
  auto swapchainInfo = vk::SwapchainCreateInfoKHR();
  swapchainInfo
      .setClipped(true)
      // vuklan底层的图片都是存在数组里面，ArrayLayer是多层的数组
      .setImageArrayLayers(1)
      // 允许GPU往图像上绘制像素点
      .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
      // 颜色绘制到屏幕上时不融混
      .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
      .setSurface(Context::GetInstance().surface)
      .setImageColorSpace(info.format.colorSpace)
      .setImageFormat(info.format.format)
      .setImageExtent(info.imageExtent)
      .setMinImageCount(info.imageCount)
      .setPresentMode(info.present)
      .setPreTransform(info.transform);
  auto& queueIndices = Context::GetInstance().queueFamilyIndices;
  if (queueIndices.graphicsQueue.value() ==
      /* 是否使用set更好？*/
      queueIndices.presentQueue.value()) {
    swapchainInfo.setQueueFamilyIndices(queueIndices.graphicsQueue.value())
        .setImageSharingMode(vk::SharingMode::eExclusive);
  } else {
    std::array indices = {queueIndices.graphicsQueue.value(),
                          queueIndices.presentQueue.value()};
    swapchainInfo.setQueueFamilyIndices(indices).setImageSharingMode(
        vk::SharingMode::eConcurrent);
  }

  swapchain = Context::GetInstance().device.createSwapchainKHR(swapchainInfo);
  getImages();
  createImageViews();
}

Swapchain::~Swapchain() {
  for (auto& framebuffer : framebuffers) {
    Context::GetInstance().device.destroyFramebuffer(framebuffer);
  }
  // 手动销毁ImageViews
  for (auto& imageView : imageViews) {
    Context::GetInstance().device.destroyImageView(imageView);
  }
  Context::GetInstance().device.destroySwapchainKHR(swapchain);
}

void Swapchain::queryInfo(int w, int h) {
  auto& phyDevice = Context::GetInstance().phyDevice;
  auto& surface = Context::GetInstance().surface;
  // 支持的surface格式
  auto formats = phyDevice.getSurfaceFormatsKHR(surface);
  info.format = formats[0];
  for (const auto& format : formats) {
    if (format.format == vk::Format::eR8G8B8A8Srgb &&
        format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      info.format = format;
      break;
    }
  }

  auto capabilities = phyDevice.getSurfaceCapabilitiesKHR(surface);
  // 双缓冲
  info.imageCount = std::clamp<unsigned int>(2, capabilities.minImageCount,
                                             capabilities.maxImageCount);
  info.imageExtent.width = std::clamp<unsigned int>(
      w, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
  info.imageExtent.height =
      std::clamp<unsigned int>(h, capabilities.minImageExtent.height,
                               capabilities.maxImageExtent.height);
  info.transform = capabilities.currentTransform;
  auto presents = phyDevice.getSurfacePresentModesKHR(surface);
  info.present = vk::PresentModeKHR::eFifo;
  for (const auto& present : presents) {
    if (present == vk::PresentModeKHR::eMailbox) {
      info.present = present;
      break;
    }
  }
}

void Swapchain::getImages() {
  images = Context::GetInstance().device.getSwapchainImagesKHR(swapchain);
}

void Swapchain::createImageViews() {
  imageViews.resize(images.size());
  for (int i = 0; i < images.size(); i++) {
    vk::ImageViewCreateInfo imageViewInfo;
    // 创建映射，默认是identify
    vk::ComponentMapping mapping;
    vk::ImageSubresourceRange range;
    // 多级纹理，0为本身
    range.setBaseMipLevel(0)
        .setLevelCount(1)
        // 之前写的imageArrayLayer
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setAspectMask(vk::ImageAspectFlagBits::eColor);
    imageViewInfo
        .setImage(images[i])
        // 设置rgb的映射
        .setComponents(mapping)
        // 通过何种方式观察图像
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(info.format.format)
        .setSubresourceRange(range);
    imageViews[i] =
        Context::GetInstance().device.createImageView(imageViewInfo);
  }
}

void Swapchain::CreateFramebuffers(int w, int h) {
  framebuffers.resize(images.size());
  for (int i = 0; i < framebuffers.size(); i++) {
    auto& framebuffer = framebuffers[i];
    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.setAttachments(imageViews[i])
        .setWidth(w)
        .setHeight(h)
        .setRenderPass(Context::GetInstance().renderProcess->renderPass)
        .setLayers(1);
    framebuffers[i] =
        Context::GetInstance().device.createFramebuffer(framebufferInfo);
  }
}
}  // namespace sktr