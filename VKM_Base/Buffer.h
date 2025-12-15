#pragma once
#include "VKM_Tools.h"

namespace vkm {
	class Buffer {
		vk::Device device;
		vk::Buffer buffer = VK_NULL_HANDLE;
		vk::DeviceMemory memory = VK_NULL_HANDLE;
		vk::DescriptorBufferInfo descriptor;
		vk::DeviceSize size = 0;
		vk::DeviceSize alignment = 0;
		void* mapped = nullptr;

		vk::BufferUsageFlags usageFlags;

		vk::MemoryPropertyFlags memoryPropertyFlags;
		uint64_t deviceAddress;
		vkm_result map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
		void unmap();
		vkm_result bind(vk::DeviceSize offset = 0);
		void setupDescriptor(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
		void copyTo(void* data, vk::DeviceSize size);
		vkm_result flush(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
		vkm_result invalidate(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
		void destroy();
	};
}