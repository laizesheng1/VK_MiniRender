#pragma once
#ifndef TEXTURE_H
#define TEXTURE_H

#include <fstream>
#include <stdlib.h>
#include <string>
#include <vector>

#include <ktx.h>
#include <ktxvulkan.h>
#define TINYGLTF_NO_STB_IMAGE_WRITE			
#include "tiny_gltf.h"

#include "Buffer.h"
#include "VKMDevice.h"
#include "VKM_Tools.h"

namespace vkm {

class Texture {
 public:
	 vk::Image image;
	 vk::ImageLayout imageLayout;
	 vk::DeviceMemory deviceMemory;
	 vk::ImageView imageView;
	 vk::Sampler sampler;
	 vk::DescriptorImageInfo descriptorImageInfo;
	 uint32_t width, height;
	 uint32_t mipLevels;
	 uint32_t layerCount;
	 Texture() = default;
	 Texture(vkm::VKMDevice* device) :device(device) {}
	 void updateDescriptor();
	 void destroy();
	 ktxResult loadKTXFile(std::string filename, ktxTexture** target);
	 void initImageCreateInfo(vk::ImageCreateInfo& imageCreateInfo, vk::Format format, const vk::ImageUsageFlags& imageUsageFlags, uint32_t arrayLevels = 1);
	 void allocImageDeviceMem(vk::MemoryPropertyFlagBits property = vk::MemoryPropertyFlagBits::eDeviceLocal);
	 void CreateDefaultSampler(vk::SamplerAddressMode samplerMipmapMode = vk::SamplerAddressMode::eRepeat);
	 void CreateImageview(vk::ImageSubresourceRange& subresourceRange, vk::Format format, vk::ImageViewType viewType = vk::ImageViewType::e2D);
protected:
	vkm::VKMDevice* device;
};

class Texture2D :public Texture
{
public:
	Texture2D() = default;
	Texture2D(vkm::VKMDevice* device) :Texture(device) {};
	uint32_t index;			//use in glTFModel

	void loadFromFile(
		std::string filename,
		vk::Format format,
		vk::Queue            copyQueue,
		vk::ImageUsageFlags  imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
		vk::ImageLayout      imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
		bool				 updateDes = false
	);


	void fromBuffer(
		void* buffer,
		vk::DeviceSize       bufferSize,
		vk::Format           format,
		uint32_t           texWidth,
		uint32_t           texHeight,
		vk::Queue            copyQueue,
		vk::Filter           filter = vk::Filter::eLinear,
		vk::ImageUsageFlags  imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
		vk::ImageLayout      imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
	);

	void fromglTFimage(tinygltf::Image& gltfimage, std::string path, vk::Queue copyQueue);
};

class TextureCubeMap :public Texture {
public:
	TextureCubeMap() = default;
	TextureCubeMap(vkm::VKMDevice* device) :Texture(device) {};
	void loadFromFile(
		std::string filename,
		vk::Format format,
		vk::Queue            copyQueue,
		vk::ImageUsageFlags  imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
		vk::ImageLayout      imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
	);
};

}
#endif // !TEXTURE_H