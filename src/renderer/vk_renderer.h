#pragma once
#include "vk_common.h"
#include "vk_descriptors.h"

struct DeletionQueue {
	void push(std::function<void()>&& function) { m_deletors.push_back(function); }
	void flush() {
		for (auto itr = m_deletors.rbegin(); itr != m_deletors.rend(); itr++) (*itr)();
		m_deletors.clear();
	}
private:
	std::deque<std::function<void()>> m_deletors;
};

struct FrameData {
	VkSemaphore _swapchainSemaphore, _renderSemaphore;
	VkFence _renderFence;

	VkCommandPool _cmdPool;
	VkCommandBuffer _cmdBuf;

	DeletionQueue _deletionQueue;
};

const uint32_t FRAME_OVERLAP = 2;

class Renderer {
public:

    bool _isInitialised = false;
	bool _stopRendering = false;
	
    GLFWwindow* _wnd;
	VkExtent2D _wndExtent = {};

    VkInstance _instance;
	VkDebugUtilsMessengerEXT _dbgMsgr;
    VkPhysicalDevice _physDev;
	VkDevice _dev;
	
	uint64_t _frameNum = 0;
	FrameData _frames[FRAME_OVERLAP];
	FrameData& get_current_frame() { return _frames[_frameNum % FRAME_OVERLAP]; };

	VmaAllocator _allocator;
	DeletionQueue _primaryDeletionQueue;

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
	VkQueue _computeQueue;
	uint32_t _computeQueueFamily;

	VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    VkFormat _swapchainImgFormat;
	VkExtent2D _swapchainExtent;
	VkExtent2D _drawExtent;

	std::vector<VkImage> _swapchainImgs;
	std::vector<VkImageView> _swapchainImgViews;

	DescriptorAllocator _descriptorAllocator;

	VkPipeline _gradientPipeline;
	VkPipelineLayout _gradientPipelineLayout;

	VkDescriptorSet _drawImgDescriptors;
	VkDescriptorSetLayout _drawImgDescriptorLayout;

	VkFence _imdFence;
    VkCommandBuffer _imdCmdBuf;
    VkCommandPool _imdCmdPool;

	AllocatedImg _drawImg;

	void init();
    void cleanup();
	
	void render();

	void imd_submit(std::function<void(VkCommandBuffer cmd)>&& fn);

private:
	// init funcs
    void init_vk();
	void init_cmds();
	void init_sync();
	void init_descriptors();
	void init_pipelines();
	void init_swapchain();
	void init_imgui();

	// other funcs
	void create_swapchain();
	void rebuild_swapchain();
	void destroy_swapchain();

	// draw funcs
	void draw();
	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);
	void draw_background(VkCommandBuffer cmd);

};