#include "VKMDevice.h"

namespace vkm {
	VKMDevice::VKMDevice(vk::PhysicalDevice physicalDevice)
	{
		assert(physicalDevice);
		this->physicalDevice = physicalDevice;
		properties = physicalDevice.getProperties();
		features = physicalDevice.getFeatures();
		memoryProperties = physicalDevice.getMemoryProperties();

		queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

		auto  extensions = physicalDevice.enumerateDeviceExtensionProperties();
		for (auto& ext : extensions)
		{
			supportedExtensions.push_back(ext.extensionName);
		}
	}

	VKMDevice::~VKMDevice()
	{
		if (commandPool)
			logicalDevice.destroyCommandPool(commandPool);
		if (logicalDevice)
			logicalDevice.destroy();
	}

	vkm_result VKMDevice::createLogicalDevice(vk::PhysicalDeviceFeatures enabledFeatures, std::vector<const char*> enabledExtensions, void* pNextChain, bool useSwapChain, vk::QueueFlags requestedQueueTypes)
	{
		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
		const float defaultQueuePriority = 0.0f;
		// Graphics queue
		if (requestedQueueTypes & vk::QueueFlagBits::eGraphics)
		{
			queueFamilyIndices.graphics = getQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
			
			vk::DeviceQueueCreateInfo queueInfo;
			queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
		else
		{
			queueFamilyIndices.graphics = 0;
		}
		// compute queue
		if (requestedQueueTypes & vk::QueueFlagBits::eCompute)
		{
			queueFamilyIndices.compute = getQueueFamilyIndex(vk::QueueFlagBits::eCompute);
			if (queueFamilyIndices.compute != queueFamilyIndices.graphics)
			{
				// If compute family index differs, we need an additional queue create info for the compute queue
				vk::DeviceQueueCreateInfo queueInfo;
				queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				queueCreateInfos.push_back(queueInfo);
			}
		}
		else
		{
			queueFamilyIndices.compute = queueFamilyIndices.graphics;
		}
		//tranfer
		if (requestedQueueTypes & vk::QueueFlagBits::eTransfer)
		{
			queueFamilyIndices.transfer = getQueueFamilyIndex(vk::QueueFlagBits::eTransfer);
			if (queueFamilyIndices.transfer != queueFamilyIndices.graphics)
			{
				// If compute family index differs, we need an additional queue create info for the compute queue
				vk::DeviceQueueCreateInfo queueInfo;
				queueInfo.queueFamilyIndex = queueFamilyIndices.transfer;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				queueCreateInfos.push_back(queueInfo);
			}
		}
		else
		{
			queueFamilyIndices.transfer = queueFamilyIndices.graphics;
		}
		std::vector<const char*> deviceExtensions(enabledExtensions);
		if (useSwapChain)
		{
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}
		vk::DeviceCreateInfo createInfo;
		createInfo.setQueueCreateInfos(queueCreateInfos);

		//TODO:
		vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2{};
		if (pNextChain) {
			physicalDeviceFeatures2.sType = vk::StructureType::ePhysicalDeviceFeatures2;
			physicalDeviceFeatures2.features = enabledFeatures;
			physicalDeviceFeatures2.pNext = pNextChain;
			createInfo.pEnabledFeatures = nullptr;
			createInfo.pNext = &physicalDeviceFeatures2;
		}

		if (deviceExtensions.size() > 0)
		{
			for (const char* enabledExtension : deviceExtensions)
			{
				if (!extensionSupported(enabledExtension)) {
					OutputMessage("Enabled device extension {} is not present at device level", enabledExtension);
				}
			}
			createInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

		this->enabledFeatures = enabledFeatures;

		vk::Result result = physicalDevice.createDevice(&createInfo,nullptr,&logicalDevice);
		if (result != vk::Result::eSuccess)
		{
			OutputMessage("[ VKM_DEVICE ] ERROR\nFailed to create logical device!\nError code: {}\n", int32_t(result));
			return result;
		}
		commandPool = createCommandPool(queueFamilyIndices.graphics);

		return result;
	}

	uint32_t VKMDevice::getQueueFamilyIndex(vk::QueueFlags queueFlags) const
	{
		//for compute but not graphics
		if ((queueFlags & vk::QueueFlagBits::eCompute) == queueFlags)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
			{
				if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute) && (!(queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)))
				{
					return i;
				}
			}
		}
		//only for transfer
		if ((queueFlags & vk::QueueFlagBits::eTransfer) == queueFlags)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
			{
				if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer) && (!(queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)) && (!(queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute)))
				{
					return i;
				}
			}
		}

		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
		{
			if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute) && (!(queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)))
			{
				return i;
			}
		}
		OutputMessage("Could not find a matching queue family index!");
	}

	bool VKMDevice::extensionSupported(std::string extension)
	{
		return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end());
	}

	vk::CommandPool VKMDevice::createCommandPool(uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags createFlags)
	{
		vk::CommandPoolCreateInfo createInfo;
		createInfo.setQueueFamilyIndex(queueFamilyIndex)
			.setFlags(createFlags);

		vk::CommandPool cmdPool;
		auto result = logicalDevice.createCommandPool(&createInfo, nullptr, &cmdPool);
		if (result != vk::Result::eSuccess)
		{
			OutputMessage("[ VKM_DEVICE ] ERROR\nFailed to create cmdPool!\nError code: {}\n", int32_t(result));
		}
		return cmdPool;
	}
}
