// 交换链属于extension
// vulkan只负责GPU的图像渲染，不关心CPU或窗口，需要交换链来进行数据交换、图像格式转换，
// 屏幕的格式需用用surface拓展来处理
// graphicsQueue只负责渲染，不负责图像交换、显示，需要PresentQueue。

// 1. 首先将拓展传给instance
// 2. 再用SDL创建Surface catcher，方便转换
// 3. 创建交换链
// 4. 创建presentQueue

// frameBuffer - 帧缓冲
// 包含多个attachment - 纹理附件 - Texture - vk::Image
// 1+ Color Attachment颜色附件
// 0+ Stencil/Depth attachment模板/深度附件

#pragma once

#include "pch.hpp"

namespace sktr {
class Swapchain final {
 private:
  void queryInfo(int w, int h);
  void getImages();
  // imageView是对image的视图，读取时不会直接对image进行操作。可以修改读取image的方式
  void createImageViews();

 public:
  vk::SwapchainKHR swapchain;
  Swapchain(int w, int h);
  ~Swapchain();

  struct SwapchainInfo {
    // 图像大小
    vk::Extent2D imageExtent;
    // 图像的数量
    uint32_t imageCount;
    // 图像的颜色属性
    vk::SurfaceFormatKHR format;
    // SRT
    vk::SurfaceTransformFlagBitsKHR transform;
    // 处理顺序。
    // Fifo先进先出（vulkan一定支持）
    // FifoReleaxed先进先出但是绘制一半超时就可以放弃
    // Immediate，立刻绘制上去，无论前一张是否完成
    // mailbox最好，信箱里面最多只有一封信
    vk::PresentModeKHR present;
  };

  SwapchainInfo info;
  std::vector<vk::Image> images;
  std::vector<vk::ImageView> imageViews;
  std::vector<vk::Framebuffer> framebuffers;

  void CreateFramebuffers(int w, int h);
};
}  // namespace sktr