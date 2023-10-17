#pragma once

namespace sktr {
using Color = glm::vec3;
using Vec2 = glm::vec2;
using Mat4 = glm::mat4;

struct Rect {
  glm::vec2 position;
  glm::vec2 size;
};
}  // namespace sktr