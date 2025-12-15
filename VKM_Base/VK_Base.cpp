#include "VK_Base.h"

VKM_Base* VKM_Base::singleton = nullptr;

VKM_Base::VKM_Base()
{
	
}

VKM_Base::~VKM_Base()
{
	delete VKMDevice;
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

void VKM_Base::createSwapChain()
{
	swapChain.CreateSwapchain(width, height, false);
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

void VKM_Base::prepare()
{
	createSurface();
	createSwapChain();
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

