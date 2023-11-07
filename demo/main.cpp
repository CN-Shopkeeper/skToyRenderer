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

glm::vec3 polarVector(float p, float y) {
  // this form is already normalized
  return glm::vec3(std::cos(y) * std::cos(p), std::sin(y) * std::cos(p),
                   std::sin(p));
}

// clamp pitch to [-89, 89]
float clampPitch(float p) {
  return p > 89.0f ? 89.0f : (p < -89.0f ? -89.0f : p);
}

// clamp yaw to [-180, 180] to reduce floating point inaccuracy
float clampYaw(float y) {
  float temp = (y + 180.0f) / 360.0f;
  return y - ((int)temp - (temp < 0.0f ? 1 : 0)) * 360.0f;
}

const float sensitivity = 0.01f;

int main(int argc, char** argv) {
  SDL_CHECK(SDL_Init(SDL_INIT_EVERYTHING));
  SDL_CHECK(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG));

  SDL_SetRelativeMouseMode(SDL_TRUE);
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

  sktr::Model viking =
      sktr::Model{"viking", "models/viking_room.obj", "models/"};
  viking.texture =
      sktr::TextureManager::GetInstance().Load("resources/viking_room.png");

  bool shouldClose = false;
  SDL_Event event;

  glm::vec3 eye = {2, 0, 2};
  float pitch = -45.0f, yaw = 0.0f;
  glm::vec3 cameraFront = {0, 0, 1};
  const glm::vec3 upVector = {0.f, 0.f, 1.f};
  while (!shouldClose) {
    float xrel = 0;
    float yrel = 0;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        shouldClose = true;
      }
      if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.scancode) {
          case SDL_SCANCODE_ESCAPE:
            shouldClose = true;
            break;
          case SDL_SCANCODE_W:
            eye -= sensitivity * cameraFront;
            break;
          case SDL_SCANCODE_S:
            eye += sensitivity * cameraFront;
            break;
          case SDL_SCANCODE_A: {
            glm::vec3 cameraLeft =
                glm::normalize(glm::cross(cameraFront, upVector));
            eye += sensitivity * cameraLeft;
          } break;
          case SDL_SCANCODE_D: {
            glm::vec3 cameraRight =
                glm::normalize(glm::cross(upVector, cameraFront));
            eye += sensitivity * cameraRight;
          } break;
          default:
            break;
        }
        if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
        }
      }
      if (event.type == SDL_MOUSEMOTION) {
        xrel += event.motion.xrel;
        yrel += event.motion.yrel;
      }
    }

    yaw = clampYaw(yaw + sensitivity * xrel);
    pitch = clampPitch(pitch - sensitivity * yrel);

    // assumes radians input
    cameraFront = polarVector(glm::radians(-pitch), glm::radians(-yaw));

    renderer.SetView(glm::lookAt(eye, eye - cameraFront, upVector));

    // static auto startTime = std::chrono::high_resolution_clock::now();

    // auto currentTime = std::chrono::high_resolution_clock::now();
    // float time = std::chrono::duration<float, std::chrono::seconds::period>(
    //                  currentTime - startTime)
    //                  .count();
    // viking.SetModelM(glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
    //                              glm::vec3(0.0f, 0.0f, 1.0f)));
    renderer.StartRender();
    renderer.SetDrawColor({1, 1, 1});
    renderer.DrawModel(viking);
    renderer.EndRender();
    SDL_Delay(30);
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