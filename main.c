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

typedef struct SwapChainSupportDetails 
{
	VkSurfaceCapabilitiesKHR capabilities;
	VkSurfaceFormatKHR *formats;
	VkPresentModeKHR *presentModes;
} SwapChainSupportDetails;

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

	VkSwapchainKHR swapChain;
	VkImage *swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkImageView *swapChainImageViews;
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
	for (uint32_t i = 0; i < extensionCount; i++) 
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

	for(uint32_t i = 0; i < queueFamilyCount; i++) 
	{
		VkQueueFamilyProperties queueFamily = queueFamilies[i];

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
		{
			indices.graphicsFamily.value = i;
			indices.graphicsFamily.hasValue = true;
		}

		if (presentSupport) 
		{
			indices.presentFamily.value = i;
			indices.presentFamily.hasValue = true;
		}

		if(indicesIsComplete(indices)) break;
	}

	return indices;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) 
{
	unsigned int deviceExtensionCount = 0;
	VkResult res = vkEnumerateDeviceExtensionProperties(device, NULL, &deviceExtensionCount, NULL);
	if (deviceExtensionCount <= 0 || res != VK_SUCCESS) 
	{
		printf("Could not find any Vulkan device extensions (found %d, error %d)\n", deviceExtensionCount, res);
		return false;
	}

	VkExtensionProperties *deviceExtensionProperties = calloc(deviceExtensionCount, sizeof(VkExtensionProperties));
	res = vkEnumerateDeviceExtensionProperties(device, NULL, &deviceExtensionCount, deviceExtensionProperties);
	if (res != VK_SUCCESS) 
	{
		printf("vkEnumerateDeviceExtensionProperties() failed (%d)\n", res);
		return false;
	}

	const char **devExtensions = calloc(deviceExtensionCount, sizeof(void*));
	for (uint32_t i = 0; i < deviceExtensionCount; i++)
	{
		devExtensions[i] = &deviceExtensionProperties[i].extensionName[0];
	}

	return true;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) 
{
	SwapChainSupportDetails details = 
	{
		.presentModes = NULL,
		.formats = NULL
	};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);

	if (formatCount != 0) 
	{
		details.formats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats);
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);

	if (presentModeCount != 0) 
	{
		details.presentModes = malloc(presentModeCount * sizeof(VkPresentModeKHR));
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes);
	}

	return details;
}

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) 
{
	QueueFamilyIndices indices = findQueueFamilies(device, surface);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) 
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
		swapChainAdequate = !(swapChainSupport.formats == NULL) && !(swapChainSupport.presentModes == NULL);

		free(swapChainSupport.presentModes);
		free(swapChainSupport.formats);
	}

	return indicesIsComplete(indices) && extensionsSupported && swapChainAdequate;
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

	for(uint32_t i = 0; i < deviceCount; i++) 
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
	
	unsigned int deviceExtensionCount = 0;
	VkResult res = vkEnumerateDeviceExtensionProperties((*app).physicalDevice, NULL, &deviceExtensionCount, NULL);
	if (deviceExtensionCount <= 0 || res != VK_SUCCESS) 
	{
		printf("Could not find any Vulkan device extensions (found %d, error %d)\n", deviceExtensionCount, res);
		return;
	}

	VkExtensionProperties *deviceExtensionProperties = calloc(deviceExtensionCount, sizeof(VkExtensionProperties));
	res = vkEnumerateDeviceExtensionProperties((*app).physicalDevice, NULL, &deviceExtensionCount, deviceExtensionProperties);
	if (res != VK_SUCCESS) 
	{
		printf("vkEnumerateDeviceExtensionProperties() failed (%d)\n", res);
		return;
	}

	const char **deviceExtensions = calloc(deviceExtensionCount, sizeof(void*));
	for (uint32_t i = 0; i < deviceExtensionCount; i++)
	{
		deviceExtensions[i] = &deviceExtensionProperties[i].extensionName[0];
	}

	createInfo.enabledExtensionCount = deviceExtensionCount;
	createInfo.ppEnabledExtensionNames = deviceExtensions;

	res = vkCreateDevice((*app).physicalDevice, &createInfo, NULL, &(*app).device);  
	if (res != VK_SUCCESS) 
	{
		printf("failed to create logical device!\n");
		glfwDestroyWindow((*app).windowStruct.pWindow);
		glfwTerminate();
		exit(res);
	}
	
	vkGetDeviceQueue((*app).device, presentIndices.presentFamily.value, 0, &(*app).presentQueue);

	free(queueCreateInfos);
	free(deviceExtensions);
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR *availableFormats, VkPhysicalDevice device, VkSurfaceKHR surface) 
{
	uint32_t surfaceFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, NULL);

	for(uint32_t i = 0; i < surfaceFormatCount; i++)
	{
		VkSurfaceFormatKHR availableFormat = availableFormats[i];

		if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR *availablePresentModes, VkPhysicalDevice device, VkSurfaceKHR surface) 
{
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);

	for(uint32_t i = 0; i < presentModeCount; i++) 
	{
		VkPresentModeKHR availablePresentMode = availablePresentModes[i];

		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR *capabilities, GLFWwindow *window) 
{
	uint32_t max = -1;

	if ((*capabilities).currentExtent.width != max) 
	{
		return (*capabilities).currentExtent;
	} else 
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		
		VkExtent2D actualExtent = 
		{
			(uint32_t)(width),
			(uint32_t)(height)
		};
	
		actualExtent.width = clamp(actualExtent.width, (*capabilities).minImageExtent.width, (*capabilities).maxImageExtent.width);
		actualExtent.height = clamp(actualExtent.height, (*capabilities).minImageExtent.height, (*capabilities).maxImageExtent.height);
	
		return actualExtent;
	}
}

void createSwapchain(vulkanApp *app)
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport((*app).physicalDevice, (*app).surface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats, (*app).physicalDevice, (*app).surface);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, (*app).physicalDevice, (*app).surface);
	VkExtent2D extent = chooseSwapExtent(&swapChainSupport.capabilities, (*app).windowStruct.pWindow);

	(*app).swapChainImageFormat = surfaceFormat.format;
	(*app).swapChainExtent = extent;

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) 
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = (*app).surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies((*app).physicalDevice, (*app).surface);
	uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value, indices.presentFamily.value};

	if (indices.graphicsFamily.value != indices.presentFamily.value) 
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else 
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = NULL; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR((*app).device, &createInfo, NULL, &(*app).swapChain) != VK_SUCCESS) 
	{
		printf("failed to create swap chain!\n");
		glfwDestroyWindow((*app).windowStruct.pWindow);
		glfwTerminate();
		exit(-1);
	}
}

void createSwapchainImages(vulkanApp *app)
{
	uint32_t imageCount;
	vkGetSwapchainImagesKHR((*app).device, (*app).swapChain, &imageCount, NULL);
	
	(*app).swapChainImages = malloc(imageCount * sizeof(VkImage));
	vkGetSwapchainImagesKHR((*app).device, (*app).swapChain, &imageCount, (*app).swapChainImages);
}

void createImageViews(vulkanApp *app) 
{
	uint32_t imageCount;
	vkGetSwapchainImagesKHR((*app).device, (*app).swapChain, &imageCount, NULL);

	uint32_t formatCount;
	VkSurfaceFormatKHR* colorFormat = malloc(sizeof(VkSurfaceFormatKHR));
	vkGetPhysicalDeviceSurfaceFormatsKHR((*app).physicalDevice, (*app).surface, &formatCount, colorFormat);

	VkImageViewCreateInfo iv_info = {0};
	iv_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	iv_info.pNext = NULL;
	iv_info.format = (*colorFormat).format;
	iv_info.components = (VkComponentMapping){
		.r = VK_COMPONENT_SWIZZLE_R,
		.g = VK_COMPONENT_SWIZZLE_G,
		.b = VK_COMPONENT_SWIZZLE_B,
		.a = VK_COMPONENT_SWIZZLE_A
	};
	iv_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	iv_info.subresourceRange.baseMipLevel = 0;
	iv_info.subresourceRange.levelCount = 1;
	iv_info.subresourceRange.baseArrayLayer = 0;
	iv_info.subresourceRange.layerCount = 1;
	iv_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	iv_info.flags = 0;

	(*app).swapChainImageViews = malloc(imageCount * sizeof(VkImageView));
	for (uint32_t i = 0; i < imageCount; i++) 
	{
		iv_info.image = (*app).swapChainImages[i];
		VkResult res = vkCreateImageView((*app).device, &iv_info, NULL, &(*app).swapChainImageViews[i]);
		if (res != VK_SUCCESS) 
		{
			printf("vkCreateImageView() %d failed (%d)\n", i, res);
			glfwDestroyWindow((*app).windowStruct.pWindow);
			glfwTerminate();
			exit(res);
		}
	}
}

void initVulkanApp(vulkanApp *app)
{
	createInstance((*app).windowStruct.pWindow, &(*app).instance);
	
	VkResult err = glfwCreateWindowSurface((*app).instance, (*app).windowStruct.pWindow, NULL, &(*app).surface);
	if(err)
	{
		printf("Failed to create Window Surface\n");
		glfwDestroyWindow((*app).windowStruct.pWindow);
		glfwTerminate();
		exit(-1);
	}

	selectGPU(app);
	createLogicalDevice(app);

	vkGetDeviceQueue((*app).device, (*app).graphicsQueueFamily.graphicsFamily.value, 0, &(*app).graphicsQueue);
	createSwapchain(app);
}

void renderLoop(vulkanApp app)
{
	glfwPollEvents();
}

void freeVulkanApp(vulkanApp *app)
{
	uint32_t imageCount;
	vkGetSwapchainImagesKHR((*app).device, (*app).swapChain, &imageCount, NULL);

	for (uint32_t i = 0; i < imageCount; i++)
		vkDestroyImageView((*app).device, (*app).swapChainImageViews[i], NULL);
	free((*app).swapChainImageViews);

	free((*app).swapChainImages);
	vkDestroySwapchainKHR((*app).device, (*app).swapChain, NULL);

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
