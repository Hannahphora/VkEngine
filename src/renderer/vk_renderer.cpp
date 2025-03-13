#include "vk_renderer.h"
#include "vk_initialisers.h"
#include "vk_images.h"
#include "vk_descriptors.h"
#include "vk_pipelines.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

void Renderer::init() {

    glfwGetWindowSize(_wnd, (int*)&_wndExtent.width, (int*)&_wndExtent.height);
    
    init_vk();
    init_swapchain();
    init_cmds();
    init_sync();
    init_descriptors();
    init_pipelines();
    init_imgui();

    _isInitialised = true;
}

void Renderer::cleanup() {
    if (_isInitialised) {
        vkDeviceWaitIdle(_dev);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
			vkDestroyCommandPool(_dev, _frames[i]._cmdPool, nullptr);
            vkDestroyFence(_dev, _frames[i]._renderFence, nullptr);
            vkDestroySemaphore(_dev, _frames[i]._renderSemaphore, nullptr);
            vkDestroySemaphore(_dev ,_frames[i]._swapchainSemaphore, nullptr);

            _frames[i]._deletionQueue.flush();
		}

        _primaryDeletionQueue.flush();

        destroy_swapchain();
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkDestroyDevice(_dev, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _dbgMsgr);
        vkDestroyInstance(_instance, nullptr);
    }
}

void Renderer::render() {
    
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();
    ImGui::Render();

    draw();
}

void Renderer::draw() {
    
    // wait for gpu to finish rendering last frame, 1sec timeout
    VK_CHECK(vkWaitForFences(_dev, 1, &get_current_frame()._renderFence, true, 1000000000));
    get_current_frame()._deletionQueue.flush();

    // request img from swapchain
    uint32_t swapchainImageIndex;
    auto e = vkAcquireNextImageKHR(_dev, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex);
	if (e == VK_ERROR_OUT_OF_DATE_KHR) {
		rebuild_swapchain();
		return;
	}

    // reset fence and cmd buffer
    VK_CHECK(vkResetFences(_dev, 1, &get_current_frame()._renderFence));
    VK_CHECK(vkResetCommandBuffer(get_current_frame()._cmdBuf, 0));
    auto cmd = get_current_frame()._cmdBuf; // alias command buffer to cmd

    // begin cmd buffer recording
	auto cmdBeginInfo = vkinit::cmd_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // setup draw img
    _drawExtent.width = _drawImg.extent.width;
	_drawExtent.height = _drawImg.extent.height;
    vkutil::transition_img(cmd, _drawImg.img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    draw_background(cmd);

    // transition draw and swapchain imgs to transfer layouts
	vkutil::transition_img(cmd, _drawImg.img, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transition_img(cmd, _swapchainImgs[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // execute copy from draw img into swapchain
	vkutil::copy_img_to_img(cmd, _drawImg.img, _swapchainImgs[swapchainImageIndex], _drawExtent, _swapchainExtent);

    // set swapchain img layout to attachment optimal
	vkutil::transition_img(cmd, _swapchainImgs[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // draw imgui to swapchain img
	draw_imgui(cmd, _swapchainImgViews[swapchainImageIndex]);

	// set swapchain image layout to present
	vkutil::transition_img(cmd, _swapchainImgs[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// finish recordings commands
	VK_CHECK(vkEndCommandBuffer(cmd));

    auto cmdinfo = vkinit::cmd_buffer_submit_info(cmd);
	auto waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame()._swapchainSemaphore);
	auto signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);
	auto submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);	

	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

    auto presentInfo = vkinit::present_info();
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

	_frameNum++;
}

void Renderer::draw_background(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImgDescriptors, 0, nullptr);

    vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
}

void Renderer::init_vk() {

    vkb::InstanceBuilder instanceBuilder;
	auto vkbInstance = instanceBuilder
        .set_app_name("VkEngine")
		.request_validation_layers(VULKAN_DEBUG_REPORT)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();
    if (!vkbInstance) {
        fmt::print("error creating instance: {}\n", vkbInstance.error().message());
        abort();
    }

	_instance = vkbInstance.value().instance;
	_dbgMsgr = vkbInstance.value().debug_messenger;

    VK_CHECK(glfwCreateWindowSurface(_instance, _wnd, nullptr, &_surface));

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
		.set_surface(_surface)
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

    _physDev = vkbPhysicalDevice.value().physical_device;
	_dev = vkbDevice.value().device;
	
    _graphicsQueue = vkbDevice.value().get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily = vkbDevice.value().get_queue_index(vkb::QueueType::graphics).value();

    _computeQueue = vkbDevice.value().get_queue(vkb::QueueType::compute).value();
	_computeQueueFamily = vkbDevice.value().get_queue_index(vkb::QueueType::compute).value();

    VmaAllocatorCreateInfo allocInfo = {};
    allocInfo.physicalDevice = _physDev;
    allocInfo.device = _dev;
    allocInfo.instance = _instance;
    allocInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocInfo, &_allocator);

    _primaryDeletionQueue.push([&]() {
        vmaDestroyAllocator(_allocator);
    });
}

void Renderer::init_swapchain() {
    create_swapchain();

    VkExtent3D drawImageExtent = { _wndExtent.width, _wndExtent.height, 1 };

    _drawImg.format = VK_FORMAT_R16G16B16A16_SFLOAT; // hardcode 32 bit float format
    _drawImg.extent = drawImageExtent;

    VkImageUsageFlags drawImageUsages = {};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // alloc and create img
    auto rImgInfo = vkinit::img_create_info(_drawImg.format, drawImageUsages, drawImageExtent);
    VmaAllocationCreateInfo rImgAllocInfo = {};
	rImgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rImgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vmaCreateImage(_allocator, &rImgInfo, &rImgAllocInfo, &_drawImg.img, &_drawImg.allocation, nullptr);

    // create img view for draw img
    auto rViewInfo = vkinit::imgview_create_info(_drawImg.format, _drawImg.img, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(_dev, &rViewInfo, nullptr, &_drawImg.view));

    _primaryDeletionQueue.push([=]() {
		vkDestroyImageView(_dev, _drawImg.view, nullptr);
		vmaDestroyImage(_allocator, _drawImg.img, _drawImg.allocation);
	});
}

void Renderer::create_swapchain() {
    vkb::SwapchainBuilder swapchainBuilder(_physDev, _dev, _surface);
	_swapchainImgFormat = VK_FORMAT_B8G8R8A8_UNORM;

	auto vkbSwapchain = swapchainBuilder
		.set_desired_format(VkSurfaceFormatKHR{ .format = _swapchainImgFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
        .set_desired_extent(_wndExtent.width, _wndExtent.height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();
    if (!vkbSwapchain) {
        fmt::print("error creating swapchain: {}\n", vkbSwapchain.error().message());
        abort();
    }

	_swapchainExtent = vkbSwapchain.value().extent;
	_swapchain = vkbSwapchain.value().swapchain;
	_swapchainImgs = vkbSwapchain.value().get_images().value();
	_swapchainImgViews = vkbSwapchain.value().get_image_views().value();
}

void Renderer::destroy_swapchain() {
    vkDestroySwapchainKHR(_dev, _swapchain, nullptr);
    for (int i = 0; i < _swapchainImgViews.size(); i++) {
        vkDestroyImageView(_dev, _swapchainImgViews[i], nullptr);
    }
}

void Renderer::rebuild_swapchain() {
    vkQueueWaitIdle(_graphicsQueue);

    vkb::SwapchainBuilder swapchainBuilder(_physDev, _dev, _surface);

    glfwGetWindowSize(_wnd, (int*)&_wndExtent.width, (int*)&_wndExtent.height);

	vkDestroySwapchainKHR(_dev, _swapchain, nullptr);
	vkDestroyImageView(_dev, _drawImg.view, nullptr);
	vmaDestroyImage(_allocator, _drawImg.img, _drawImg.allocation);
	
	auto vkbSwapchain = swapchainBuilder
		.set_desired_format(VkSurfaceFormatKHR{ .format = _swapchainImgFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
        .set_desired_extent(_wndExtent.width, _wndExtent.height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();
    if (!vkbSwapchain) {
        fmt::print("error creating swapchain: {}\n", vkbSwapchain.error().message());
        abort();
    }

	_swapchain = vkbSwapchain.value().swapchain;
	_swapchainImgs = vkbSwapchain.value().get_images().value();
	_swapchainImgViews = vkbSwapchain.value().get_image_views().value();
	_swapchainImgFormat = vkbSwapchain.value().image_format;

	VkExtent3D drawImageExtent = { _wndExtent.width, _wndExtent.height, 1 };

	_drawImg.format = VK_FORMAT_R16G16B16A16_SFLOAT; // hardcode 32 bit float format

	VkImageUsageFlags drawImageUsages = {};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;

	auto imgCreateInfo = vkinit::img_create_info(_drawImg.format, drawImageUsages, drawImageExtent);

	// alloc draw img from gpu local heap
	VmaAllocationCreateInfo imgAllocInfo = {};
	imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	imgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// create img
	vmaCreateImage(_allocator, &imgCreateInfo, &imgAllocInfo, &_drawImg.img, &_drawImg.allocation, nullptr);

    // create draw img view
	auto imgviewInfo = vkinit::imgview_create_info(_drawImg.format, _drawImg.img, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(_dev, &imgviewInfo, nullptr, &_drawImg.view));

	VkDescriptorImageInfo imgInfo = {};
	imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imgInfo.imageView = _drawImg.view;

	VkWriteDescriptorSet cameraWrite = vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, _drawImgDescriptors, &imgInfo, 0);
	vkUpdateDescriptorSets(_dev, 1, &cameraWrite, 0, nullptr);

	_primaryDeletionQueue.push([&]() {
		vkDestroyImageView(_dev, _drawImg.view, nullptr);
		vmaDestroyImage(_allocator, _drawImg.img, _drawImg.allocation);
	});
}

void Renderer::init_cmds() {

    auto cmdPoolInfo = vkinit::cmd_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateCommandPool(_dev, &cmdPoolInfo, nullptr, &_frames[i]._cmdPool));
        auto cmdAllocInfo = vkinit::cmd_buffer_alloc_info(_frames[i]._cmdPool, 1);
        VK_CHECK(vkAllocateCommandBuffers(_dev, &cmdAllocInfo, &_frames[i]._cmdBuf));
	}

    VK_CHECK(vkCreateCommandPool(_dev, &cmdPoolInfo, nullptr, &_imdCmdPool));
	auto cmdAllocInfo = vkinit::cmd_buffer_alloc_info(_imdCmdPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(_dev, &cmdAllocInfo, &_imdCmdBuf));

	_primaryDeletionQueue.push([=]() { 
	    vkDestroyCommandPool(_dev, _imdCmdPool, nullptr);
	});
}

void Renderer::init_sync() {

    auto fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    auto semaphoreCreateInfo = vkinit::semaphore_create_info();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(_dev, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
		VK_CHECK(vkCreateSemaphore(_dev, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(_dev, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
	}

    VK_CHECK(vkCreateFence(_dev, &fenceCreateInfo, nullptr, &_imdFence));
	_primaryDeletionQueue.push([=]() {
        vkDestroyFence(_dev, _imdFence, nullptr);
    });
}

void Renderer::init_descriptors() {

    // init descriptor allocator with 10 sets
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
    };
    _descriptorAllocator.initPool(_dev, 10, sizes);
    
    { // create descriptor set layout for compute draw
        DescriptorLayoutBuilder builder = {};
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _drawImgDescriptorLayout = builder.build(_dev, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    // allocate descriptor set for draw img
    _drawImgDescriptors = _descriptorAllocator.allocate(_dev, _drawImgDescriptorLayout);

    VkDescriptorImageInfo imgInfo = {};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imgInfo.imageView = _drawImg.view;

    VkWriteDescriptorSet drawImgWrite = {};
    drawImgWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	drawImgWrite.pNext = nullptr;
    drawImgWrite.dstBinding = 0;
	drawImgWrite.dstSet = _drawImgDescriptors;
	drawImgWrite.descriptorCount = 1;
	drawImgWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	drawImgWrite.pImageInfo = &imgInfo;

    vkUpdateDescriptorSets(_dev, 1, &drawImgWrite, 0, nullptr); // updates desc set with draw img

    _primaryDeletionQueue.push([&]() {
        _descriptorAllocator.destroyPool(_dev);
        vkDestroyDescriptorSetLayout(_dev, _drawImgDescriptorLayout, nullptr);
    });
}

void Renderer::init_pipelines() {
    VkPipelineLayoutCreateInfo computeLayout = {};
    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext = nullptr;
	computeLayout.pSetLayouts = &_drawImgDescriptorLayout;
	computeLayout.setLayoutCount = 1;

    VK_CHECK(vkCreatePipelineLayout(_dev, &computeLayout, nullptr, &_gradientPipelineLayout));

    // pipeline layout
    VkShaderModule computeDrawShader = {};
	if (!vkutil::load_shader_module("gradient.spv", _dev, &computeDrawShader)) {
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
	computePipelineCreateInfo.layout = _gradientPipelineLayout;
	computePipelineCreateInfo.stage = stageinfo;
	
	VK_CHECK(vkCreateComputePipelines(_dev, nullptr, 1, &computePipelineCreateInfo, nullptr, &_gradientPipeline));

    vkDestroyShaderModule(_dev, computeDrawShader, nullptr);
	_primaryDeletionQueue.push([&]() {
		vkDestroyPipelineLayout(_dev, _gradientPipelineLayout, nullptr);
		vkDestroyPipeline(_dev, _gradientPipeline, nullptr);
	});
}

void Renderer::init_imgui() {

    // create descriptor pools for imgui
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000;
	poolInfo.poolSizeCount = (uint32_t)std::size(pool_sizes);
	poolInfo.pPoolSizes = pool_sizes;

    VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(_dev, &poolInfo, nullptr, &imguiPool));

    // init imgui for glfw/vulkan
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(_wnd, true);

    ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = _instance;
	initInfo.PhysicalDevice = _physDev;
	initInfo.Device = _dev;
    initInfo.QueueFamily = _graphicsQueueFamily;
	initInfo.Queue = _graphicsQueue;
	initInfo.DescriptorPool = imguiPool;
	initInfo.MinImageCount = 3;
	initInfo.ImageCount = 3;
	initInfo.UseDynamicRendering = true;

    VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo = {};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.pNext = nullptr;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &_swapchainImgFormat;

    initInfo.PipelineRenderingCreateInfo = pipelineRenderingInfo;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo);
    ImGui_ImplVulkan_CreateFontsTexture();

    _primaryDeletionQueue.push([=]() {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(_dev, imguiPool, nullptr);
	});
}

void Renderer::imd_submit(std::function<void(VkCommandBuffer cmd)>&& fn) {
    VK_CHECK(vkResetFences(_dev, 1, &_imdFence));
	VK_CHECK(vkResetCommandBuffer(_imdCmdBuf, 0));

	VkCommandBuffer cmd = _imdCmdBuf;

	auto cmdBeginInfo = vkinit::cmd_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	fn(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	auto cmdinfo = vkinit::cmd_buffer_submit_info(cmd);
	auto submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _imdFence));
	VK_CHECK(vkWaitForFences(_dev, 1, &_imdFence, true, 9999999999));
}

void Renderer::draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView) {
    auto colorAttachment = vkinit::color_attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	auto renderInfo = vkinit::rendering_info(_swapchainExtent, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
	vkCmdEndRendering(cmd);
}
