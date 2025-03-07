#pragma once
#include "vk_common.h"
#include "vk_descriptors.h"

struct DeletionQueue {
	std::deque<std::function<void()>> deletors;

	void push(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		for (auto itr = deletors.rbegin(); itr != deletors.rend(); itr++) (*itr)();
		deletors.clear();
	}
};

struct FrameData {
	VkSemaphore swapchainSemaphore, renderSemaphore;
	VkFence renderFence;

	VkCommandPool cmdPool;
	VkCommandBuffer mainCmdBuffer;

	DeletionQueue deletionQueue;
};

const uint32_t FRAME_OVERLAP = 2;

class Renderer {
public:

    bool isInitialised = false;
    uint64_t frameNumber = 0;
	VkExtent2D windowExtent = {};

    void init();
    void cleanup();

    void draw();
	void drawBg(VkCommandBuffer cmd);

	FrameData& getCurrentFrame() { return frames[frameNumber % FRAME_OVERLAP]; };
	FrameData frames[FRAME_OVERLAP];

    GLFWwindow* window;

    VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkSurfaceKHR surface;

	VmaAllocator allocator;
	DeletionQueue mainDeletionQueue;

	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;
	VkQueue computeQueue;
	uint32_t computeQueueFamily;

    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkExtent2D swapchainExtent;

	AllocatedImage drawImage;
	VkExtent2D drawExtent;

	DescriptorAllocator globalDescriptorAllocator;
	VkDescriptorSet drawImageDescriptors;
	VkDescriptorSetLayout drawImageDescriptorLayout;

	VkPipeline gradientPipeline;
	VkPipelineLayout gradientPipelineLayout;

	VkFence imdFence;
    VkCommandBuffer imdCmdBuffer;
    VkCommandPool imdCmdPool;

	void imdSubmit(std::function<void(VkCommandBuffer cmd)>&& fn);

private:
    void initVulkan();
	void initCommands();
	void initSyncStructures();

	void initDescriptors();
	void initPipelines();
	void initBgPipelines();

	void initImgui();

	void initSwapchain();
	void createSwapchain();
	void destroySwapchain();
};