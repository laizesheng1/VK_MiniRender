#pragma once
#include <fstream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "vulkan/vulkan.h"

#include <ktx.h>
#include <ktxvulkan.h>

#include "Buffer.h"
#include "VKMDevice.h"
#include "VKM_Tools.h"

namespace vkm {

class Texture {
 public:
	 vkm::VKMDevice* device;
	 vk::Image image;
	 vk::ImageLayout imageLayout;
	 vk::DeviceMemory deviceMemory;
	 vk::ImageView imageView;
	 vk::Sampler sampler;
	 vk::DescriptorImageInfo descriptorImageInfo;
	 uint32_t width, height;
	 uint32_t mipLevels;
	 uint32_t layerCount;

	 void updateDescriptor();
	 void destroy();
	 ktxResult loadKTXFile(std::string filename, ktxTexture** target);
};

class Texture2D :public Texture
{
public:
	void loadFromFile(
		std::string filename,
		vk::Format format,
		vkm::VKMDevice* device,
		vk::Queue            copyQueue,
		vk::ImageUsageFlags  imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
		vk::ImageLayout      imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
	);

	void fromBuffer(
		void* buffer,
		VkDeviceSize       bufferSize,
		VkFormat           format,
		uint32_t           texWidth,
		uint32_t           texHeight,
		vkm::VKMDevice*    device,
		vk::Queue            copyQueue,
		vk::Filter           filter = vk::Filter::eLinear,
		vk::ImageUsageFlags  imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
		vk::ImageLayout      imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
	);
};

}