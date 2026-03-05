#include "VKM_Base/VK_Base.h"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <glTFModel.h>

//GLFWwindow* pWindow;
//GLFWmonitor* pMonitor;
//const char* windowTitle = "MiniRender";
//
//bool InitializeWindow(VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true) {
//
//	if (!glfwInit()) {
//		std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n");
//		return false;
//	}
//	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//	glfwWindowHint(GLFW_RESIZABLE, isResizable);
//	pMonitor = glfwGetPrimaryMonitor();
//	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
//	pWindow = fullScreen ?
//		glfwCreateWindow(pMode->width, pMode->height, windowTitle, pMonitor, nullptr) :
//		glfwCreateWindow(size.width, size.height, windowTitle, nullptr, nullptr);
//	if (!pWindow) {
//		std::cout << std::format("[ InitializeWindow ]\nFailed to create a glfw window!\n");
//		glfwTerminate();
//		return false;
//	}
//	/*TODO*/
//	
//	VKM_Base::Get().SetCreateSurface([](vk::Instance instance) {
//		VkSurfaceKHR surface;
//		if (glfwCreateWindowSurface(instance, pWindow, nullptr, &surface) != VK_SUCCESS)
//		{
//			throw std::runtime_error("create surface is failed");
//		}
//		return surface;
//		});
//	
//
//	return true;
//}
//
//void TerminateWindow() {
//	glfwTerminate();
//}
//void MakeWindowFullScreen() {
//	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
//	glfwSetWindowMonitor(pWindow, pMonitor, 0, 0, pMode->width, pMode->height, pMode->refreshRate);
//}
//void MakeWindowWindowed(VkOffset2D position, VkExtent2D size) {
//	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
//	glfwSetWindowMonitor(pWindow, nullptr, position.x, position.y, size.width, size.height, pMode->refreshRate);
//}
//void TitleFps() {
//	static double time0 = glfwGetTime();
//	static double time1;
//	static double dt;
//	static int dframe = -1;
//	static std::stringstream info;
//	time1 = glfwGetTime();
//	dframe++;
//	if ((dt = time1 - time0) >= 1) {
//		info.precision(1);
//		info << windowTitle << "    " << std::fixed << dframe / dt << " FPS";
//		glfwSetWindowTitle(pWindow, info.str().c_str());
//		info.str("");
//		time0 = time1;
//		dframe = 0;
//	}
//}

//int main() {
//	auto& singleton = VKM_Base::Get();
//
//	if (!InitializeWindow({ 1280, 720 }))
//		return -1;
//	
//	singleton.initVulkan();
//	singleton.prepare();
//
//	while (!glfwWindowShouldClose(pWindow)) {
//
//		/*äÖÈ¾¹ý³Ì£¬´ýÌî³ä*/
//
//		glfwPollEvents();
//		TitleFps();
//	}
//	TerminateWindow();
//	delete(&singleton);
//	return 0;
//	
//}

#define FB_DIM 256
#define FB_COLOR_FORMAT vk::Format::eR8G8B8A8Unorm
class VulkanExample :public VKM_Base
{
private:
	bool bloom = true;
	std::unique_ptr<vkm::TextureCubeMap> cubemap;
	struct Models {
		std::optional<vkmglTF::Model> ufo;
		std::optional<vkmglTF::Model> ufoGlow;
		std::optional<vkmglTF::Model> skyBox;
	}models;

	struct UBO {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
	};

	struct UBOBlurParams {
		float blurScale = 1.0f;
		float blurStrength = 1.5f;
	};

	struct {
		UBO scene, skyBox;
		UBOBlurParams blurParams;
	} ubos;

	struct UniformBuffers {
		vkm::Buffer scene;
		vkm::Buffer skyBox;
		vkm::Buffer blurParams;
	};
	std::array<UniformBuffers, maxConcurrentFrames> uniformBuffers{};

	struct {
		vk::PipelineLayout blur;
		vk::PipelineLayout scene;
	} pipelineLayouts{};

	struct {
		vk::Pipeline blurVert;
		vk::Pipeline blurHorz;
		vk::Pipeline glowPass;
		vk::Pipeline phongPass;
		vk::Pipeline skyBox;
	} pipelines{};

	struct {
		vk::DescriptorSetLayout blur;
		vk::DescriptorSetLayout scene;
	} descriptorSetLayouts{};

	struct DescriptorSets {
		vk::DescriptorSet blurVert;
		vk::DescriptorSet blurHorz;
		vk::DescriptorSet scene;
		vk::DescriptorSet skyBox;
	};
	std::array<DescriptorSets, maxConcurrentFrames> descriptorSets{};

	// Framebuffer for offscreen rendering
	struct FrameBufferAttachment {
		vk::Image image;
		vk::DeviceMemory mem;
		vk::ImageView view;
	};
	struct FrameBuffer {
		vk::Framebuffer framebuffer;
		FrameBufferAttachment color, depth;
		vk::DescriptorImageInfo descriptor;
	};
	struct OffscreenPass {
		uint32_t width, height;
		vk::RenderPass renderPass;
		vk::Sampler sampler;
		std::array<FrameBuffer, 2> framebuffers;
	} offscreenPass{};
public:
	VulkanExample() :VKM_Base()
	{
		displayWindows.title = "Bloom (offscreen rendering)";
		timerSpeed *= 0.5f;
		auto& camera = displayWindows.camera;
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -10.25f));
		camera.setRotation(glm::vec3(7.5f, -343.0f, 0.0f));
		camera.setPerspective(45.0f, (float)width / (float)height, 0.1f, 256.0f);
	}

	~VulkanExample()
	{
		if (device) {
			device.destroySampler(offscreenPass.sampler, nullptr);
			for (auto& framebuffer : offscreenPass.framebuffers) {
				device.destroyImageView(framebuffer.color.view, nullptr);
				device.destroyImage(framebuffer.color.image, nullptr);
				device.freeMemory(framebuffer.color.mem, nullptr);
				device.destroyImageView(framebuffer.depth.view, nullptr);
				device.destroyImage(framebuffer.depth.image, nullptr);
				device.freeMemory(framebuffer.depth.mem, nullptr);
				device.destroyFramebuffer(framebuffer.framebuffer, nullptr);
			}
			device.destroyRenderPass( offscreenPass.renderPass, nullptr);
			device.destroyPipeline(pipelines.blurHorz, nullptr);
			device.destroyPipeline(pipelines.blurVert, nullptr);
			device.destroyPipeline(pipelines.phongPass, nullptr);
			device.destroyPipeline(pipelines.glowPass, nullptr);
			device.destroyPipeline(pipelines.skyBox, nullptr);
			device.destroyPipelineLayout(pipelineLayouts.blur, nullptr);
			device.destroyPipelineLayout(pipelineLayouts.scene, nullptr);
			device.destroyDescriptorSetLayout(descriptorSetLayouts.blur, nullptr);
			device.destroyDescriptorSetLayout(descriptorSetLayouts.scene, nullptr);
			for (auto& buffer : uniformBuffers) {
				buffer.blurParams.destroy();
				buffer.scene.destroy();
				buffer.skyBox.destroy();
			}
			if(cubemap)
				cubemap->destroy();
		}
	}

	void prepareOffscreenFramebuffer(FrameBuffer* framebuffer, vk::Format colorFormat, vk::Format depthFormat)
	{
		//Color attachment
		vk::ImageCreateInfo imageCreateInfo;
		imageCreateInfo.setImageType(vk::ImageType::e2D)
			.setFormat(colorFormat)
			.setExtent({ FB_DIM,FB_DIM,1 })
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			// sample directly from the color attachment
			.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
		VK_CHECK_RESULT(device.createImage(&imageCreateInfo, nullptr, &framebuffer->color.image));
		
		vk::ImageViewCreateInfo colorImageViewCI;
		vk::ImageSubresourceRange colorRange;
		colorRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);
		colorImageViewCI.setViewType(vk::ImageViewType::e2D)
			.setImage(framebuffer->color.image)
			.setFormat(colorFormat)
			.setSubresourceRange(colorRange);
		VKMDevice->AllocBindImageMem(vk::MemoryPropertyFlagBits::eDeviceLocal, framebuffer->color.image, framebuffer->color.mem);
		VK_CHECK_RESULT(device.createImageView(&colorImageViewCI, nullptr, &framebuffer->color.view));

		//depth stencil attachment
		imageCreateInfo.setFormat(depthFormat)
			.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
		VK_CHECK_RESULT(device.createImage(&imageCreateInfo, nullptr, &framebuffer->depth.image));

		vk::ImageViewCreateInfo depthStencilViewCI;
		vk::ImageSubresourceRange depthRange;
		depthRange.setAspectMask(vk::ImageAspectFlagBits::eDepth)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);
		if (vkm::tools::formatHasStencil(depthFormat))
		{
			depthRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
		depthStencilViewCI.setViewType(vk::ImageViewType::e2D)
			.setImage(framebuffer->depth.image)
			.setFormat(depthFormat)
			.setSubresourceRange(depthRange);
		VKMDevice->AllocBindImageMem(vk::MemoryPropertyFlagBits::eDeviceLocal, framebuffer->depth.image, framebuffer->depth.mem);
		VK_CHECK_RESULT(device.createImageView(&depthStencilViewCI, nullptr, &framebuffer->depth.view));
		//create framebuffer
		vk::ImageView attachments[2]{
			framebuffer->color.view,
			framebuffer->depth.view
		};
		vk::FramebufferCreateInfo FramebufferCI;
		FramebufferCI.setRenderPass(offscreenPass.renderPass)
			.setAttachments(attachments)
			.setWidth(FB_DIM)
			.setHeight(FB_DIM)
			.setLayers(1);
		VK_CHECK_RESULT(device.createFramebuffer(&FramebufferCI, nullptr, &framebuffer->framebuffer));
		framebuffer->descriptor.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		framebuffer->descriptor.imageView = framebuffer->color.view;
		framebuffer->descriptor.sampler = offscreenPass.sampler;
	}

	void prepareOffscreen()
	{
		offscreenPass.width = FB_DIM;
		offscreenPass.height = FB_DIM;
		vk::Format fbDepthFormat;
		vk::Bool32 validDepthFormat = vkm::tools::getSupportedDepthFormat(physicalDevice, &fbDepthFormat);
		assert(validDepthFormat);
		//Create a separate render pass for the offscreen rendering
		std::array<vk::AttachmentDescription, 2> attchmentDescriptions{};
		//color attachment
		attchmentDescriptions[0].format = FB_COLOR_FORMAT;
		attchmentDescriptions[0].samples = vk::SampleCountFlagBits::e1;
		attchmentDescriptions[0].loadOp = vk::AttachmentLoadOp::eClear;
		attchmentDescriptions[0].storeOp = vk::AttachmentStoreOp::eStore;
		attchmentDescriptions[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attchmentDescriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attchmentDescriptions[0].initialLayout = vk::ImageLayout::eUndefined;
		attchmentDescriptions[0].finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		//depth attachment
		attchmentDescriptions[1].format = fbDepthFormat;
		attchmentDescriptions[1].samples = vk::SampleCountFlagBits::e1;
		attchmentDescriptions[1].loadOp = vk::AttachmentLoadOp::eClear;
		attchmentDescriptions[1].storeOp = vk::AttachmentStoreOp::eDontCare;
		attchmentDescriptions[1].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attchmentDescriptions[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attchmentDescriptions[1].initialLayout = vk::ImageLayout::eUndefined;
		attchmentDescriptions[1].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		vk::AttachmentReference colorReference = { 0,vk::ImageLayout::eColorAttachmentOptimal };
		vk::AttachmentReference depthReference = { 1,vk::ImageLayout::eDepthStencilAttachmentOptimal };

		vk::SubpassDescription subpassDescription;
		subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.pDepthStencilAttachment = &depthReference;

		// Use subpass dependencies for layout transitions
		std::array<vk::SubpassDependency, 3> dependencies{};
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
		dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
		dependencies[0].srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		dependencies[0].dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
		dependencies[0].dependencyFlags = {};

		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eFragmentShader;
		dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependencies[1].srcAccessMask = vk::AccessFlagBits::eShaderRead;
		dependencies[1].dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		dependencies[2].srcSubpass = 0;
		dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[2].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependencies[2].dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
		dependencies[2].srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		dependencies[2].dstAccessMask = vk::AccessFlagBits::eShaderRead;
		dependencies[2].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		// Create the actual renderpass
		vk::RenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.setAttachments(attchmentDescriptions)
			.setSubpasses(subpassDescription)
			.setDependencies(dependencies);
		VK_CHECK_RESULT(device.createRenderPass(&renderPassInfo, nullptr, &offscreenPass.renderPass));
		
		//Create sampler to sample from the color attachments
		vk::SamplerCreateInfo samplerCreateInfo;
		samplerCreateInfo.setMagFilter(vk::Filter::eLinear)
			.setMinFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
			.setMipLodBias(0.f)
			.setMaxAnisotropy(1.f)
			.setMinLod(0.f)
			.setMaxLod(1.0f)
			.setBorderColor(vk::BorderColor::eFloatOpaqueWhite);
		VK_CHECK_RESULT(device.createSampler(&samplerCreateInfo, nullptr, &offscreenPass.sampler));

		// Create two frame buffers
		prepareOffscreenFramebuffer(&offscreenPass.framebuffers[0], FB_COLOR_FORMAT, fbDepthFormat);
		prepareOffscreenFramebuffer(&offscreenPass.framebuffers[1], FB_COLOR_FORMAT, fbDepthFormat);
	}

	void setupDescriptors()
	{
		//pool
		std::vector < vk::DescriptorPoolSize> poolSizes{
			{vk::DescriptorType::eUniformBuffer, maxConcurrentFrames * 8},/*type, size*/
			{vk::DescriptorType::eCombinedImageSampler, maxConcurrentFrames * 6}
		};
		vk::DescriptorPoolCreateInfo descriptorPoolInfo;
		descriptorPoolInfo.setPoolSizes(poolSizes)
			.setMaxSets(maxConcurrentFrames * 4);
		VK_CHECK_RESULT(device.createDescriptorPool(&descriptorPoolInfo, nullptr, &descriptorPool));
		// Layouts
		std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings;
		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;

		// Fullscreen blur
		setLayoutBindings = {
			{/*bindings*/ 0, vk::DescriptorType::eUniformBuffer,/*descriptorCount*/ 1, vk::ShaderStageFlagBits::eFragment },
			{ 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment }
		};
		descriptorSetLayoutCreateInfo.setBindings(setLayoutBindings);
		VK_CHECK_RESULT(device.createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayouts.blur));
		
		//Scene rendering
		setLayoutBindings = {
			{/*bindings*/ 0, vk::DescriptorType::eUniformBuffer,/*descriptorCount*/ 1, vk::ShaderStageFlagBits::eVertex },
			{ 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
			{ 2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment }
		};
		descriptorSetLayoutCreateInfo.setBindings(setLayoutBindings);
		VK_CHECK_RESULT(device.createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayouts.scene));

		// Sets per frame, just like the buffers themselves
		// Images do not need to be duplicated per frame, we reuse the same one for each frame
		for (auto i = 0; i < uniformBuffers.size(); i++)
		{
			// Vertical full screen blur
			vk::DescriptorSetAllocateInfo descriptorSetAllocInfo{ descriptorPool,1, &descriptorSetLayouts.blur };
			VK_CHECK_RESULT(device.allocateDescriptorSets(&descriptorSetAllocInfo, &descriptorSets[i].blurVert));
			std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
				vkm::initializers::writeDescriptorSet(descriptorSets[i].blurVert, vk::DescriptorType::eUniformBuffer, 0, &uniformBuffers[i].blurParams.descriptor),
				vkm::initializers::writeDescriptorSet(descriptorSets[i].blurVert, vk::DescriptorType::eCombinedImageSampler, 1, &offscreenPass.framebuffers[0].descriptor),
			};
			device.updateDescriptorSets(writeDescriptorSets, {});

			// Horizontal full screen blur
			descriptorSetAllocInfo = { descriptorPool,1, &descriptorSetLayouts.blur };
			VK_CHECK_RESULT(device.allocateDescriptorSets(&descriptorSetAllocInfo, &descriptorSets[i].blurHorz));
			writeDescriptorSets = {
				vkm::initializers::writeDescriptorSet(descriptorSets[i].blurHorz, vk::DescriptorType::eUniformBuffer, 0, &uniformBuffers[i].blurParams.descriptor),
				vkm::initializers::writeDescriptorSet(descriptorSets[i].blurHorz, vk::DescriptorType::eCombinedImageSampler, 1, &offscreenPass.framebuffers[1].descriptor),
			};
			device.updateDescriptorSets(writeDescriptorSets,{});

			// Scene rendering
			descriptorSetAllocInfo = { descriptorPool,1, &descriptorSetLayouts.scene };
			VK_CHECK_RESULT(device.allocateDescriptorSets(&descriptorSetAllocInfo, &descriptorSets[i].scene));
			writeDescriptorSets = {
				vkm::initializers::writeDescriptorSet(descriptorSets[i].scene, vk::DescriptorType::eUniformBuffer, 0, &uniformBuffers[i].scene.descriptor)
			};
			device.updateDescriptorSets(writeDescriptorSets, {});

			// Skybox
			VK_CHECK_RESULT(device.allocateDescriptorSets(&descriptorSetAllocInfo, &descriptorSets[i].skyBox));
			writeDescriptorSets = {
				vkm::initializers::writeDescriptorSet(descriptorSets[i].skyBox, vk::DescriptorType::eUniformBuffer, 0, &uniformBuffers[i].skyBox.descriptor),
				vkm::initializers::writeDescriptorSet(descriptorSets[i].skyBox, vk::DescriptorType::eCombinedImageSampler, 1, &cubemap->descriptorImageInfo)
			};
			device.updateDescriptorSets(writeDescriptorSets, {});
		}
	}

	void preparePipelines()
	{
		//blur
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		pipelineLayoutCreateInfo.setSetLayouts(descriptorSetLayouts.blur);
		VK_CHECK_RESULT(device.createPipelineLayout( &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.blur));

		//scene
		pipelineLayoutCreateInfo.setSetLayouts(descriptorSetLayouts.scene);
		VK_CHECK_RESULT(device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.scene));

		//Input Assemble
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCI;
		inputAssemblyStateCI.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setPrimitiveRestartEnable(VK_FALSE);

		//rasterization
		vk::PipelineRasterizationStateCreateInfo rasterizationStateCI;
		rasterizationStateCI.setPolygonMode(vk::PolygonMode::eFill)
			.setCullMode(vk::CullModeFlagBits::eNone)
			.setFrontFace(vk::FrontFace::eCounterClockwise)
			.setDepthClampEnable(VK_FALSE)
			.setLineWidth(1.0f);

		//blend
		vk::PipelineColorBlendAttachmentState blendAttachmentState;
		blendAttachmentState.setColorWriteMask(vk::ColorComponentFlagBits::eA |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eR)
			.setBlendEnable(VK_TRUE)
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setSrcColorBlendFactor(vk::BlendFactor::eOne)
			.setDstColorBlendFactor(vk::BlendFactor::eOne)
			.setAlphaBlendOp(vk::BlendOp::eAdd)
			.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
			.setDstAlphaBlendFactor(vk::BlendFactor::eDstAlpha);
		vk::PipelineColorBlendStateCreateInfo colorBlendStateCI;
		colorBlendStateCI.setAttachments(blendAttachmentState);
		//depth | stencil
		vk::PipelineDepthStencilStateCreateInfo depthStencilStateCI;
		vk::StencilOpState backState;
		backState.setCompareOp(vk::CompareOp::eAlways);
		depthStencilStateCI.setDepthTestEnable(VK_TRUE)
			.setDepthWriteEnable(VK_TRUE)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
			.setBack(backState);
		// viewport
		vk::PipelineViewportStateCreateInfo viewportStateCI;
		viewportStateCI.setViewportCount(1)
			.setScissorCount(1);
		// multiSample
		vk::PipelineMultisampleStateCreateInfo multisampleStateCI;
		multisampleStateCI.setRasterizationSamples(vk::SampleCountFlagBits::e1);
		// dynamicState
		std::vector<vk::DynamicState> dynamicStateEnables = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamicStateCI;
		dynamicStateCI.setDynamicStates(dynamicStateEnables);
		//
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages{};
		//pipeline
		vk::GraphicsPipelineCreateInfo pipelineCI;
		pipelineCI.setRenderPass(renderPass)
			.setLayout(pipelineLayouts.blur)
			.setBasePipelineIndex(-1)
			.setBasePipelineHandle(VK_NULL_HANDLE)
			.setPInputAssemblyState(&inputAssemblyStateCI)
			.setPRasterizationState(&rasterizationStateCI)
			.setPColorBlendState(&colorBlendStateCI)
			.setPMultisampleState(&multisampleStateCI)
			.setPViewportState(&viewportStateCI)
			.setPDepthStencilState(&depthStencilStateCI)
			.setPDynamicState(&dynamicStateCI)
			.setStageCount(static_cast<uint32_t>(shaderStages.size()))
			.setPStages(shaderStages.data());
		//blur pipelines
		shaderStages[0] = loadShader(getShadersPath() + "bloom/gaussblur.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = loadShader(getShadersPath() + "bloom/gaussblur.frag.spv", vk::ShaderStageFlagBits::eFragment);
		// empty vertex input state
		vk::PipelineVertexInputStateCreateInfo emptyInputState;
		pipelineCI.pVertexInputState = &emptyInputState;
		pipelineCI.layout = pipelineLayouts.blur;

		// Use specialization constants to select between horizontal and vertical blur
		uint32_t blurdirection = 0;
		vk::SpecializationMapEntry specializationMapEntry;
		specializationMapEntry.setConstantID(0)
			.setOffset(0)
			.setSize(sizeof(uint32_t));
		vk::SpecializationInfo specializationInfo;
		specializationInfo.setMapEntries(specializationMapEntry)
			.setDataSize(sizeof(uint32_t))
			.setPData(&blurdirection);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		// Vertical blur pipeline
		pipelineCI.renderPass = offscreenPass.renderPass;
		VK_CHECK_RESULT(device.createGraphicsPipelines(pipelineCache, 1, &pipelineCI, nullptr, &pipelines.blurVert));
		// Horizontal blur pipeline
		blurdirection = 1;
		pipelineCI.renderPass = renderPass;
		VK_CHECK_RESULT(device.createGraphicsPipelines(pipelineCache, 1, &pipelineCI, nullptr, &pipelines.blurHorz));

		// Phong pass (3D model)
		/*bind with input vertex attributation/ description, VertexInputBindingDescription | VertexAttributeDescriptions*/
		pipelineCI.pVertexInputState = vkmglTF::Vertex::getPipelineVertexInputState({ vkmglTF::VertexComponent::Position, vkmglTF::VertexComponent::UV, vkmglTF::VertexComponent::Color, vkmglTF::VertexComponent::Normal });
		pipelineCI.layout = pipelineLayouts.scene;
		shaderStages[0] = loadShader(getShadersPath() + "bloom/phongpass.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = loadShader(getShadersPath() + "bloom/phongpass.frag.spv", vk::ShaderStageFlagBits::eFragment);
		blendAttachmentState.blendEnable = VK_FALSE;
		depthStencilStateCI.depthWriteEnable = VK_TRUE;
		rasterizationStateCI.cullMode = vk::CullModeFlagBits::eBack;
		pipelineCI.renderPass = renderPass;
		VK_CHECK_RESULT(device.createGraphicsPipelines(pipelineCache, 1, &pipelineCI, nullptr, &pipelines.phongPass));

		// Color only pass (offscreen blur base)
		shaderStages[0] = loadShader(getShadersPath() + "bloom/colorpass.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = loadShader(getShadersPath() + "bloom/colorpass.frag.spv", vk::ShaderStageFlagBits::eFragment);
		pipelineCI.renderPass = offscreenPass.renderPass;
		VK_CHECK_RESULT(device.createGraphicsPipelines(pipelineCache, 1, &pipelineCI, nullptr, &pipelines.glowPass));

		// Skybox (cubemap)
		shaderStages[0] = loadShader(getShadersPath() + "bloom/skybox.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = loadShader(getShadersPath() + "bloom/skybox.frag.spv", vk::ShaderStageFlagBits::eFragment);
		depthStencilStateCI.depthWriteEnable = VK_FALSE;
		rasterizationStateCI.cullMode = vk::CullModeFlagBits::eFront;
		pipelineCI.renderPass = renderPass;
		VK_CHECK_RESULT(device.createGraphicsPipelines(pipelineCache, 1, &pipelineCI, nullptr, &pipelines.skyBox));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers() 
	{
		for (auto& buffer : uniformBuffers) {
			// Phong and color pass vertex shader uniform buffer
			VK_CHECK_RESULT(VKMDevice->createBuffer(vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, &buffer.scene, sizeof(ubos.scene)));
			VK_CHECK_RESULT(buffer.scene.map());
			// Blur parameters uniform buffers
			VK_CHECK_RESULT(VKMDevice->createBuffer(vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, &buffer.blurParams, sizeof(ubos.blurParams)));
			VK_CHECK_RESULT(buffer.blurParams.map());
			// Skybox
			VK_CHECK_RESULT(VKMDevice->createBuffer(vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, &buffer.skyBox, sizeof(ubos.skyBox)));
			VK_CHECK_RESULT(buffer.skyBox.map());
		}
	}

	void buildCommandBuffer()
	{
		vk::CommandBufferBeginInfo cmdBufferBeginInfo;
		vk::CommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];

		cmdBuffer.begin(cmdBufferBeginInfo);
		if (bloom)
		{
			//glowPass
			vk::ClearValue clearValue[2]{};
			vk::ClearColorValue color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
			vk::ClearDepthStencilValue depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
			clearValue[0].setColor(color);
			clearValue[1].setDepthStencil(depthStencil);

			vk::RenderPassBeginInfo renderPassBeginInfo;
			renderPassBeginInfo.setRenderPass(offscreenPass.renderPass)
				.setFramebuffer(offscreenPass.framebuffers[0].framebuffer)
				.setRenderArea({ {0,0}, { offscreenPass.width, offscreenPass.height } })
				.setClearValueCount(2)
				.setClearValues(clearValue);

			vk::Viewport viewport;
			viewport.setWidth((float)offscreenPass.width)
				.setHeight((float)offscreenPass.height)
				.setMinDepth(0.f)
				.setMaxDepth(1.f);
			cmdBuffer.setViewport(0, 1, &viewport);
			vk::Rect2D scissor = { { 0, 0},{offscreenPass.width, offscreenPass.height} };
			cmdBuffer.setScissor(0, 1, &scissor);
			//First render pass : Render glow parts of the model(separate mesh) to an offscreen frame buffer
			{
				cmdBuffer.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.scene, 0, 1, &descriptorSets[currentBuffer].scene, 0, nullptr);
				cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.glowPass);
				models.ufoGlow->draw(cmdBuffer);
				cmdBuffer.endRenderPass();
			}
			/*
				Second render pass: Vertical blur
				Render contents of the first pass into a second framebuffer and apply a vertical blur
				This is the first blur pass, the horizontal blur is applied when rendering on top of the scene
			*/
			{
				renderPassBeginInfo.framebuffer = offscreenPass.framebuffers[1].framebuffer;
				cmdBuffer.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.blur, 0, 1, &descriptorSets[currentBuffer].blurVert, 0, nullptr);
				cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.blurVert);
				cmdBuffer.draw(3, 1, 0, 0);
				cmdBuffer.endRenderPass();
			}

		}
		//Third render pass: Scene rendering with applied vertical blur
		{
			vk::ClearValue clearValue[2]{};
			vk::ClearColorValue color = vk::ClearColorValue(std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f});
			vk::ClearDepthStencilValue depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
			clearValue[0].setColor(color);
			clearValue[1].setDepthStencil(depthStencil);

			vk::RenderPassBeginInfo renderPassBeginInfo;
			renderPassBeginInfo.setRenderPass(renderPass)
				.setFramebuffer(frameBuffers[currentImageIndex])
				.setRenderArea({ {0,0}, { width, height } })
				.setClearValueCount(2)
				.setClearValues(clearValue);

			vk::Viewport viewport;
			viewport.setWidth(width)
				.setHeight(height)
				.setMinDepth(0.f)
				.setMaxDepth(1.f);
			cmdBuffer.setViewport(0, 1, &viewport);
			vk::Rect2D scissor = { { 0, 0},{width, height} };
			cmdBuffer.setScissor(0, 1, &scissor);

			cmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

			//Skybox
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.scene, 0, 1, &descriptorSets[currentBuffer].skyBox, 0, nullptr);
			cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.skyBox);
			models.skyBox->draw(cmdBuffer);

			// 3D scene
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.scene, 0, 1, &descriptorSets[currentBuffer].scene, 0, nullptr);
			cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.phongPass);
			models.ufo->draw(cmdBuffer);

			if(bloom)
			{
				// horizontal blur
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.blur, 0, 1, &descriptorSets[currentBuffer].blurHorz, 0, nullptr);
				cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.blurHorz);
				cmdBuffer.draw(3, 1, 0, 0);
			}
			//ui
			drawUI(cmdBuffer);
			cmdBuffer.endRenderPass();
		}
		cmdBuffer.end();
	}

	void updateUniformBuffers()
	{
		// UFO
		auto& camera = displayWindows.camera;
		ubos.scene.projection = camera.matrices.perspective;
		ubos.scene.view = camera.matrices.view;
		ubos.scene.model = glm::translate(glm::mat4(1.0f), glm::vec3(sin(glm::radians(timer * 360.0f)) * 0.25f, -1.0f, cos(glm::radians(timer * 360.0f)) * 0.25f));
		ubos.scene.model = glm::rotate(ubos.scene.model, -sinf(glm::radians(timer * 360.0f)) * 0.15f, glm::vec3(1.0f, 0.0f, 0.0f));
		ubos.scene.model = glm::rotate(ubos.scene.model, glm::radians(timer * 360.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		memcpy(uniformBuffers[currentBuffer].scene.mapped, &ubos.scene, sizeof(ubos.scene));

		// Skybox
		ubos.skyBox.projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 256.0f);
		ubos.skyBox.view = glm::mat4(glm::mat3(camera.matrices.view));
		ubos.skyBox.model = glm::mat4(1.0f);
		memcpy(uniformBuffers[currentBuffer].skyBox.mapped, &ubos.skyBox, sizeof(ubos.skyBox));

		// Blur parameters
		memcpy(uniformBuffers[currentBuffer].blurParams.mapped, &ubos.blurParams, sizeof(ubos.blurParams));
	}

	void loadAssets()
	{
		models.ufo.emplace(VKMDevice);
		models.ufoGlow.emplace(VKMDevice);
		models.skyBox.emplace(VKMDevice);
		cubemap = std::make_unique<vkm::TextureCubeMap>(VKMDevice);
		const uint32_t glTFLoadingFlags = vkmglTF::FileLoadingFlags::PreTransformVertices | vkmglTF::FileLoadingFlags::PreMultiplyVertexColors | vkmglTF::FileLoadingFlags::FlipY;
		models.ufo->loadFromFile(vkm::tools::getAssetPath() + "models/retroufo.gltf", queue, glTFLoadingFlags);
		models.ufoGlow->loadFromFile(vkm::tools::getAssetPath() + "models/retroufo_glow.gltf", queue, glTFLoadingFlags);
		models.skyBox->loadFromFile(vkm::tools::getAssetPath() + "models/cube.gltf", queue, glTFLoadingFlags);
		cubemap->loadFromFile(vkm::tools::getAssetPath() + "textures/cubemap_space.ktx", vk::Format::eR8G8B8A8Unorm, queue);
	}

	void prepare()
	{
		VKM_Base::prepare();
		loadAssets();
		prepareUniformBuffers();
		prepareOffscreen();
		setupDescriptors();
		preparePipelines();
		displayWindows.prepared = true;
	}
	virtual void render()
	{
		if (!displayWindows.prepared)
			return;
		VKM_Base::prepareFrame();
		updateUniformBuffers();
		buildCommandBuffer();
		VKM_Base::submitFrame();
	}
	virtual void OnUpdateHUD(vkm::HUD* overlay)
	{
		if (overlay->header("Settings")) {
			overlay->checkBox("bloom", &bloom);
			overlay->inputFloat("Scale", &ubos.blurParams.blurScale, 0.1f, 2);
		}
	}
};
VulkanExample* vulkanExample;																		
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)						
{																							
	if (vulkanExample != NULL)																	
	{																							
		vulkanExample->displayWindows.handleMessages(hWnd, uMsg, wParam, lParam);									
	}																							
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));											
}																							
#ifdef CONSOLE
int main(int argc, char** argv)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->displayWindows.setupWindow(hInstance, WndProc);
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#else
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int)
{
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->displayWindows.setupWindow(hInstance, WndProc);
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	std::cout << "Press Enter to exit..." << std::endl;
	std::cin.get();
	return 0;
}
#endif // CONSOLE

