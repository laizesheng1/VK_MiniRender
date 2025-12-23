#include "Buffer.h"

namespace vkm {
	void Buffer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags property, uint32_t MemoryTypeIndex, void* data)
	{
		assert(device);
		vk::BufferCreateInfo createInfo;
		VK_CHECK_RESULT(device.createBuffer(&createInfo, nullptr, &buffer));

		vk::MemoryRequirements memReqs;
		device.getBufferMemoryRequirements(buffer, &memReqs);
		vk::MemoryAllocateInfo memAlloc;
		memAlloc.setAllocationSize(memReqs.size)
			.setMemoryTypeIndex(MemoryTypeIndex);
		vk::MemoryAllocateFlagsInfoKHR allocFlagsInfo;
		if (usageFlags & vk::BufferUsageFlagBits::eShaderDeviceAddress)
		{
			allocFlagsInfo.setFlags(vk::MemoryAllocateFlagBits::eDeviceAddress);
			memAlloc.pNext = &allocFlagsInfo;
		}
		VK_CHECK_RESULT(device.allocateMemory(&memAlloc, nullptr, &memory));
		this->alignment = memReqs.alignment;
		this->size = size;
		this->usageFlags = usageFlags;
		if (data != nullptr)
		{
			mapped = device.mapMemory(memory, 0, size);
			memcpy(mapped, data, size);
			if (!(memoryPropertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent))
			{
				vk::MappedMemoryRange mappedRange;
				mappedRange.setMemory(memory)
					.setSize(size);
				VK_CHECK_RESULT(device.flushMappedMemoryRanges(1, &mappedRange));
			}
			unmap();
		}
		device.bindBufferMemory(buffer, memory, 0);
	}

	vkm_result Buffer::map(vk::DeviceSize size, vk::DeviceSize offset)
	{
		vk::MemoryMapFlags flags = {};
		return device.mapMemory(memory, offset, size, flags, &mapped);
	}
	void Buffer::unmap()
	{
		if (mapped)
		{
			device.unmapMemory(memory);
			mapped = nullptr;
		}
	}
}