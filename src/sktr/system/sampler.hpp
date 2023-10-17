#pragma once

#include "sktr/pch.hpp"

namespace sktr {
class Sampler final {
 public:
  vk::Sampler sampler;
  vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;

 private:
};
}  // namespace sktr