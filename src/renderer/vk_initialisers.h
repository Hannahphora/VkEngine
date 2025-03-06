#pragma once
#include "vk_common.h"

namespace VkInitialisers {

    bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>* requiredExtensions);

    void setQueueIndices(VkPhysicalDevice device, std::vector<Queue>* queues);
    bool checkQueuesAvailable(VkPhysicalDevice device, std::vector<Queue>* queues);
    void setQueues(VkDevice device, std::vector<Queue>* queues);

}