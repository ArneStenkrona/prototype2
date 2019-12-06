#ifndef VULKAN_UTIL_H
#define VULKAN_UTIL_H

#include <vulkan/vulkan.h>

namespace vulkan_util {

    VkCommandBuffer beginSingleTimeCommands(const VkDevice& device, VkCommandPool& commandPool);
    
    void endSingleTimeCommands(const VkDevice& device, 
                               VkCommandPool& commandPool, VkCommandBuffer commandBuffer,
                               VkQueue& queue);

    void createBuffer(const VkPhysicalDevice& physicalDevice, const VkDevice& device, VkDeviceSize size, 
                      VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                      VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    
    void copyBuffer(const VkDevice& device, VkCommandPool& commandPool, VkQueue& commandQueue,
                    VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void createAndMapBuffer(const VkPhysicalDevice& physicalDevice, const VkDevice& device, void* bufferData, VkDeviceSize bufferSize, 
                            VkBufferUsageFlagBits BufferUsageFlagBits,
                            VkBuffer& destinationBuffer, VkDeviceMemory& destinationBufferMemory,
                            VkCommandPool& commandPool, VkQueue& commandQueue);

    uint32_t findMemoryType(const VkPhysicalDevice& physicalDevice, 
                            uint32_t typeFilter, VkMemoryPropertyFlags properties);

};

#endif