#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <sstream>
#include <iomanip>

#include <vulkan/vulkan.h>
#include "VKM_Tools.h"
#include "Buffer.h"
#include "VKMDevice.h"

#include "../external/imgui/imgui.h"

namespace vkm {
	class HUD {
	private:

	protected:
		vk::Queue queue;
		vk::SampleCountFlagBits rasterizationSamples = vk::SampleCountFlagBits::e1;


	public:
		vkm::VKMDevice* device = nullptr;

		HUD();
		~HUD();
		void text(const char* formatstr, ...);
	};
}