#include "vk_renderer.h"
#include "vk_initialisers.h"
#include "vk_images.h"

void Renderer::init() {
    
    initVulkan();
    createSwapchain();
    initCommands();
    initSyncStructures();

    isInitialised = true;
}

void Renderer::cleanup() {
    if (isInitialised) {
        vkDeviceWaitIdle(device);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
			vkDestroyCommandPool(device, frames[i].cmdPool, nullptr);
            vkDestroyFence(device, frames[i].renderFence, nullptr);
            vkDestroySemaphore(device, frames[i].renderSemaphore, nullptr);
            vkDestroySemaphore(device ,frames[i].swapchainSemaphore, nullptr);
		}

        destroySwapchain();
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
        vkDestroyInstance(instance, nullptr);
    }
}

void Renderer::draw() {

    VK_CHECK(vkWaitForFences(device, 1, &getCurrentFrame().renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(device, 1, &getCurrentFrame().renderFence));

    uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, getCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex));

    // reset command buffer and begin recording
    auto cmd = getCurrentFrame().mainCmdBuffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
	auto cmdBeginInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    vkutil::transitionImage(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    VkClearColorValue clearValue = {};
	float flash = std::abs(std::sin(frameNumber / 120.0f));
	clearValue = { { 0.0f, 0.0f, flash, 1.0f } };
	auto clearRange = vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdClearColorImage(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

	vkutil::transitionImage(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	VK_CHECK(vkEndCommandBuffer(cmd));

    auto cmdinfo = vkinit::commandBufferSubmitInfo(cmd);	
	auto waitInfo = vkinit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame().swapchainSemaphore);
	auto signalInfo = vkinit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);	
	auto submit = vkinit::submitInfo(&cmdinfo, &signalInfo, &waitInfo);	

	VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, getCurrentFrame().renderFence));

    auto presentInfo = vkinit::presentInfo();
    presentInfo.pSwapchains = &swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

	frameNumber++;
}

void Renderer::initVulkan() {

    vkb::InstanceBuilder instanceBuilder;
	auto vkbInstance = instanceBuilder
        .set_app_name("VkEngine")
		.request_validation_layers(useValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();
    if (!vkbInstance) {
        fmt::print("error creating instance: {}\n", vkbInstance.error().message());
        abort();
    }

	instance = vkbInstance.value().instance;
	debugMessenger = vkbInstance.value().debug_messenger;

    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));

	vkb::PhysicalDeviceSelector selector(vkbInstance.value());
	auto vkbPhysicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13({
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .synchronization2 = true,
            .dynamicRendering = true,
        })
		.set_required_features_12({
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .descriptorIndexing = true,
            .bufferDeviceAddress = true,
        })
		.set_surface(surface)
		.select();
    if (!vkbPhysicalDevice) {
        fmt::print("error selecting physical device: {}\n", vkbPhysicalDevice.error().message());
        abort();
    }

	vkb::DeviceBuilder deviceBuilder(vkbPhysicalDevice.value());
	auto vkbDevice = deviceBuilder.build();
    if (!vkbDevice) {
        fmt::print("error creating device: {}\n", vkbDevice.error().message());
        abort();
    }

    physicalDevice = vkbPhysicalDevice.value().physical_device;
	device = vkbDevice.value().device;
	
    graphicsQueue = vkbDevice.value().get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily = vkbDevice.value().get_queue_index(vkb::QueueType::graphics).value();

    computeQueue = vkbDevice.value().get_queue(vkb::QueueType::compute).value();
	computeQueueFamily = vkbDevice.value().get_queue_index(vkb::QueueType::compute).value();
}

void Renderer::createSwapchain() {

    vkb::SwapchainBuilder swapchainBuilder(physicalDevice, device, surface);
	swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	auto vkbSwapchain = swapchainBuilder
		.set_desired_format(VkSurfaceFormatKHR{ .format = swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();
    if (!vkbSwapchain) {
        fmt::print("error creating swapchain: {}\n", vkbSwapchain.error().message());
        abort();
    }

	swapchainExtent = vkbSwapchain.value().extent;
	swapchain = vkbSwapchain.value().swapchain;
	swapchainImages = vkbSwapchain.value().get_images().value();
	swapchainImageViews = vkbSwapchain.value().get_image_views().value();
}

void Renderer::destroySwapchain() {
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    for (int i = 0; i < swapchainImageViews.size(); i++) {
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    }
}

void Renderer::initCommands() {
    auto cmdPoolInfo = vkinit::commandPoolInfoCreate(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &frames[i].cmdPool));
        auto cmdAllocInfo = vkinit::commandBufferAllocateInfo(frames[i].cmdPool, 1);
        VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].mainCmdBuffer));
	}

}

void Renderer::initSyncStructures() {
    auto fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    auto semaphoreCreateInfo = vkinit::semaphoreCreateInfo();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));
	}
}
