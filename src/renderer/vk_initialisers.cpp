#include "vk_initialisers.h"

VkCommandPoolCreateInfo vkinit::cmd_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.queueFamilyIndex = queueFamilyIndex;
    info.flags = flags;
    return info;
}

VkCommandBufferAllocateInfo vkinit::cmd_buffer_alloc_info(VkCommandPool pool, uint32_t count) {
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.commandPool = pool;
    info.commandBufferCount = count;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    return info;
}

VkCommandBufferBeginInfo vkinit::cmd_buffer_begin_info(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    return info;
}

VkCommandBufferSubmitInfo vkinit::cmd_buffer_submit_info(VkCommandBuffer cmd) {
    VkCommandBufferSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.pNext = nullptr;
	info.commandBuffer = cmd;
	info.deviceMask = 0;
	return info;
}

VkFenceCreateInfo vkinit::fence_create_info(VkFenceCreateFlags flags) {
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    return info;
}

VkSemaphoreCreateInfo vkinit::semaphore_create_info(VkSemaphoreCreateFlags flags) {
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    return info;
}

VkSemaphoreSubmitInfo vkinit::semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {
    VkSemaphoreSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    info.pNext = nullptr;
	info.semaphore = semaphore;
	info.stageMask = stageMask;
	info.deviceIndex = 0;
	info.value = 1;
	return info;
}

VkSubmitInfo2 vkinit::submit_info(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signalSemaphoreInfo, VkSemaphoreSubmitInfo *waitSemaphoreInfo) {
    VkSubmitInfo2 info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.pNext = nullptr;
    info.waitSemaphoreInfoCount = (waitSemaphoreInfo == nullptr) ? 0 : 1;
    info.pWaitSemaphoreInfos = waitSemaphoreInfo;
    info.signalSemaphoreInfoCount = (signalSemaphoreInfo == nullptr) ? 0 : 1;
    info.pSignalSemaphoreInfos = signalSemaphoreInfo;
    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos = cmd;
    return info;
}

VkPresentInfoKHR vkinit::present_info() {
    VkPresentInfoKHR info = {};
    info.sType =  VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.pNext = nullptr;
    return info;
}

VkImageSubresourceRange vkinit::img_subresource_range(VkImageAspectFlags aspectMask) {
    VkImageSubresourceRange subImage = {};
    subImage.aspectMask = aspectMask;
    subImage.baseMipLevel = 0;
    subImage.levelCount = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;
    return subImage;
}

VkImageCreateInfo vkinit::img_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent) {
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;

    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent = extent;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT; // for MSAA, defaults to 1 sample/px as not in use
    info.tiling = VK_IMAGE_TILING_OPTIMAL; // means the image is stored with the best gpu format
    info.usage = usageFlags;

    return info;
}

VkImageViewCreateInfo vkinit::imgview_create_info(VkFormat format, VkImage img, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;

    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.image = img;
    info.format = format;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = aspectFlags;

    return info;
}

VkRenderingAttachmentInfo vkinit::color_attachment_info(VkImageView view, VkClearValue *clear, VkImageLayout layout) {
    VkRenderingAttachmentInfo attachment = {};
    attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachment.pNext = nullptr;
    
    attachment.imageView = view;
    attachment.imageLayout = layout;
    attachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    if (clear) attachment.clearValue = *clear;

    return attachment;
}

VkRenderingInfo vkinit::rendering_info(VkExtent2D renderExtent, VkRenderingAttachmentInfo *colorAttachment, VkRenderingAttachmentInfo *depthAttachment) {
    VkRenderingInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    info.pNext = nullptr;

    info.renderArea = VkRect2D { VkOffset2D { 0, 0 }, renderExtent };
    info.layerCount = 1;
    info.colorAttachmentCount = 1;
    info.pColorAttachments = colorAttachment;
    info.pDepthAttachment = depthAttachment;
    info.pStencilAttachment = nullptr;
    
    return info;
}

VkWriteDescriptorSet vkinit::write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo *imgInfo, uint32_t binding) {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;

    write.dstBinding = binding;
    write.dstSet = dstSet;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = imgInfo;
    
    return write;
}
