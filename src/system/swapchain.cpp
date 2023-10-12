#include "swapchain.hpp"

#include "core/context.hpp"

namespace sktr {

Swapchain::Swapchain(int w, int h) {
  queryInfo(w, h);
  auto swapchainInfo = vk::SwapchainCreateInfoKHR{};
  swapchainInfo
      .setClipped(true)
      // vuklan底层的图片都是存在数组里面，ArrayLayer是多层的数组
      .setImageArrayLayers(1)
      // 允许GPU往图像上绘制像素点
      .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
      // 颜色绘制到屏幕上时不融混
      .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
      .setSurface(Context::GetInstance().surface)
      .setImageColorSpace(info.surfaceFormat.colorSpace)
      .setImageFormat(info.surfaceFormat.format)
      .setImageExtent(info.imageExtent)
      .setMinImageCount(info.imageCount)
      .setPresentMode(info.present)
      .setPreTransform(info.transform)
      .setOldSwapchain(nullptr);
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

  images = Context::GetInstance().device.getSwapchainImagesKHR(swapchain);

  createImageViews();

  createImageResource(w, h);
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
  auto& ctx = Context::GetInstance();
  auto& phyDevice = ctx.phyDevice;
  auto& surface = ctx.surface;

  // 支持的surface格式
  SwapChainSupportDetails details = ctx.QuerySwapChainSupport(phyDevice);
  info.surfaceFormat = chooseSwapSurfaceFormat(details.formats);

  // 多缓冲，多少张图像可以在swapchain中渲染
  info.imageCount = std::clamp<unsigned int>(
      details.capabilities.minImageCount + 1,
      details.capabilities.minImageCount, details.capabilities.maxImageCount);

  info.imageExtent = chooseSwapExtent(details.capabilities);

  info.transform = details.capabilities.currentTransform;

  info.present = chooseSwapPresentMode(details.presentModes);
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
        .setFormat(info.surfaceFormat.format)
        .setSubresourceRange(range);
    imageViews[i] =
        Context::GetInstance().device.createImageView(imageViewInfo);
  }
}

void Swapchain::createImageResource(int w, int h) {
  auto& msaa = Context::GetInstance().sampler.msaaSamples;
  colorResource = ImageResource::CreateColorResource(
      w, h, 1, msaa, info.surfaceFormat.format, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eTransientAttachment |
          vk::ImageUsageFlagBits::eColorAttachment,
      vk::MemoryPropertyFlagBits::eDeviceLocal);
  depthResource = ImageResource::CreateDepthResource(
      w, h, 1, msaa, findDepthFormat(), vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eDepthStencilAttachment,
      vk::MemoryPropertyFlagBits::eDeviceLocal);
}

void Swapchain::CreateFramebuffers(int w, int h) {
  framebuffers.resize(images.size());
  for (int i = 0; i < framebuffers.size(); i++) {
    auto& framebuffer = framebuffers[i];
    std::array<vk::ImageView, 3> attachments = {
        colorResource.view, depthResource.view, imageViews[i]};
    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.setAttachments(attachments)
        .setWidth(w)
        .setHeight(h)
        .setRenderPass(Context::GetInstance().renderProcess->renderPass)
        .setLayers(1);
    framebuffers[i] =
        Context::GetInstance().device.createFramebuffer(framebufferInfo);
  }
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes) {
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
      return availablePresentMode;
    }
  }
  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
                              int width, int height) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    vk::Extent2D actualExtent = {static_cast<uint32_t>(width),
                                 static_cast<uint32_t>(height)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

}  // namespace sktr