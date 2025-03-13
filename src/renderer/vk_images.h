#pragma once
#include "vk_common.h"

namespace vkutil {
    void transition_img(VkCommandBuffer cmd, VkImage img, VkImageLayout currentLayout, VkImageLayout newLayout);
    void copy_img_to_img(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);
    void gen_mipmaps(VkCommandBuffer cmd, VkImage img, VkExtent2D imgSize);
} // namespace vkutil