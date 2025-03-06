#pragma once
#include "vk_common.h"

struct FrameData {
	VkCommandPool cmdPool;
	VkCommandBuffer mainCmdBuffer;
	VkSemaphore swapchainSemaphore, renderSemaphore;
	VkFence renderFence;
};

const uint32_t FRAME_OVERLAP = 2;

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

	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;
	VkQueue computeQueue;
	uint32_t computeQueueFamily;

    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkExtent2D swapchainExtent;

	FrameData& getCurrentFrame() { return frames[frameNumber % FRAME_OVERLAP]; };
	FrameData frames[FRAME_OVERLAP];

private:

    void initVulkan();
	void initCommands();
	void initSyncStructures();

	void createSwapchain();
	void destroySwapchain();

};