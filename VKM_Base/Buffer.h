#pragma once
#include "VKM_Tools.h"


namespace vkm {
	struct Buffer {
	public:
		vk::Device device;
		vk::DescriptorBufferInfo descriptor;
		vk::DeviceSize size = 0;
		vk::DeviceSize alignment = 0;
		vk::BufferUsageFlags usageFlags;
		vk::MemoryPropertyFlags memoryPropertyFlags;
		uint32_t memoryTypeIndex;

		vk::Buffer buffer = VK_NULL_HANDLE;
		vk::DeviceMemory memory = VK_NULL_HANDLE;
		uint64_t deviceAddress;

		void* mapped = nullptr;
		void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags property, uint32_t MemoryTypeIndex, void* data);
		vkm_result map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
		void unmap();
	};
}