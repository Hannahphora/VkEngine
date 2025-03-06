#pragma once
#include "vk_common.h"

namespace vkutil {
    void transitionImage(VkCommandBuffer cmd, VkImage img, VkImageLayout currentLayout, VkImageLayout newLayout);
}