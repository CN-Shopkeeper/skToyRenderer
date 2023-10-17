#pragma once

#include "sktr/core/context.hpp"

namespace sktr {
void Init(const std::vector<const char *> &extensions, CreateSurfaceFunc func,
          int w, int h);
void Quit();

void ResizeSwapchainImage(int w, int h);

inline Renderer &getRenderer() { return *Context::GetInstance().renderer; }

}  // namespace sktr