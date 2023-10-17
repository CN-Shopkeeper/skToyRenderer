#pragma once

#include "renderer.hpp"
#include "sktr/pch.hpp"
#include "sktr/system/command_manager.hpp"
#include "sktr/system/render_process.hpp"
#include "sktr/system/sampler.hpp"
#include "sktr/system/shader.hpp"
#include "sktr/system/swapchain.hpp"
#include "sktr/utils/common.hpp"
#include "sktr/utils/singlton.hpp"
#include "sktr/utils/tools.hpp"

namespace sktr {
class Context final : public Singlton<Context> {
 public:
  vk::Instance instance;
  vk::PhysicalDevice phyDevice = nullptr;
  vk::Device device;
  vk::Queue graphicsQueue;
  vk::Queue presentQueue;
  vk::SurfaceKHR surface = nullptr;
  std::unique_ptr<Swapchain> swapchain;
  std::unique_ptr<RenderProcess> renderProcess;
  std::unique_ptr<Renderer> renderer;
  std::unique_ptr<CommandManager> commandManager;
  QueueFamilyIndices queueFamilyIndices;
  Sampler sampler;
  bool windowMinimized = false;
  bool frameBufferResized = false;

  Context(const std::vector<const char *> &extensions, CreateSurfaceFunc func);
  ~Context();

  void ResizeSwapchainImage(int w, int h);

  void InitSwapchain(int w, int h) { swapchain.reset(new Swapchain{w, h}); }
  void InitRenderer(int w, int h) { renderer.reset(new Renderer{w, h}); }
  void InitCommandPool() { commandManager.reset(new CommandManager); }
  void InitRenderProcess(int w, int h) {
    renderProcess.reset(new RenderProcess{w, h});
  }
  void InitSampler();
  void GetSurface();

  void DestroySwapchain() { swapchain.reset(); }
  void DestroyRenderer() { renderer.reset(); }
  void DestroyCommandPool() { commandManager.reset(); }
  void DestroyRenderProcess() { renderProcess.reset(); }
  void DestroySampler() { device.destroySampler(sampler.sampler); }

  SwapChainSupportDetails QuerySwapChainSupport(vk::PhysicalDevice device);

 private:
  CreateSurfaceFunc func_;

  void createInstance(const std::vector<const char *> &extensions);
  bool checkValidationLayerSupport();
  void pickupPhysicalDevice();
  void createDevice();

  QueueFamilyIndices queryQueueFamilyIndices(vk::PhysicalDevice physicalDevice);
  bool checkDeviceExtensionSupport(vk::PhysicalDevice);
  bool isDeviceSuitable(vk::PhysicalDevice);
  vk::SampleCountFlagBits getMaxUsableSampleCount();
};
}  // namespace sktr
