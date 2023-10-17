#include "sktr.hpp"

namespace sktr {

void Init(const std::vector<const char *> &extensions, CreateSurfaceFunc func,
          int w, int h) {
  Context::Init(extensions, func);
  auto &ctx = Context::GetInstance();
  ctx.InitSwapchain(w, h);
  Shader::Init(ReadWholeFile("./shaders/vert.spv"),
               ReadWholeFile("./shaders/frag.spv"));
  ctx.InitRenderProcess(w, h);
  // ! after renderPass
  ctx.swapchain->CreateFramebuffers(w, h);
  // ! CommandPool before renderer
  ctx.InitCommandPool();
  DescriptorSetManager::Init(2);
  ctx.InitSampler();
  ctx.InitRenderer(w, h);
}

void Quit() {
  auto &ctx = Context::GetInstance();
  ctx.device.waitIdle();
  ctx.DestroyRenderer();
  ctx.DestroySampler();
  // textureManager.clear is executed by ~renderer
  // ! can not deconstruct in ~Context and after render
  ctx.DestroyCommandPool();
  ctx.DestroyRenderProcess();
  Shader::Quit();
  // it will also destroy Framebuffers
  ctx.DestroySwapchain();
  DescriptorSetManager::Quit();
  Context::Quit();
}

void ResizeSwapchainImage(int w, int h) {
  auto &ctx = Context::GetInstance();
  ctx.device.waitIdle();

  ctx.swapchain.reset();
  ctx.GetSurface();
  ctx.InitSwapchain(w, h);
  ctx.swapchain->CreateFramebuffers(w, h);
}
}  // namespace sktr