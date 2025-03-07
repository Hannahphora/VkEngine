#include "vk_pipelines.h"
#include "vk_initialisers.h"
#include <fstream>

bool vkutil::loadShaderModule(const char *filePath, VkDevice device, VkShaderModule *outShaderModule) {
    
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) return false;

    size_t fSize = (size_t)file.tellg(); // get file size, by finding cursor pos (which is at end)
    std::vector<uint32_t> buffer(fSize / sizeof(uint32_t));

    file.seekg(0);
    file.read((char*)buffer.data(), fSize);
    file.close();

    VkShaderModuleCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.pNext = nullptr;
    info.codeSize = buffer.size() * sizeof(uint32_t); // size in bytes
    info.pCode = buffer.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &info, nullptr, &shaderModule) != VK_SUCCESS) return false;
    *outShaderModule = shaderModule;
    return true;
}
