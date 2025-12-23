#include "VK_Base.h"

VKM_Base* VKM_Base::singleton = nullptr;

VKM_Base::VKM_Base()
{
	width = 1280;
	height = 720;

}

VKM_Base::~VKM_Base()
{
	delete VKMDevice;
}

void VKM_Base::prepare()
{
	createSurface();
	createCmdPool();
	createSwapChain();
	createCmdBuffer();
	InitializedSync();
	InitDefaultDepthStencil();
	InitRenderPass();
	createPipelineCache();
	InitFrameBuffer();

}

bool VKM_Base::initVulkan()
{
	vk::Result result = createInstance();
	// Physical device
	auto devices = instance.enumeratePhysicalDevices();
	if (devices.size()==0)
	{
		OutputMessage("No device with Vulkan support found!");
		return false;
	}
	physicalDevice = devices[0];
	if (physicalDevice == nullptr)
	{
		OutputMessage("failed to find a suitable GPU!");
		return false;
	}
	physicalDeviceProperties = physicalDevice.getProperties();

	std::cout << "Renderer: " << physicalDeviceProperties.deviceName << std::endl;

	PhysicalDeviceFeatures = physicalDevice.getFeatures();
	physicalDeviceMemoryProperties= physicalDevice.getMemoryProperties();

	getEnabledFeatures();

	VKMDevice = new vkm::VKMDevice(physicalDevice);
	getEnabledExtensions();

	result = VKMDevice->createLogicalDevice(enbaleFeatures, enabledDeviceExtensions, deviceCreatepNextChain);
	if (result != vk::Result::eSuccess)
	{
		OutputMessage("[ VK_Base ] ERROR\nFailed to create logical device!\nError code: {}\n", int32_t(result));
		return false;
	}
	device = VKMDevice->logicalDevice;
	queue = device.getQueue(VKMDevice->queueFamilyIndices.graphics, 0);
	//Find a suitable depth and/or stencil format
	vk::Bool32 validFormat = false;
	if (requireStencil) {
		validFormat = vkm::tools::getSupportedDepthStencilFormat(physicalDevice, &depthFormat);
	}
	else {
		validFormat = vkm::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);
	}

	if(createSurface_callback)
	{
		vk::SurfaceKHR surface = createSurface_callback(instance);
		swapChain.Surface(surface);
	}
	swapChain.setContext(instance, physicalDevice, device);
	return true;
}

VKM_Base& VKM_Base::Get()
{
	if (singleton == nullptr)
		singleton = new VKM_Base();
	return *singleton;
}

void VKM_Base::createSurface()
{
	swapChain.initSurface();
}

void VKM_Base::createCmdPool()
{
	vk::CommandPoolCreateInfo cmdPoolInfo;
	cmdPoolInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
		.setQueueFamilyIndex(swapChain.queueNodeIndex);
	VK_CHECK_RESULT(device.createCommandPool(&cmdPoolInfo, nullptr,&cmdPool));
}

void VKM_Base::createSwapChain()
{
	swapChain.CreateSwapchain(width, height, false);
}

void VKM_Base::createCmdBuffer()
{
	vk::CommandBufferAllocateInfo cmdBufAllocateInfo;
	cmdBufAllocateInfo.setCommandPool(cmdPool)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount(static_cast<uint32_t>(drawCmdBuffers.size()));
	VK_CHECK_RESULT(device.allocateCommandBuffers(&cmdBufAllocateInfo, drawCmdBuffers.data()));
}

void VKM_Base::InitializedSync()
{
	// Wait fences to sync command buffer access
	vk::FenceCreateInfo fenceCreateInfo;
	fenceCreateInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

	for (auto& fence : waitFences) {
		VK_CHECK_RESULT(device.createFence(&fenceCreateInfo, nullptr, &fence));
	}
	// Used to ensure that image presentation is complete before starting to submit again
	for (auto& semaphore : imageAvaliableSemaphores) {
		vk::SemaphoreCreateInfo semaphoreCreateInfo;
		VK_CHECK_RESULT(device.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore));
	}
	// Semaphore used to ensure that all commands submitted have been finished before submitting the image to the queue
	renderCompleteSemaphores.resize(swapChain.images.size());
	for (auto& semaphore : renderCompleteSemaphores) {
		vk::SemaphoreCreateInfo semaphoreCreateInfo;
		VK_CHECK_RESULT(device.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore));
	}
}

void VKM_Base::InitDefaultDepthStencil()
{
	vk::ImageCreateInfo createInfo;
	createInfo.setArrayLayers(1)
		.setExtent({ width, height, 1 })
		.setFormat(depthFormat)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setMipLevels(1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setImageType(vk::ImageType::e2D);
	//create image | alloc memory | bind image to memory
	VK_CHECK_RESULT(device.createImage(&createInfo, nullptr, &depthStencil.image));
	vk::MemoryRequirements memReqs=device.getImageMemoryRequirements(depthStencil.image);
	vk::MemoryAllocateInfo memAllocInfo;
	memAllocInfo.setAllocationSize(memReqs.size)
		.setMemoryTypeIndex(VKMDevice->queryMemTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
	depthStencil.memory = device.allocateMemory(memAllocInfo);
	VK_CHECK_RESULT(device.allocateMemory(&memAllocInfo, nullptr, &depthStencil.memory));			
	device.bindImageMemory(depthStencil.image, depthStencil.memory, 0);
	//create imageview
	vk::ImageViewCreateInfo imageViewCreateInfo;
	vk::ImageSubresourceRange range;
	range.setAspectMask(vk::ImageAspectFlagBits::eDepth)
		.setBaseArrayLayer(0)
		.setBaseMipLevel(0)
		.setLayerCount(1)
		.setLevelCount(1);
	imageViewCreateInfo.setImage(depthStencil.image)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(depthFormat)
		.setSubresourceRange(range);
	if (depthFormat >= vk::Format::eD16UnormS8Uint)
	{
		imageViewCreateInfo.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
	}
	VK_CHECK_RESULT(device.createImageView(&imageViewCreateInfo, nullptr, &depthStencil.view));
}

void VKM_Base::InitRenderPass()
{
	vk::AttachmentDescription ColorattachDes;
	ColorattachDes.setFormat(swapChain.colorFormat)			
		.setInitialLayout(vk::ImageLayout::eUndefined)					
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)			
		.setLoadOp(vk::AttachmentLoadOp::eClear)			
		.setStoreOp(vk::AttachmentStoreOp::eStore)			
		.setSamples(vk::SampleCountFlagBits::e1)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)			
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	vk::AttachmentDescription depthAttachment;
	depthAttachment.setFormat(depthFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eClear)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	std::array<vk::AttachmentDescription, 2> attachments = { ColorattachDes ,depthAttachment };

	vk::AttachmentReference colorReference;
	colorReference.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depthReference;
	depthReference.setAttachment(1)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	vk::SubpassDescription subpassDes;
	subpassDes.setColorAttachments(colorReference)			//设置的颜色附着在数组中的索引会被片段着色器使用
		.setColorAttachmentCount(1)
		.setPDepthStencilAttachment(&depthReference);

	//dependencies for layout transitions
	std::array<vk::SubpassDependency, 2> dependencies{};
	dependencies[0].setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests)
		.setDstStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests)
		.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
		.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead);

	dependencies[1].setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlagBits::eNone)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);
	vk::RenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.setAttachments(attachments)
		.setSubpasses(subpassDes)
		.setDependencies(dependencies);
	VK_CHECK_RESULT(device.createRenderPass(&renderPassCreateInfo, nullptr, &renderPass));
}

void VKM_Base::createPipelineCache()
{
	vk::PipelineCacheCreateInfo pipelineCacheCreateInfo;
	VK_CHECK_RESULT(device.createPipelineCache(&pipelineCacheCreateInfo, nullptr, &pipelineCache));
}

void VKM_Base::InitFrameBuffer()
{
	frameBuffers.resize(swapChain.images.size());
	for (uint32_t i = 0; i < frameBuffers.size(); i++) {
		const vk::ImageView attachments[2] = { swapChain.imageViews[i], depthStencil.view };
		vk::FramebufferCreateInfo frameBufferCreateInfo;
		frameBufferCreateInfo.setRenderPass(renderPass)
			.setAttachments(attachments)
			.setWidth(width)
			.setHeight(height)
			.setLayers(1);
		VK_CHECK_RESULT(device.createFramebuffer(&frameBufferCreateInfo, nullptr, &frameBuffers[i]));
	}
}

vkm_result VKM_Base::createInstance()
{
    std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
    instanceExtensions.push_back("VK_KHR_win32_surface");

	// Get extensions supported by the instance and store for later use
	std::vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties();
	if (extensions.size() > 0)
	{
		for (VkExtensionProperties& extension : extensions)
		{
			supportedInstanceExtensions.push_back(extension.extensionName);
		}
	}
	// Enabled requested instance extensions
	if (!enabledInstanceExtensions.empty())
	{
		for (const char* enabledExtension : enabledInstanceExtensions)
		{
			// Output message if requested extension is not available
			if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), enabledExtension) == supportedInstanceExtensions.end())
			{
				std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
			}
			instanceExtensions.push_back(enabledExtension);
		}
	}

    std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };

    vk::ApplicationInfo applicationInfo;
	applicationInfo.setApiVersion(apiVersion)
		.setPEngineName(name.c_str())
		.setPApplicationName(name.c_str());

    vk::InstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo.setPApplicationInfo(&applicationInfo)
		.setEnabledExtensionCount(instanceExtensions.size())
		.setPpEnabledExtensionNames(instanceExtensions.data())
		.setPpEnabledLayerNames(layers.data());
   
    if (vk::Result result = vk::createInstance(&instanceCreateInfo, nullptr, &instance); result != vk::Result::eSuccess) {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a vulkan instance!\nError code: {}\n", int32_t(result));
        return result;
    }
    return vk::Result::eSuccess;
}

void VKM_Base::getEnabledFeatures()
{

}

void VKM_Base::getEnabledExtensions()
{
}

void VKM_Base::AddLayerOrExtension(std::vector<const char*>& container, const char* name)
{
	for (auto& i : container)
		if (!strcmp(name, i))
			return;
	container.push_back(name);
}

void VKM_Base::AddInstanceExtensions(const char* extension)
{
	AddLayerOrExtension(enabledDeviceExtensions, extension);
}

void VKM_Base::SetCreateSurface(CreateSurfaceCallback createSurface)
{
	this->createSurface_callback = createSurface;
}

