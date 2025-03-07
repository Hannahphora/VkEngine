#pragma once
#include "vk_common.h"

namespace vkutil {
    bool loadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
}