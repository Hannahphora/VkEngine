#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vma/vk_mem_alloc.h>

#include <vkb/VkBootstrap.h>

#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <set>
#include <limits>
#include <algorithm>

#ifdef _DEBUG
#define VULKAN_DEBUG_REPORT
#endif // _DEBUG

#define VK_CHECK(x)\
    do {\
        VkResult err = x;\
        if (err) {\
            fmt::print("Detected Vulkan error: {}\n", string_VkResult(err));\
            abort();\
        }\
    } while (0)
// VK_CHECK

struct AllocatedImg {
    VkImage img;
    VkImageView view;
    VmaAllocation allocation;
    VkExtent3D extent;
    VkFormat format;
};