#include "vk_renderer.h"
#include "vk_initialisers.h"
#include "vk_images.h"
#include "vk_descriptors.h"
#include "vk_pipelines.h"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

void Renderer::init() {

    int w{}, h{};
    glfwGetWindowSize(window, &w, &h);
    windowExtent = { (uint32_t)w, (uint32_t)h };
    
    initVulkan();
    initSwapchain();
    initCommands();
    initSyncStructures();
    initDescriptors();
    initPipelines();
    initImgui();

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

            frames[i].deletionQueue.flush();
		}

        mainDeletionQueue.flush();

        destroySwapchain();
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
        vkDestroyInstance(instance, nullptr);
    }
}

void Renderer::draw() {
    // wait for gpu to finish rendering last frame
    VK_CHECK(vkWaitForFences(device, 1, &getCurrentFrame().renderFence, true, 1000000000));
    getCurrentFrame().deletionQueue.flush();

    // request img from swapchain
    uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, getCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex));

    // reset fence and cmd buffer
    VK_CHECK(vkResetFences(device, 1, &getCurrentFrame().renderFence));
    VK_CHECK(vkResetCommandBuffer(getCurrentFrame().mainCmdBuffer, 0));
    auto cmd = getCurrentFrame().mainCmdBuffer; // alias command buffer to cmd

    // begin cmd buffer recording
	auto cmdBeginInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // setup draw img
    drawExtent.width = drawImage.imageExtent.width;
	drawExtent.height = drawImage.imageExtent.height;
    vkutil::transitionImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    drawBg(cmd); // records drawing cmds

    // transition draw and swapchain imgs to transfer layouts
	vkutil::transitionImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transitionImage(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// copy from draw img to swapchain
	vkutil::copyImageToImage(cmd, drawImage.image, swapchainImages[swapchainImageIndex], drawExtent, swapchainExtent);

	// set swapchain image layout to present
	vkutil::transitionImage(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// finish recordings commands
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

void Renderer::drawBg(VkCommandBuffer cmd) {

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipelineLayout, 0, 1, &drawImageDescriptors, 0, nullptr);

    vkCmdDispatch(cmd, std::ceil(drawExtent.width / 16.0), std::ceil(drawExtent.height / 16.0), 1);
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

    VkPhysicalDeviceVulkan13Features features = {};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features.dynamicRendering = true;
	features.synchronization2 = true;

	VkPhysicalDeviceVulkan12Features features12 = {};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	vkb::PhysicalDeviceSelector selector(vkbInstance.value());
	auto vkbPhysicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)
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

    VmaAllocatorCreateInfo allocInfo = {};
    allocInfo.physicalDevice = physicalDevice;
    allocInfo.device = device;
    allocInfo.instance = instance;
    allocInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocInfo, &allocator);

    mainDeletionQueue.push([&]() {
        vmaDestroyAllocator(allocator);
    });
}

void Renderer::initSwapchain() {
    createSwapchain();

    // setup draw img
    VkExtent3D drawImageExtent = { windowExtent.width, windowExtent.height, 1 };

    drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // alloc and create img
    auto rImgInfo = vkinit::imageCreateInfo(drawImage.imageFormat, drawImageUsages, drawImageExtent);
    VmaAllocationCreateInfo rImgAllocInfo = {};
	rImgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rImgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vmaCreateImage(allocator, &rImgInfo, &rImgAllocInfo, &drawImage.image, &drawImage.allocation, nullptr);

    // create img view for draw img
    auto rViewInfo = vkinit::imageViewCreateInfo(drawImage.imageFormat, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(device, &rViewInfo, nullptr, &drawImage.imageView));

    mainDeletionQueue.push([=]() {
		vkDestroyImageView(device, drawImage.imageView, nullptr);
		vmaDestroyImage(allocator, drawImage.image, drawImage.allocation);
	});
}

void Renderer::createSwapchain() {
    vkb::SwapchainBuilder swapchainBuilder(physicalDevice, device, surface);
	swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	auto vkbSwapchain = swapchainBuilder
		.set_desired_format(VkSurfaceFormatKHR{ .format = swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
        .set_desired_extent(windowExtent.width, windowExtent.height)
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

    VK_CHECK(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &imdCmdPool));
	auto cmdAllocInfo = vkinit::commandBufferAllocateInfo(imdCmdPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &imdCmdBuffer));

	mainDeletionQueue.push([=]() { 
	    vkDestroyCommandPool(device, imdCmdPool, nullptr);
	});
}

void Renderer::initSyncStructures() {

    auto fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    auto semaphoreCreateInfo = vkinit::semaphoreCreateInfo();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));
	}

    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &imdFence));
	mainDeletionQueue.push([=]() {
        vkDestroyFence(device, imdFence, nullptr);
    });
}

void Renderer::initDescriptors() {

    // init descriptor allocator with 10 sets
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
    };
    globalDescriptorAllocator.initPool(device, 10, sizes);
    
    { // create descriptor set layout for compute draw
        DescriptorLayoutBuilder builder = {};
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        drawImageDescriptorLayout = builder.build(device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    // allocate descriptor set for draw img
    drawImageDescriptors = globalDescriptorAllocator.allocate(device, drawImageDescriptorLayout);

    VkDescriptorImageInfo imgInfo = {};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imgInfo.imageView = drawImage.imageView;

    VkWriteDescriptorSet drawImgWrite = {};
    drawImgWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	drawImgWrite.pNext = nullptr;
    drawImgWrite.dstBinding = 0;
	drawImgWrite.dstSet = drawImageDescriptors;
	drawImgWrite.descriptorCount = 1;
	drawImgWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	drawImgWrite.pImageInfo = &imgInfo;

    vkUpdateDescriptorSets(device, 1, &drawImgWrite, 0, nullptr); // updates desc set with draw img

    mainDeletionQueue.push([&]() {
        globalDescriptorAllocator.destroyPool(device);
        vkDestroyDescriptorSetLayout(device, drawImageDescriptorLayout, nullptr);
    });
}

void Renderer::initPipelines() {
    initBgPipelines();
}

void Renderer::initBgPipelines() {
    VkPipelineLayoutCreateInfo computeLayout = {};
    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext = nullptr;
	computeLayout.pSetLayouts = &drawImageDescriptorLayout;
	computeLayout.setLayoutCount = 1;

    VK_CHECK(vkCreatePipelineLayout(device, &computeLayout, nullptr, &gradientPipelineLayout));

    // pipeline layout
    VkShaderModule computeDrawShader = {};
	if (!vkutil::loadShaderModule("gradient.spv", device, &computeDrawShader)) {
		fmt::print("error building shader \n");
	}

    VkPipelineShaderStageCreateInfo stageinfo = {};
	stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.pNext = nullptr;
	stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.module = computeDrawShader;
	stageinfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo = {};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = gradientPipelineLayout;
	computePipelineCreateInfo.stage = stageinfo;
	
	VK_CHECK(vkCreateComputePipelines(device, nullptr, 1, &computePipelineCreateInfo, nullptr, &gradientPipeline));

    vkDestroyShaderModule(device, computeDrawShader, nullptr);
	mainDeletionQueue.push([&]() {
		vkDestroyPipelineLayout(device, gradientPipelineLayout, nullptr);
		vkDestroyPipeline(device, gradientPipeline, nullptr);
	});
}

void Renderer::initImgui() {

}

void Renderer::imdSubmit(std::function<void(VkCommandBuffer cmd)>&& fn) {
    
}