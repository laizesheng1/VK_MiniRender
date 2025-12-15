#include "VKM_Base/VK_Base.h"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

GLFWwindow* pWindow;
GLFWmonitor* pMonitor;
const char* windowTitle = "MiniRender";

bool InitializeWindow(VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true) {

	if (!glfwInit()) {
		std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n");
		return false;
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, isResizable);
	pMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
	pWindow = fullScreen ?
		glfwCreateWindow(pMode->width, pMode->height, windowTitle, pMonitor, nullptr) :
		glfwCreateWindow(size.width, size.height, windowTitle, nullptr, nullptr);
	if (!pWindow) {
		std::cout << std::format("[ InitializeWindow ]\nFailed to create a glfw window!\n");
		glfwTerminate();
		return false;
	}
	/*TODO*/
	
	VKM_Base::Get().SetCreateSurface([](vk::Instance instance) {
		VkSurfaceKHR surface;
		if (glfwCreateWindowSurface(instance, pWindow, nullptr, &surface) != VK_SUCCESS)
		{
			throw std::runtime_error("create surface is failed");
		}
		return surface;
		});
	

	return true;
}

void TerminateWindow() {
	glfwTerminate();
}
void MakeWindowFullScreen() {
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
	glfwSetWindowMonitor(pWindow, pMonitor, 0, 0, pMode->width, pMode->height, pMode->refreshRate);
}
void MakeWindowWindowed(VkOffset2D position, VkExtent2D size) {
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
	glfwSetWindowMonitor(pWindow, nullptr, position.x, position.y, size.width, size.height, pMode->refreshRate);
}
void TitleFps() {
	static double time0 = glfwGetTime();
	static double time1;
	static double dt;
	static int dframe = -1;
	static std::stringstream info;
	time1 = glfwGetTime();
	dframe++;
	if ((dt = time1 - time0) >= 1) {
		info.precision(1);
		info << windowTitle << "    " << std::fixed << dframe / dt << " FPS";
		glfwSetWindowTitle(pWindow, info.str().c_str());
		info.str("");
		time0 = time1;
		dframe = 0;
	}
}

int main() {
	auto& singleton = VKM_Base::Get();

	if (!InitializeWindow({ 1280, 720 }))
		return -1;
	
	singleton.initVulkan();
	singleton.prepare();

	while (!glfwWindowShouldClose(pWindow)) {

		/*äÖÈ¾¹ý³Ì£¬´ýÌî³ä*/

		glfwPollEvents();
		TitleFps();
	}
	TerminateWindow();
	delete(&singleton);
	return 0;
	
}