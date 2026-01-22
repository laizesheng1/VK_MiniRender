#include "Texture.h"

namespace vkm {
	void Texture::updateDescriptor()
	{
		descriptorImageInfo.sampler = sampler;
		descriptorImageInfo.imageView = imageView;
		descriptorImageInfo.imageLayout = imageLayout;
	}

	void Texture::destroy()
	{
		device->logicalDevice.destroyImageView(imageView);
		device->logicalDevice.destroyImage(image);
		if (sampler)
		{
			device->logicalDevice.destroySampler(sampler);
		}
		device->logicalDevice.freeMemory(deviceMemory);
	}

	ktxResult Texture::loadKTXFile(std::string filename, ktxTexture** target)
	{
		ktxResult result = KTX_SUCCESS;
		if (!vkm::tools::fileExist(filename))
		{
			vkm::tools::exitFatal("Could not load texture from " + filename + "\n\nMake sure the assets submodule has been checked out and is up-to-date.", -1);
		}
		result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, target);
		return result;
	}

	//Load a 2D texture including all mip levels
	void Texture2D::loadFromFile(std::string filename, vk::Format format, vkm::VKMDevice* device, vk::Queue copyQueue, vk::ImageUsageFlags imageUsageFlags, vk::ImageLayout imageLayout)
	{
		ktxTexture* ktxTexture;
		ktxResult result = loadKTXFile(filename, &ktxTexture);
		assert(result == KTX_SUCCESS);

		this->device = device;
		width = ktxTexture->baseWidth;
		height = ktxTexture->baseHeight;
		mipLevels = ktxTexture->numLevels;

		ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

		vk::FormatProperties formatProperties = device->physicalDevice.getFormatProperties(format);
		vk::CommandBuffer copyCmd = device->createCommandBuffer(vk::CommandBufferLevel::ePrimary, true);
		
		vk::Buffer stagingBuffer;
		vk::DeviceMemory stagingMemory;
		//create and bind buffer | copy data
		device->createBuffer(vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			ktxTextureSize,
			&stagingBuffer,
			&stagingMemory,
			ktxTextureData
		);
		// Setup buffer copy regions for each mip level
		std::vector<vk::BufferImageCopy> bufferCopyRegions;
		for (uint32_t i = 0; i < mipLevels; i++) {
			ktx_size_t offset;
			KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);
			assert(result == KTX_SUCCESS);
			vk::ImageSubresourceLayers Layers;
			Layers.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setMipLevel(i)
				.setBaseArrayLayer(0)
				.setLayerCount(1);
			
			vk::BufferImageCopy bufferCopyRegion;
			bufferCopyRegion.setBufferOffset(offset)
				.setImageSubresource(Layers)
				.setImageExtent({ std::max(1u, ktxTexture->baseWidth >> i),	std::max(1u, ktxTexture->baseHeight >> i), 1 });
			bufferCopyRegions.push_back(bufferCopyRegion);
		}

		// Create optimal tiled target image
		vk::ImageCreateInfo imageCreateInfo;
		imageCreateInfo.setImageType(vk::ImageType::e2D)
			.setFormat(format)
			.setExtent({ width,height,1 })
			.setMipLevels(mipLevels)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(imageUsageFlags)
			.setSharingMode(vk::SharingMode::eExclusive)
			.setInitialLayout(vk::ImageLayout::eUndefined);
		if (!(imageCreateInfo.usage & vk::ImageUsageFlagBits::eTransferDst)) {
			imageCreateInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
		}
		VK_CHECK_RESULT(device->logicalDevice.createImage(&imageCreateInfo, nullptr, &image));

		vk::MemoryRequirements memReqs = device->logicalDevice.getImageMemoryRequirements(image);
		vk::MemoryAllocateInfo memAllocInfo;
		memAllocInfo.setAllocationSize(memReqs.size);
		memAllocInfo.setMemoryTypeIndex(device->queryMemTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible));
		VK_CHECK_RESULT(device->logicalDevice.allocateMemory(&memAllocInfo, nullptr, &deviceMemory));
		device->logicalDevice.bindImageMemory(image, deviceMemory, 0);
		vk::ImageSubresourceRange subresourceRange;
		subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)
			.setLayerCount(1)
			.setLevelCount(mipLevels);
		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy
		vkm::tools::setImageLayout(
			copyCmd,
			image,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal,
			subresourceRange
		);
		// Copy mip levels from staging buffer
		copyCmd.copyBufferToImage(
			stagingBuffer,
			image,
			vk::ImageLayout::eTransferDstOptimal,
			bufferCopyRegions
		);
		// Change texture image layout to shader read after all mip levels have been copied
		this->imageLayout = imageLayout;
		vkm::tools::setImageLayout(
			copyCmd,
			image,
			vk::ImageLayout::eTransferDstOptimal,
			imageLayout,
			subresourceRange
		);
		device->flushCommandBuffer(copyCmd, copyQueue);
		//clean up
		device->logicalDevice.destroyBuffer(stagingBuffer, nullptr);
		device->logicalDevice.freeMemory(stagingMemory, nullptr);
		ktxTexture_Destroy(ktxTexture);

		// Create a default sampler
		vk::SamplerCreateInfo samplerCreateInfo;
		samplerCreateInfo.setMagFilter(vk::Filter::eLinear)
			.setMinFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat)
			.setMipLodBias(0.f)
			.setAnisotropyEnable(device->enabledFeatures.samplerAnisotropy)
			.setMaxAnisotropy(device->enabledFeatures.samplerAnisotropy ? device->properties.limits.maxSamplerAnisotropy : 1.0f)
			.setCompareOp(vk::CompareOp::eNever)
			.setMinLod(0.f)
			.setMaxLod((float)mipLevels)
			.setBorderColor(vk::BorderColor::eFloatOpaqueWhite);
		VK_CHECK_RESULT(device->logicalDevice.createSampler(&samplerCreateInfo, nullptr, &sampler));
		// Create image view
		vk::ImageViewCreateInfo viewCreateInfo;
		subresourceRange.setBaseArrayLayer(0);
		viewCreateInfo.setImage(image)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(format)
			.setSubresourceRange(subresourceRange);
		VK_CHECK_RESULT(device->logicalDevice.createImageView(&viewCreateInfo, nullptr, &imageView));
		
		updateDescriptor();
	}

	//
	void Texture2D::fromBuffer(void* buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight, vkm::VKMDevice* device, vk::Queue copyQueue, vk::Filter filter, vk::ImageUsageFlags imageUsageFlags, vk::ImageLayout imageLayout)
	{

	}
}