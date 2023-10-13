#include "render_process.hpp"

#include "core/context.hpp"
#include "shader.hpp"
#include "swapchain.hpp"

namespace sktr {

RenderProcess::RenderProcess(int w, int h) {
  initRenderPass();
  initPipelineLayout();
  pipelineCache_ = createPipelineCache();
  initPipeline(w, h);
}

RenderProcess::~RenderProcess() {
  auto& device = Context::GetInstance().device;
  device.destroyPipelineCache(pipelineCache_);
  device.destroyRenderPass(renderPass);
  device.destroyPipelineLayout(pipelineLayout);
  device.destroyPipeline(graphicsPipelineWithLineTopology);
  device.destroyPipeline(graphicsPipelineWithTriangleTopology);
}

vk::Pipeline RenderProcess::createPipeline(int width, int height,
                                           vk::PrimitiveTopology topology) {
  vk::GraphicsPipelineCreateInfo graphicsPipelineInfo;

  // dynamic state

  std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport,
                                                 vk::DynamicState::eScissor};
  auto& ctx = Context::GetInstance();

  vk::PipelineDynamicStateCreateInfo dynamicStateInfo{};
  dynamicStateInfo.setDynamicStates(dynamicStates);
  graphicsPipelineInfo.setPDynamicState(&dynamicStateInfo);

  // 1. vertex input
  vk::PipelineVertexInputStateCreateInfo pipelineVertexInputeStateInfo;
  auto attribute = Vertex::GetAttributeDescriptions();
  auto binding = Vertex::GetBindingDescriptions();
  pipelineVertexInputeStateInfo.setVertexBindingDescriptions(binding)
      .setVertexAttributeDescriptions(attribute);
  graphicsPipelineInfo.setPVertexInputState(&pipelineVertexInputeStateInfo);

  // 2. vertex assembly
  vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateInfo;
  pipelineInputAssemblyStateInfo
      // 图元重写
      .setPrimitiveRestartEnable(false)
      // LineList: 1-2 3-4
      // LineStrip: 1-2-3-4
      // PointList
      // TriangleList: 1-2-3 4-5-6
      // TriangleStrip: 1-2-3 2-3-4 3-4-5
      // TriangleFan: 1-2-3 1-3-4 1-4-5
      .setTopology(topology);

  graphicsPipelineInfo.setPInputAssemblyState(&pipelineInputAssemblyStateInfo);

  // 3. shader
  auto stages = Shader::GetInstance().GetStages();
  graphicsPipelineInfo.setStages(stages);

  // 4. viewport
  vk::PipelineViewportStateCreateInfo viewportStateInfo;
  vk::Viewport viewport(0, 0, width, height, 0, 1);
  viewportStateInfo.setViewports(viewport);
  vk::Rect2D rect{
      {0, 0}, {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}};
  viewportStateInfo.setScissors(rect);
  graphicsPipelineInfo.setPViewportState(&viewportStateInfo);

  // 5. rasterization
  vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateInfo;
  pipelineRasterizationStateInfo.setDepthClampEnable(false)
      .setRasterizerDiscardEnable(false)
      // 面剔除
      .setCullMode(vk::CullModeFlagBits::eBack)
      // 正面方向
      .setFrontFace(vk::FrontFace::eCounterClockwise)
      // 多边形的填充模式
      .setPolygonMode(vk::PolygonMode::eFill)
      // 边框的宽度
      .setLineWidth(1)
      .setDepthBiasEnable(vk::False);
  graphicsPipelineInfo.setPRasterizationState(&pipelineRasterizationStateInfo);

  // 6. multisample
  vk::PipelineMultisampleStateCreateInfo multisampleStateInfo;
  multisampleStateInfo
      // 不进行超采样
      .setSampleShadingEnable(vk::True)
      // 在光栅化时进行的采样
      .setRasterizationSamples(ctx.sampler.msaaSamples)
      .setMinSampleShading(.2f);
  graphicsPipelineInfo.setPMultisampleState(&multisampleStateInfo);

  // 7. test - stencil test, depth test
  vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};
  depthStencilInfo.setDepthTestEnable(vk::True)
      .setDepthWriteEnable(vk::True)
      .setDepthCompareOp(vk::CompareOp::eLess)
      .setDepthBoundsTestEnable(vk::False)
      .setMinDepthBounds(0.0f)
      .setMaxDepthBounds(1.0f)
      .setStencilTestEnable(vk::False)
      // .setFront()
      // .setBack()
      ;
  graphicsPipelineInfo.setPDepthStencilState(&depthStencilInfo);

  // 8. color blending
  /*
   * newRGB = (srcFactor * srcRGB) <op> (dstFactor * dstRGB)
   * newA = (srcFactor * srcA) <op> (dstFactor * dstA)
   *
   * newRGB = 1 * srcRGB + (1 - srcA) * dstRGB
   * newA = srcA === 1 * srcA +0 * dstA
   */
  vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
  colorBlendAttachmentState.setBlendEnable(true)
      .setColorWriteMask(
          vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eB |
          vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eR)
      .setSrcColorBlendFactor(vk::BlendFactor::eOne)
      .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
      .setColorBlendOp(vk::BlendOp::eAdd)
      .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
      // .setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
      .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
      .setAlphaBlendOp(vk::BlendOp::eAdd);
  vk::PipelineColorBlendStateCreateInfo colorBlendStateInfo;
  colorBlendStateInfo
      // 不进行融混
      .setLogicOpEnable(false)
      // 设置附件
      .setAttachments(colorBlendAttachmentState);

  graphicsPipelineInfo.setPColorBlendState(&colorBlendStateInfo);

  // 9. renderPass and layout
  graphicsPipelineInfo.setRenderPass(renderPass).setLayout(pipelineLayout);

  auto result = Context::GetInstance().device.createGraphicsPipeline(
      // pipeline cache 是黑盒，不需要关心里面存了什么
      // 并不是说有了cache，上面的一大串就不用写了
      pipelineCache_, graphicsPipelineInfo);

  if (result.result != vk::Result::eSuccess) {
    throw std::runtime_error("create graphics pipeline failed");
  }
  return result.value;
}

void RenderProcess::initPipeline(int width, int height) {
  graphicsPipelineWithTriangleTopology =
      createPipeline(width, height, vk::PrimitiveTopology::eTriangleList);
  graphicsPipelineWithLineTopology =
      createPipeline(width, height, vk::PrimitiveTopology::eLineList);
}

void RenderProcess::initPipelineLayout() {
  vk::PipelineLayoutCreateInfo layoutInfo;
  auto range = Shader::GetInstance().GetPushConstantRange();
  //     createInfo.setSetLayouts(Context::Instance().shader->GetDescriptorSetLayouts())
  //               .setPushConstantRanges(range);
  layoutInfo.setSetLayouts(Shader::GetInstance().descriptorSetLayouts)
      .setPushConstantRanges(range);
  pipelineLayout =
      Context::GetInstance().device.createPipelineLayout(layoutInfo);
}

void RenderProcess::initRenderPass() {
  auto& ctx = Context::GetInstance();

  // 纹理附件的描述

  //   MSAA
  vk::AttachmentDescription colorAttachment;
  colorAttachment
      .setFormat(ctx.swapchain->info.surfaceFormat.format)
      // 初始渲染布局
      .setInitialLayout(vk::ImageLayout::eUndefined)
      // 出去的布局
      .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
      // 加载时进行的操作
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      // 绘制完成后如何存储
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      // 模板缓冲
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setSamples(ctx.sampler.msaaSamples);

  vk::AttachmentReference colorAttachmentRef{};
  //   下标
  colorAttachmentRef.setAttachment(0);
  colorAttachmentRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

  // image to present
  vk::AttachmentDescription colorAttachmentResolve{};
  colorAttachmentResolve.setFormat(ctx.swapchain->info.surfaceFormat.format)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

  vk::AttachmentReference colorAttachmentResolveRef{};
  colorAttachmentRef.setAttachment(2);
  colorAttachmentRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

  // depth
  vk::AttachmentDescription depthAttachment{};
  depthAttachment.setFormat(findDepthFormat())
      .setSamples(ctx.sampler.msaaSamples)
      .setLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

  vk::AttachmentReference depthAttachmentRef{};
  depthAttachmentRef.setAttachment(1);
  depthAttachmentRef.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

  vk::SubpassDescription subpass{};
  subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
      .setColorAttachments(colorAttachmentRef)
      .setPDepthStencilAttachment(&depthAttachmentRef)
      .setResolveAttachments(colorAttachmentResolveRef);

  std::array<vk::AttachmentDescription, 3> attachments = {
      colorAttachment, depthAttachment, colorAttachmentResolve};

  vk::RenderPassCreateInfo renderPassInfo;
  renderPassInfo
      // 一个渲染流程可以分为多个子流程
      .setSubpasses(subpass)
      .setAttachments(attachments);

  // 当有多个subpass的时候，先后执行顺序
  vk::SubpassDependency dependency;
  dependency
      // 有默认的subpass
      .setSrcSubpass(VK_SUBPASS_EXTERNAL)
      // 也是dst是结果subpass，也就是我们设置的。下标
      .setDstSubpass(0)
      // 设置dst的访问权限
      .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite |
                        vk::AccessFlagBits::eDepthStencilAttachmentWrite)
      // 渲染过程完成后用于什么场景
      .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                       vk::PipelineStageFlagBits::eEarlyFragmentTests)
      .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                       vk::PipelineStageFlagBits::eEarlyFragmentTests);
  renderPassInfo.setDependencies(dependency);

  renderPass = Context::GetInstance().device.createRenderPass(renderPassInfo);
}

vk::PipelineCache RenderProcess::createPipelineCache() {
  vk::PipelineCacheCreateInfo cacheInfo;
  cacheInfo
      // 不设置初始值
      .setInitialDataSize(0)
      // .setInitialData(
      //     Context::GetInstance().device.getPipelineCacheData(pipelineCache));
      ;

  return Context::GetInstance().device.createPipelineCache(cacheInfo);
}
}  // namespace sktr