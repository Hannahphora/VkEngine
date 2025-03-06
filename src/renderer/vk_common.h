#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vma/vk_mem_alloc.h>

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

#ifdef DEBUG
const bool useValidationLayers = true;
const bool debug = true;
#else
const bool useValidationLayers = false;
const bool debug = false;
#endif

#define VK_CHECK(x)\
    do {\
        VkResult err = x;\
        if (err) {\
            fmt::print("Detected Vulkan error: {}\n", string_VkResult(err));\
            abort();\
        }\
    } while (0)

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct Queue {
    VkQueueFlagBits flag;
    std::optional<uint32_t> index;
    VkQueue queue;
    static bool allQueuesAvailable(const std::vector<Queue>& queues) {
        for (const auto& queue : queues) {
            if (!queue.index.has_value()) return false;
        }
        return true;
    }
};