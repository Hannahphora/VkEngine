#pragma once
#include "vk_common.h"

namespace vkutil {
    bool load_shader_module(const char* filePath, VkDevice device, VkShaderModule* out);
} // namespace vkutil