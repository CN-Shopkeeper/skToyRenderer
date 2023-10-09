#include "context.hpp"

#include "constant.hpp"

namespace sktr {
Context::Context(const std::vector<const char*>& extensions,
                 CreateSurfaceFunc func)
    : func_(func) {
  createInstance(extensions);
  GetSurface();
  pickupPhysicalDevice();
  createDevice();
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
  vk::enumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<vk::LayerProperties> availableLayers(layerCount);
  vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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
  for (const auto& device : devices) {
    if (isDeviceSuitable(device)) {
      phyDevice = device;
      queueFamilyIndices = queryQueueFamilyIndices(device);
      sampler.msaaSamples = getMaxUsableSampleCount();
      break;
    }
  }
  if (phyDevice == nullptr) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

void Context::createDevice() {
  // 逻辑设备也存在extension和layer，默认是继承自instance。但是它也有独有的。
  // 可以通过physicalDevice查询
  // phyDevice.enumerateDeviceExtensionProperties();
  // phyDevice.enumerateDeviceLayerProperties();

  vk::DeviceCreateInfo deviceInfo;
  // 逻辑设备与物理设备通过Command queue进行打交道
  std::vector<vk::DeviceQueueCreateInfo> deviceQueueInfos;
  float prior = 1.0f;
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

  // 采样
  vk::PhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = vk::True;
  // enable sample shading feature for the device
  deviceFeatures.sampleRateShading = vk::True;

  deviceInfo.setQueueCreateInfos(deviceQueueInfos)
      .setPEnabledFeatures(&deviceFeatures);

  // 加入Swapchain的拓展
  std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  if (EnableValidationLayers) {
    // 在新版本里面逻辑设备和instance只需要设置一个validation layers
    extensions.insert(extensions.end(), DeviceExtensions.begin(),
                      DeviceExtensions.end());
  }
  deviceInfo.setPEnabledExtensionNames(extensions);

  device = phyDevice.createDevice(deviceInfo);

  // 从硬件中获取queue，并且队列只有一个则下标为0
  graphicsQueue = device.getQueue(queueFamilyIndices.graphicsQueue.value(), 0);
  presentQueue = device.getQueue(queueFamilyIndices.presentQueue.value(), 0);
}

QueueFamilyIndices Context::queryQueueFamilyIndices(
    vk::PhysicalDevice physicalDevice) {
  QueueFamilyIndices queueFamilyIndices_;
  // 所有队列的属性
  auto properties = phyDevice.getQueueFamilyProperties();
  for (int i = 0; i < properties.size(); i++) {
    const auto& property = properties[i];
    if (property.queueFlags | vk::QueueFlagBits::eGraphics) {
      // 最多可以创建多少个queue，因为只创建了一个，所以不需要检查
      // property.queueCount;
      queueFamilyIndices_.graphicsQueue = i;
    }
    if (phyDevice.getSurfaceSupportKHR(i, surface)) {
      queueFamilyIndices_.presentQueue = i;
    }

    if (queueFamilyIndices_) {
      break;
    }
  }
  return queueFamilyIndices_;
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
  sampler.sampler = Context::GetInstance().device.createSampler(createInfo);
}

void Context::GetSurface() {
  surface = func_(instance);
  if (!surface) {
    throw std::runtime_error("surface is nullptr");
  }
}

bool Context::isDeviceSuitable(vk::PhysicalDevice physicalDevice) {
  QueueFamilyIndices queueFamilyIndices_ =
      queryQueueFamilyIndices(physicalDevice);
  auto extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport =
        QuerySwapChainSupport(physicalDevice);
    swapChainAdequate = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }
  vk::PhysicalDeviceFeatures supportedFeatures = physicalDevice.getFeatures();

  return queueFamilyIndices_ && extensionsSupported && swapChainAdequate &&
         supportedFeatures.samplerAnisotropy;
}

bool Context::checkDeviceExtensionSupport(vk::PhysicalDevice physicalDevice) {
  uint32_t extensionCount;
  physicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount,
                                                    nullptr);
  std::vector<vk::ExtensionProperties> availableExtensions(extensionCount);
  physicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount,
                                                    availableExtensions.data());

  std::set<std::string> requiredExtensions(DeviceExtensions.begin(),
                                           DeviceExtensions.end());

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

SwapChainSupportDetails Context::QuerySwapChainSupport(
    vk::PhysicalDevice device) {
  SwapChainSupportDetails details;
  details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
  details.formats = device.getSurfaceFormatsKHR(surface);
  details.presentModes = device.getSurfacePresentModesKHR(surface);
  return details;
}

vk::SampleCountFlagBits Context::getMaxUsableSampleCount() {
  vk::PhysicalDeviceProperties physicalDeviceProperties =
      phyDevice.getProperties();
  vk::SampleCountFlags counts =
      physicalDeviceProperties.limits.framebufferColorSampleCounts &
      physicalDeviceProperties.limits.framebufferDepthSampleCounts;
  if (counts & vk::SampleCountFlagBits::e64) {
    return vk::SampleCountFlagBits::e64;
  }
  if (counts & vk::SampleCountFlagBits::e32) {
    return vk::SampleCountFlagBits::e32;
  }
  if (counts & vk::SampleCountFlagBits::e16) {
    return vk::SampleCountFlagBits::e16;
  }
  if (counts & vk::SampleCountFlagBits::e8) {
    return vk::SampleCountFlagBits::e8;
  }
  if (counts & vk::SampleCountFlagBits::e4) {
    return vk::SampleCountFlagBits::e4;
  }
  if (counts & vk::SampleCountFlagBits::e2) {
    return vk::SampleCountFlagBits::e2;
  }

  return vk::SampleCountFlagBits::e1;
}

}  // namespace sktr