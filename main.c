#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define max(a,b) (a>b ? a : b)
#define min(a,b) (a<b ? a : b)
#define clamp(x,lo,hi) (min( hi, max(lo,x) ))

typedef struct window
{
	GLFWwindow *pWindow;
	char *windowName;
	uint32_t width, height;
} window;

typedef struct vulkanApp
{
	window windowStruct;

	VkInstance instance;
	VkPhysicalDevice physicalDevice;
} vulkanApp;

void initWindow(window *pWindowStruct, uint32_t width, uint32_t height, char *windowName)
{
	(*pWindowStruct).width  = width;
	(*pWindowStruct).height = height;
	(*pWindowStruct).windowName = windowName;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	(*pWindowStruct).pWindow = glfwCreateWindow(width, height, windowName, NULL, NULL);
}

void createInstance(GLFWwindow *window, VkInstance *instance)
{
	VkApplicationInfo appInfo = {0};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Mouse Run Game";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "MouseRun Vulkan Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;
	createInfo.enabledLayerCount = 0;

	VkResult result = vkCreateInstance(&createInfo, NULL, instance);

	if (result != VK_SUCCESS) 
	{
		printf("failed to create instance!");
		glfwDestroyWindow(window);
		glfwTerminate();
		exit(-1);
	}

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

	VkExtensionProperties *extensions = malloc(extensionCount * sizeof(VkExtensionProperties));

	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);

	printf("available extensions:\n");
	for (int i = 0; i < extensionCount; i++) 
	{
		VkExtensionProperties extension = extensions[i];
		printf("\t%s\n", extension.extensionName);
	}

	free(extensions);
}

bool isDeviceSuitable(VkPhysicalDevice device) 
{
	return true;
}

void selectGPU(vulkanApp *app)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices((*app).instance, &deviceCount, NULL);

	if (deviceCount == 0) 
	{
		printf("failed to find GPUs with Vulkan support!");
		glfwDestroyWindow((*app).windowStruct.pWindow);
		glfwTerminate();
		exit(-1);
	}

	VkPhysicalDevice *devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices((*app).instance, &deviceCount, devices);

	for(int i = 0; i < deviceCount; i++) 
	{
		VkPhysicalDevice device = devices[i];

		if (isDeviceSuitable(device)) 
		{
			(*app).physicalDevice = device;
			printf("Found a suitable GPU\n");
			break;
		}
	}

	if((*app).physicalDevice == VK_NULL_HANDLE) 
	{
		printf("failed to find a suitable GPU!\n");
		glfwDestroyWindow((*app).windowStruct.pWindow);
		glfwTerminate();
		exit(-1);
	}

	free(devices);
}

void initVulkanApp(vulkanApp *app)
{
	createInstance((*app).windowStruct.pWindow, &(*app).instance);
	selectGPU(app);
}

void renderLoop(vulkanApp app)
{
	glfwPollEvents();
}

void freeVulkanApp(vulkanApp *app)
{
	vkDestroyInstance((*app).instance, NULL);
	glfwDestroyWindow((*app).windowStruct.pWindow);

	glfwTerminate();
}

int main()
{
	glfwInit();
	vulkanApp app = {0};

	initWindow(&app.windowStruct, 800, 600, "Mouse-Run");
	initVulkanApp(&app);

	while(!glfwWindowShouldClose(app.windowStruct.pWindow))
	{
		renderLoop(app);
	}

	freeVulkanApp(&app);
}
