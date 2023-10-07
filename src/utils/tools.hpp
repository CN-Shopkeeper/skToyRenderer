#pragma once

#include "pch.hpp"

namespace sktr {
using CreateSurfaceFunc = std::function<vk::SurfaceKHR(vk::Instance)>;

template <typename T, typename U>
void RemoveNosupportedElems(std::vector<T>& elems,
                            const std::vector<U>& supportedElems,
                            std::function<bool(const T&, const U&)> eq) {
  int i = 0;
  while (i < elems.size()) {
    if (std::find_if(supportedElems.begin(), supportedElems.end(),
                     [&](const U& elem) { return eq(elems[i], elem); }) ==
        supportedElems.end()) {
      elems.erase(elems.begin() + i);
    } else {
      i++;
    }
  }
}

/**
 * @brief  读取二进制文件
 * @note
 * @param  filename: 二进制文件名称
 * @retval
 */
std::string ReadWholeFile(const std::string& filename);

/**
 * @brief
 * @note
 * @param  bits: requirement type bits
 * @param  flags: memory property flags
 * @retval
 * */
uint32_t QueryMemoryTypeIndex(uint32_t bits, vk::MemoryPropertyFlags flags);
}  // namespace sktr