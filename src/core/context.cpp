#include "context.hpp"

#include "constant.hpp"

namespace sktr {
Context::Context(const std::vector<const char*>& extensions,
                 CreateSurfaceFunc func)
    : func_(func) {
  createInstance(extensions);
  GetSurface();
  pickupPhysicalDevice();

  queryQueueFamilyIndices();
  createDevice();
  getQueues();
}

Context::~Context() {
  instance.destroySurfaceKHR(surface);
  device.destroy();
  instance.destroy();
}

void Context::createInstance(const std::vector<const char*>& extensions) {
  // layer: 层，用于调试
  // extension: 拓展，增加新功能(光追)
  // auto layers = vk::enumerateInstanceLayerProperties();
  // for (auto& layer : layers) {
  //     std::cout << layer.layerName << std::endl;
  //     SDL_Log("%s\n", std::string(layer.layerName).c_str());
  // }

  vk::InstanceCreateInfo instanceInfo;
  // 检查验证层
  if (EnableValidationLayers) {
    if (!checkValidationLayerSupport()) {
      throw std::runtime_error(
          "validation layers requested, but not available!");
    } else {
      instanceInfo.setPEnabledLayerNames(ValidationLayers);
    }
  }
  vk::ApplicationInfo appInfo;
  appInfo.setApiVersion(VK_API_VERSION_1_3).setPEngineName("SDL");
  instanceInfo.setPApplicationInfo(&appInfo).setPEnabledExtensionNames(
      extensions);
  instance = vk::createInstance(instanceInfo);
}

bool Context::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char* layerName : ValidationLayers) {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
}

void Context::pickupPhysicalDevice() {
  // 物理显卡的意义是用于查询各种参数，不能直接交互
  auto devices = instance.enumeratePhysicalDevices();
  if (devices.size() == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }
  phyDevice = devices[0];
  //  * NVIDIA GeForce RTX 4060 Laptop GPU
  // std::cout << phyDevice.getProperties().deviceName << std::endl;
}

void Context::createDevice() {
  // 逻辑设备也存在extension和layer，默认是继承自instance。但是它也有独有的。
  // 可以通过physicalDevice查询
  // phyDevice.enumerateDeviceExtensionProperties();
  // phyDevice.enumerateDeviceLayerProperties();

  // 加入Swapchain的拓展
  std::array extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  vk::DeviceCreateInfo deviceInfo;
  // 逻辑设备与物理设备通过Command queue进行打交道
  std::vector<vk::DeviceQueueCreateInfo> deviceQueueInfos;
  float prior = 1.0;
  if (queueFamilyIndices.presentQueue.value() ==
      queueFamilyIndices.graphicsQueue.value()) {
    vk::DeviceQueueCreateInfo deviceQueueInfo;
    deviceQueueInfo.setPQueuePriorities(&prior)
        .setQueueCount(1)
        .setQueueFamilyIndex(queueFamilyIndices.graphicsQueue.value());
    deviceQueueInfos.push_back(std::move(deviceQueueInfo));
  } else {
    vk::DeviceQueueCreateInfo deviceQueueInfo;
    deviceQueueInfo.setPQueuePriorities(&prior)
        .setQueueCount(1)
        .setQueueFamilyIndex(queueFamilyIndices.graphicsQueue.value());
    deviceQueueInfos.push_back(deviceQueueInfo);
    deviceQueueInfo.setPQueuePriorities(&prior)
        .setQueueCount(1)
        .setQueueFamilyIndex(queueFamilyIndices.presentQueue.value());
    deviceQueueInfos.push_back(deviceQueueInfo);
  }

  deviceInfo.setQueueCreateInfos(deviceQueueInfos)
      .setPEnabledExtensionNames(extensions);
  device = phyDevice.createDevice(deviceInfo);
}

void Context::queryQueueFamilyIndices() {
  // 所有队列的属性
  auto properties = phyDevice.getQueueFamilyProperties();
  for (int i = 0; i < properties.size(); i++) {
    const auto& property = properties[i];
    if (property.queueFlags | vk::QueueFlagBits::eGraphics) {
      // 最多可以创建多少个queue，因为只创建了一个，所以不需要检查
      // property.queueCount;
      queueFamilyIndices.graphicsQueue = i;
    }
    if (phyDevice.getSurfaceSupportKHR(i, surface)) {
      queueFamilyIndices.presentQueue = i;
    }

    if (queueFamilyIndices) {
      break;
    }
  }
}

void Context::getQueues() {
  // 从硬件中获取queue，并且队列只有一个则下标为0
  graphicsQueue = device.getQueue(queueFamilyIndices.graphicsQueue.value(), 0);
  presentQueue = device.getQueue(queueFamilyIndices.presentQueue.value(), 0);
}

void Context::InitSampler() {
  vk::SamplerCreateInfo createInfo;
  createInfo
      // 放大图像
      // nearest：就近复制，不会插值，保持像素感
      // linear等：插值
      .setMagFilter(vk::Filter::eLinear)
      // 缩小图像
      .setMinFilter(vk::Filter::eLinear)
      // xyz上超出范围后应该如何采样
      // repeat：重复；mirror：镜像重复；edge：重复边缘；border：重复边框
      .setAddressModeU(vk::SamplerAddressMode::eRepeat)
      .setAddressModeV(vk::SamplerAddressMode::eRepeat)
      .setAddressModeW(vk::SamplerAddressMode::eRepeat)
      // 不透明的黑色
      .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
      // 各向异性过滤
      .setAnisotropyEnable(false)
      // .setMaxAnisotropy(16)
      // Context::GetInstance().phyDevice.getProperties().limits.maxSamplerAnisotropy
      // 是否 不 对图像坐标进行归一化
      .setUnnormalizedCoordinates(false)
      // 在贴纹理之前是否对比设置的颜色，如果对比符合进行某些操作setCompareOp
      .setCompareEnable(false)
      // .setCompareOp()
      .setMipmapMode(vk::SamplerMipmapMode::eLinear);
  sampler = Context::GetInstance().device.createSampler(createInfo);
}

void Context::GetSurface() {
  surface = func_(instance);
  if (!surface) {
    throw std::runtime_error("surface is nullptr");
  }
}

}  // namespace sktr