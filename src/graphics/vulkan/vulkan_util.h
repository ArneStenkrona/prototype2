#ifndef VULKAN_COMMON_H
#define VULKAN_COMMON_H

#include <vulkan/vulkan.h>

#include "src/container/vector.h"

namespace vkutil {
    void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, 
                      VkDeviceSize size,
                      VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    void createAndMapBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
                            VkCommandPool commandPool, VkQueue queue,
                            void* bufferData, VkDeviceSize bufferSize,
                            VkBufferUsageFlagBits bufferUsageFlagBits,
                            VkBuffer& destinationBuffer,
                            VkDeviceMemory& destinationBufferMemory);

    void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue,
                    VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);

    void endSingleTimeCommands(VkDevice device, VkQueue queue, 
                               VkCommandPool commandPool, VkCommandBuffer commandBuffer);

    VkShaderModule createShaderModule(VkDevice device, const prt::vector<char>& code);

    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, 
                            VkMemoryPropertyFlags properties);

};
#endif