#ifndef VULKAN_COMMON_H
#define VULKAN_COMMON_H

#include <vulkan/vulkan.h>

#include "src/container/vector.h"

namespace vulkan_common {
    void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, 
                      VkDeviceSize size,
                      VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    VkShaderModule createShaderModule(VkDevice device, const prt::vector<char>& code);

    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, 
                            VkMemoryPropertyFlags properties);

};
#endif