#pragma once
#include "vk_common.h"

struct DescriptorLayoutBuilder {
    void addBinding(uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
    private:
        std::vector<VkDescriptorSetLayoutBinding> m_bindings;
};

struct DescriptorAllocator {
    struct PoolSizeRatio{
		    VkDescriptorType type;
		    float ratio;
    };

    VkDescriptorPool pool;

    void initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
    void clearDescriptors(VkDevice device);
    void destroyPool(VkDevice device);
    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};