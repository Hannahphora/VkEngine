#pragma once
#include "vk_common.h"
#include "vk_initialisers.h"

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> requiredDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class Renderer {
public:

    bool isInitialised = false;
    uint64_t frameNumber = 0;

    void init();
    void cleanup();
    void draw();

    // handles
    GLFWwindow* window;

    VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkSurfaceKHR surface;

    std::vector<Queue> queues = {
		{ VK_QUEUE_GRAPHICS_BIT },
		{ VK_QUEUE_COMPUTE_BIT }
	};

    VkSwapchainKHR swapChain;
    VkFormat swapChainImageFormat;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkExtent2D swapChainExtent;

private:

    void createInstance();
	void createSurface();

    bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();

    void selectPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	void createLogicalDevice();

	void createSwapchain();
	void destroySwapchain();
	SwapChainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void createImageViews();

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void setupDebugMessenger();
    
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
    
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator);

};