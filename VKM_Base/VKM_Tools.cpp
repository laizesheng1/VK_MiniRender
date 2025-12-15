#include "VKM_Tools.h"

namespace vkm {
    namespace tools
    {
		vk::Bool32 getSupportedDepthFormat(vk::PhysicalDevice physicalDevice, vk::Format* depthFormat)
		{
			std::vector<vk::Format> formatList = {
				vk::Format::eD32Sfloat,
				vk::Format::eD32SfloatS8Uint,
				vk::Format::eD24UnormS8Uint,
				vk::Format::eD16Unorm,
				vk::Format::eD16UnormS8Uint
			};

			for (auto& format : formatList)
			{
				vk::FormatProperties formatProps;
				physicalDevice.getFormatProperties(format, &formatProps);
				if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
				{
					*depthFormat = format;
					return true;
				}
			}

			return false;
		}
		vk::Bool32 getSupportedDepthStencilFormat(vk::PhysicalDevice physicalDevice, vk::Format* depthStencilFormat)
		{
			std::vector<vk::Format> formatList = {
				vk::Format::eD32SfloatS8Uint,
				vk::Format::eD24UnormS8Uint,
				vk::Format::eD16UnormS8Uint
			};

			for (auto& format : formatList)
			{
				vk::FormatProperties formatProps;
				physicalDevice.getFormatProperties(format, &formatProps);
				if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
				{
					*depthStencilFormat = format;
					return true;
				}
			}

			return false;
		}
    }
}