#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "vulkan/vulkan.hpp"

#include <VKM_Tools.h>
#include "Buffer.h"
#include "VKMDevice.h"
#include "Swapchain.h"

#define DestroyHandleBy(Func) if (handle) { Func(VKM_Base::Get().Device(), handle, nullptr); handle = VK_NULL_HANDLE; }
#define MoveHandle handle = other.handle; other.handle = VK_NULL_HANDLE;
#define DefineHandleTypeOperator operator decltype(handle)() const { return handle; }
#define DefineAddressFunction const decltype(handle)* Address() const { return &handle; }

constexpr uint32_t maxConcurrentFrames = 2;

class VKM_Base {
	using CreateSurfaceCallback = std::function<vk::SurfaceKHR(vk::Instance)>;
private:
	static VKM_Base* singleton;
	void createSurface();
	void createCmdPool();
	void createSwapChain();
	void createCmdBuffer();
	void InitializedSync();
	void InitDefaultDepthStencil();
	void InitRenderPass();
	void createPipelineCache();
	void InitFrameBuffer();

protected:
	vk::Instance instance= VK_NULL_HANDLE;
	std::vector<std::string> supportedInstanceExtensions;

	vk::PhysicalDevice physicalDevice = VK_NULL_HANDLE;
	vk::PhysicalDeviceProperties physicalDeviceProperties{};
	vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{};
	vk::PhysicalDeviceFeatures PhysicalDeviceFeatures{};
	vk::PhysicalDeviceFeatures enbaleFeatures{};
	std::vector<vk::PhysicalDevice> availablePhysicalDevices;

	std::vector<const char*> enabledDeviceExtensions;			//for create vk::device
	std::vector<const char*> enabledInstanceExtensions;

	void* deviceCreatepNextChain = nullptr;

	vk::Device device = VK_NULL_HANDLE;
	vk::Queue queue = VK_NULL_HANDLE;
	vk::Format depthFormat = vk::Format::eUndefined;
	vk::CommandPool cmdPool = VK_NULL_HANDLE;
	std::array<vk::CommandBuffer, maxConcurrentFrames> drawCmdBuffers;
	vk::RenderPass renderPass = VK_NULL_HANDLE;
	vk::PipelineCache pipelineCache;
	std::vector<vk::Framebuffer>frameBuffers;

	SwainChain swapChain;
	CreateSurfaceCallback createSurface_callback;

	std::array<vk::Semaphore, maxConcurrentFrames> imageAvaliableSemaphores{};		//before present
	std::vector<vk::Semaphore> renderCompleteSemaphores{};
	std::array<vk::Fence, maxConcurrentFrames> waitFences;

	bool requireStencil = false;


public:
	uint32_t width, height;
	vkm::VKMDevice* VKMDevice{};
	std::string name = "VKM";
	uint32_t apiVersion;
	//Default depth stencil attachment used by the default render pass
	struct {
		vk::Image image;
		vk::DeviceMemory memory;
		vk::ImageView view;
	} depthStencil{};

	VKM_Base();
	VKM_Base(VKM_Base&&) = delete;
	~VKM_Base();
	bool initVulkan();
	static VKM_Base& Get();
	vk::Device Device() const { return device; }


	virtual vkm_result createInstance();
	virtual void getEnabledFeatures();
	virtual void getEnabledExtensions();
	virtual void prepare();
	
public:
	//Gretter
	void AddLayerOrExtension(std::vector<const char*>& container, const char* name);
	void AddInstanceExtensions(const char* extension);
	void SetCreateSurface(CreateSurfaceCallback createSurface);

};