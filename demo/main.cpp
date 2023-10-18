#include "SDL.h"
#include "SDL_image.h"
#include "SDL_vulkan.h"
#include "sktr/sktr.hpp"
#include "sktr/utils/sdl_check.hpp"

constexpr uint32_t WindowWidth = 1024;
constexpr uint32_t WindowHeight = 720;

int getScaleFactor() {
  // Make the largest window possible with an integer scale factor
  SDL_Rect bounds;
#if SDL_VERSION_ATLEAST(2, 0, 5)
  SDL_CHECK(SDL_GetDisplayUsableBounds(0, &bounds));
#else
#warning SDL 2.0.5 or later is recommended
  SDL_CHECK(SDL_GetDisplayBounds(0, &bounds));
#endif
  const int scaleX = bounds.w / WindowWidth;
  const int scaleY = bounds.h / WindowHeight;
  return std::max(1, std::min(scaleX, scaleY));
}

int main(int argc, char** argv) {
  SDL_CHECK(SDL_Init(SDL_INIT_EVERYTHING));
  SDL_CHECK(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG));
  // const int scaleFactor = getScaleFactor();
  // std::cout << "Using scale factor: " << scaleFactor << '\n';

  SDL_Window* window = SDL_CreateWindow(
      "demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WindowWidth,
      WindowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
  if (!window) {
    SDL_Log("create window failed");
    exit(2);
  }
  unsigned int count;
  // 不同的系统对应的extension不同，可以通过SDL查询
  // 二次条用才能真正获取值
  SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr);
  std::vector<const char*> extensions(count);
  SDL_Vulkan_GetInstanceExtensions(window, &count, extensions.data());

  sktr::Init(
      extensions,
      [&](vk::Instance instance) {
        VkSurfaceKHR surface;
        if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
          throw std::runtime_error("can't create surface");
        }
        return surface;
      },
      WindowWidth, WindowHeight);
  auto& renderer = sktr::getRenderer();

  renderer.SetDrawColor(sktr::Color{1, 1, 1});

  sktr::Model viking = sktr::Model{"viking", "models/viking_room.obj"};
  viking.texture =
      sktr::TextureManager::GetInstance().Load("resources/viking_room.png");

  bool shouldClose = false;
  SDL_Event event;
  while (!shouldClose) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        shouldClose = true;
      }
    }
    renderer.StartRender();
    renderer.SetDrawColor({1, 1, 1});
    renderer.DrawModel(viking);
    renderer.EndRender();
  }

  sktr::TextureManager::GetInstance().Destroy(viking.texture);
  viking.vertexBuffer.reset();
  viking.indicesBuffer.reset();

  sktr::Quit();
  SDL_DestroyWindow(window);

  IMG_Quit();
  SDL_Quit();
  return 0;
}