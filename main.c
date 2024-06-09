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

typedef struct QueueFamilyIndices 
{
	struct graphicsFamily
	{
		uint32_t value;
		bool hasValue;
	} graphicsFamily;
	
	struct presentFamily
	{
		uint32_t value;
		bool hasValue;
	} presentFamily;
} QueueFamilyIndices;

typedef struct vulkanApp
{
	window windowStruct;

	VkInstance instance;
	VkPhysicalDevice physicalDevice;

	QueueFamilyIndices graphicsQueueFamily;
	VkDevice device;

	VkQueue graphicsQueue;
	VkSurfaceKHR surface;
	VkQueue presentQueue;
} vulkanApp;

bool indicesIsComplete(QueueFamilyIndices q)
{
	return q.graphicsFamily.hasValue && q.presentFamily.hasValue;
}

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

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) 
{
	QueueFamilyIndices indices;
	
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

	VkQueueFamilyProperties *queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

	for(int i = 0; i < queueFamilyCount; i++) 
	{
		VkQueueFamilyProperties queueFamily = queueFamilies[i];

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
		{
			indices.graphicsFamily.value = i;
			indices.graphicsFamily.hasValue = true;
		}

		if (presentSupport) {
			indices.presentFamily.value = i;
			indices.presentFamily.hasValue = true;
		}

		if(indicesIsComplete(indices)) break;
	}

	return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) 
{
	QueueFamilyIndices indices = findQueueFamilies(device, surface);

	return indices.graphicsFamily.hasValue;
}

void selectGPU(vulkanApp *app)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices((*app).instance, &deviceCount, NULL);

	if (deviceCount == 0) 
	{
		printf("failed to find GPUs with Vulkan support!\n");
		glfwDestroyWindow((*app).windowStruct.pWindow);
		glfwTerminate();
		exit(-1);
	}

	VkPhysicalDevice *devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices((*app).instance, &deviceCount, devices);

	for(int i = 0; i < deviceCount; i++) 
	{
		VkPhysicalDevice device = devices[i];

		if (isDeviceSuitable(device, (*app).surface)) 
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

void createLogicalDevice(vulkanApp *app)
{
	QueueFamilyIndices indices = findQueueFamilies((*app).physicalDevice, (*app).surface);

	VkDeviceQueueCreateInfo queueCreateInfo = {0};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value;
	queueCreateInfo.queueCount = 1;

	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures = {0};

	VkDeviceCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = 0;
	createInfo.enabledLayerCount = 0;
	
	QueueFamilyIndices presentIndices = findQueueFamilies((*app).physicalDevice, (*app).surface);

	VkDeviceQueueCreateInfo *queueCreateInfos = malloc(2 * sizeof(VkDeviceQueueCreateInfo));
	uint32_t uniqueQueueFamilies[2] = {presentIndices.graphicsFamily.value, presentIndices.presentFamily.value};

	float queueInfoPriority = 1.0f;
	for (int i = 0; i < 2; i++)
	{
		uint32_t queueFamily = uniqueQueueFamilies[i];

		VkDeviceQueueCreateInfo queueCreateInfo = {0};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queueInfoPriority;

		queueCreateInfos[i] = queueCreateInfo;
	}

	createInfo.queueCreateInfoCount = 2;
	createInfo.pQueueCreateInfos = queueCreateInfos;

	VkResult res = vkCreateDevice((*app).physicalDevice, &createInfo, NULL, &(*app).device);  
	if (res != VK_SUCCESS) 
	{
		printf("failed to create logical device!\n");
		glfwDestroyWindow((*app).windowStruct.pWindow);
		glfwTerminate();
		exit(res);
	}
	
	vkGetDeviceQueue((*app).device, presentIndices.presentFamily.value, 0, &(*app).presentQueue);

	free(queueCreateInfos);
}

void initVulkanApp(vulkanApp *app)
{
	createInstance((*app).windowStruct.pWindow, &(*app).instance);
	
	VkResult err = glfwCreateWindowSurface((*app).instance, (*app).windowStruct.pWindow, NULL, &(*app).surface);
	if(err)
	{
		printf("Failed to create Window Surface");
		glfwDestroyWindow((*app).windowStruct.pWindow);
		glfwTerminate();
		exit(-1);
	}

	selectGPU(app);
	createLogicalDevice(app);

	vkGetDeviceQueue((*app).device, (*app).graphicsQueueFamily.graphicsFamily.value, 0, &(*app).graphicsQueue);
}

void renderLoop(vulkanApp app)
{
	glfwPollEvents();
}

void freeVulkanApp(vulkanApp *app)
{
	vkDestroyDevice((*app).device, NULL);
	vkDestroySurfaceKHR((*app).instance, (*app).surface, NULL);
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
