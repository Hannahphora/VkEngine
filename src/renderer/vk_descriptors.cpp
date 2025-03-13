#include "vk_descriptors.h"

void DescriptorLayoutBuilder::addBinding(uint32_t binding, VkDescriptorType type) {
    VkDescriptorSetLayoutBinding newbind = {};
    newbind.binding = binding;
    newbind.descriptorCount = 1;
    newbind.descriptorType = type;
    m_bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::clear() {
    m_bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext,
    VkDescriptorSetLayoutCreateFlags flags) {
    
    for (auto& b : m_bindings) b.stageFlags |= shaderStages;

    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext = pNext;
    info.pBindings = m_bindings.data();
    info.bindingCount = (uint32_t)m_bindings.size();
    info.flags = flags;

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));
    return set;
}

void DescriptorAllocator::initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (auto ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = uint32_t(ratio.ratio * maxSets)
        });
    }

	VkDescriptorPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.flags = 0;
	info.maxSets = maxSets;
	info.poolSizeCount = (uint32_t)poolSizes.size();
	info.pPoolSizes = poolSizes.data();

	vkCreateDescriptorPool(device, &info, nullptr, &pool);
}

void DescriptorAllocator::clearDescriptors(VkDevice device) {
    vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::destroyPool(VkDevice device) {
    vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout) {
    VkDescriptorSetAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.descriptorPool = pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    VkDescriptorSet dSet;
    VK_CHECK(vkAllocateDescriptorSets(device, &info, &dSet));
    return dSet;
}