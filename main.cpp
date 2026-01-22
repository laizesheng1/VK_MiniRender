#include "VKM_Base/VK_Base.h"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

//GLFWwindow* pWindow;
//GLFWmonitor* pMonitor;
//const char* windowTitle = "MiniRender";
//
//bool InitializeWindow(VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true) {
//
//	if (!glfwInit()) {
//		std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n");
//		return false;
//	}
//	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//	glfwWindowHint(GLFW_RESIZABLE, isResizable);
//	pMonitor = glfwGetPrimaryMonitor();
//	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
//	pWindow = fullScreen ?
//		glfwCreateWindow(pMode->width, pMode->height, windowTitle, pMonitor, nullptr) :
//		glfwCreateWindow(size.width, size.height, windowTitle, nullptr, nullptr);
//	if (!pWindow) {
//		std::cout << std::format("[ InitializeWindow ]\nFailed to create a glfw window!\n");
//		glfwTerminate();
//		return false;
//	}
//	/*TODO*/
//	
//	VKM_Base::Get().SetCreateSurface([](vk::Instance instance) {
//		VkSurfaceKHR surface;
//		if (glfwCreateWindowSurface(instance, pWindow, nullptr, &surface) != VK_SUCCESS)
//		{
//			throw std::runtime_error("create surface is failed");
//		}
//		return surface;
//		});
//	
//
//	return true;
//}
//
//void TerminateWindow() {
//	glfwTerminate();
//}
//void MakeWindowFullScreen() {
//	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
//	glfwSetWindowMonitor(pWindow, pMonitor, 0, 0, pMode->width, pMode->height, pMode->refreshRate);
//}
//void MakeWindowWindowed(VkOffset2D position, VkExtent2D size) {
//	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
//	glfwSetWindowMonitor(pWindow, nullptr, position.x, position.y, size.width, size.height, pMode->refreshRate);
//}
//void TitleFps() {
//	static double time0 = glfwGetTime();
//	static double time1;
//	static double dt;
//	static int dframe = -1;
//	static std::stringstream info;
//	time1 = glfwGetTime();
//	dframe++;
//	if ((dt = time1 - time0) >= 1) {
//		info.precision(1);
//		info << windowTitle << "    " << std::fixed << dframe / dt << " FPS";
//		glfwSetWindowTitle(pWindow, info.str().c_str());
//		info.str("");
//		time0 = time1;
//		dframe = 0;
//	}
//}

//int main() {
//	auto& singleton = VKM_Base::Get();
//
//	if (!InitializeWindow({ 1280, 720 }))
//		return -1;
//	
//	singleton.initVulkan();
//	singleton.prepare();
//
//	while (!glfwWindowShouldClose(pWindow)) {
//
//		/*äÖÈ¾¹ý³Ì£¬´ýÌî³ä*/
//
//		glfwPollEvents();
//		TitleFps();
//	}
//	TerminateWindow();
//	delete(&singleton);
//	return 0;
//	
//}
class VulkanExample :public VKM_Base
{
private:
	bool test = true;
public:
	VulkanExample() :VKM_Base()
	{
		displayWindows.title = "HUD Test";
		timerSpeed *= 0.5f;
		auto& camera = displayWindows.camera;
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -10.25f));
		camera.setRotation(glm::vec3(7.5f, -343.0f, 0.0f));
		camera.setPerspective(45.0f, (float)width / (float)height, 0.1f, 256.0f);
	}
	~VulkanExample()
	{
		
	}

	void buildCommandBuffer()
	{
		vk::CommandBufferBeginInfo cmdBufferBeginInfo;
		vk::CommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];

		cmdBuffer.begin(cmdBufferBeginInfo);

		vk::ClearValue clearValue[2]{};
		vk::ClearColorValue color = vk::ClearColorValue(std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f});
		vk::ClearDepthStencilValue depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
		clearValue[0].setColor(color);
		clearValue[1].setDepthStencil(depthStencil);

		vk::RenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.setRenderPass(renderPass)
			.setFramebuffer(frameBuffers[currentImageIndex])
			.setRenderArea({ {0,0}, { width, height } })
			.setClearValueCount(2)
			.setClearValues(clearValue);

		cmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		drawUI(cmdBuffer);
		cmdBuffer.endRenderPass();

		cmdBuffer.end();
	}

	void prepare()
	{
		VKM_Base::prepare();
		displayWindows.prepared = true;
	}
	virtual void render()
	{
		if (!displayWindows.prepared)
			return;
		VKM_Base::prepareFrame();
		buildCommandBuffer();
		VKM_Base::submitFrame();
	}
	virtual void OnUpdateHUD(vkm::HUD* overlay)
	{
		if (overlay->header("Settings")) {
			overlay->checkBox("Test", &test);
		}
	}
};
VulkanExample* vulkanExample;																		
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)						
{																							
	if (vulkanExample != NULL)																	
	{																							
		vulkanExample->displayWindows.handleMessages(hWnd, uMsg, wParam, lParam);									
	}																							
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));											
}																							
#ifdef CONSOLE
int main(int argc, char** argv)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->displayWindows.setupWindow(hInstance, WndProc);
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#else
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int)
{
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->displayWindows.setupWindow(hInstance, WndProc);
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#endif // CONSOLE

