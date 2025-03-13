#pragma once
#include "vk_common.h"

namespace vkinit {
    VkCommandPoolCreateInfo cmd_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    VkCommandBufferAllocateInfo cmd_buffer_alloc_info(VkCommandPool pool, uint32_t count = 1);

    VkCommandBufferBeginInfo cmd_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);
    VkCommandBufferSubmitInfo cmd_buffer_submit_info(VkCommandBuffer cmd);

    VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

    VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);
    VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

    VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);
    VkPresentInfoKHR present_info();

    VkRenderingAttachmentInfo color_attachment_info(VkImageView view, VkClearValue* clear ,VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo depth_attachment_info(VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingInfo rendering_info(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment);

    VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imgInfo, uint32_t binding);

    VkImageSubresourceRange img_subresource_range(VkImageAspectFlags aspectMask);

    VkImageCreateInfo img_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
    VkImageViewCreateInfo imgview_create_info(VkFormat format, VkImage img, VkImageAspectFlags aspectFlags);
} // namespace vkutil