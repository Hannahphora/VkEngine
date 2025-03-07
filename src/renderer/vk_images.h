#pragma once
#include "vk_common.h"

namespace vkutil {
    void transitionImage(VkCommandBuffer cmd, VkImage img, VkImageLayout currentLayout, VkImageLayout newLayout);
    void copyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
}