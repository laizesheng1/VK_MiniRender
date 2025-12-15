#pragma once

#include <math.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <stack>
#include <map>
#include <unordered_map>
#include <span>
#include <memory>
#include <functional>
#include <concepts>
#include <format>
#include <chrono>
#include <numeric>
#include <numbers>
#include <print>
//glm
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if defined(_WIN32)
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <vulkan/vulkan.hpp>
#endif

#define VK_RESULT_THROW

#ifdef VK_RESULT_THROW
class vkm_result {
	vk::Result result;
public:
	static void (*callback_func)(vk::Result);
	vkm_result(vk::Result result): result(result){}
	vkm_result(vkm_result&& other) noexcept :result(other.result) {}
	~vkm_result() noexcept(false) {
		if (uint32_t(result) < VK_RESULT_MAX_ENUM)
			return;
		if (callback_func)
			callback_func(result);
		throw result;
	}
	operator vk::Result()
	{
		vk::Result result = this->result;
		this->result = vk::Result::eSuccess;
		return result;
	}
};
inline void (*vkm_result::callback_func)(vk::Result);
#else
using vkm_result = vk::Result;
#endif // VK_RESULT_THROW

#define ExecuteOnce(...) { static bool executed = false; if (executed) return __VA_ARGS__; executed = true; }

template<typename... Ts>
void OutputMessage(const std::format_string<Ts...> format, Ts&&... arguments) {
	std::print(format, std::forward<Ts>(arguments)...);
}

#define VK_CHECK_RESULT(f)																				\
{																										\
	vk::Result res = (f);																					\
	if (res != vk::Result::eSuccess)																				\
	{																									\
		OutputMessage("Fatal : Result is false. Error code: {} in {} at line {}\n", int32_t(res),__FILE__,__LINE__); \
		assert(res == vk::Result::eSuccess);																		\
	}																									\
}


namespace vkm {
	namespace tools {
		vk::Bool32 getSupportedDepthFormat(vk::PhysicalDevice physicalDevice, vk::Format* depthFormat);
		vk::Bool32 getSupportedDepthStencilFormat(vk::PhysicalDevice physicalDevice, vk::Format* depthStencilFormat);
		
	}
}