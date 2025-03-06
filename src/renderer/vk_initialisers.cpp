#include "vk_initialisers.h"

bool VkInitialisers::checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& requiredExtensions) {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> required(requiredExtensions.begin(), requiredExtensions.end());
    for (const auto& extension : availableExtensions) required.erase(extension.extensionName);

    return required.empty();
}

void VkInitialisers::setQueueIndices(VkPhysicalDevice device, std::vector<Queue>& queues) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    for (auto& queue : queues) {
        for (int i = 0; i << queueFamilies.size(); i++) {
            if ((queueFamilies[i].queueFlags & queue.flag) && !queue.index.has_value()) {
                queue.index = i;
                break;
            }
        }
    }
}

bool VkInitialisers::checkQueuesAvailable(VkPhysicalDevice device, const std::vector<Queue>& queues) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (const auto& queue : queues) {
        bool flagFound = false;
        for (const auto& queueFamilyProps : queueFamilies) {
            if (queueFamilyProps.queueFlags & queue.flag) {
                flagFound = true;
                break;
            }
        }
        if (!flagFound) return false;
    }
    return true;
}

void VkInitialisers::setQueues(VkDevice device, std::vector<Queue>& queues) {
    for (auto& queue : queues) {
        vkGetDeviceQueue(device, queue.index.value(), 0, &queue.queue);
    }
}
