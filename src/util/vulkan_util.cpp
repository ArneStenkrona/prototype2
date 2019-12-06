#include "vulkan_util.h"

#include <cstring>
#include <cassert>

VkCommandBuffer vulkan_util::beginSingleTimeCommands(const VkDevice& device, VkCommandPool& commandPool) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    return commandBuffer;
}

void vulkan_util::endSingleTimeCommands(const VkDevice& device, 
                                        VkCommandPool& commandPool, VkCommandBuffer commandBuffer,
                                        VkQueue& commandQueue) {
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(commandQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(commandQueue);
    
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void vulkan_util::createBuffer(const VkPhysicalDevice& physicalDevice, const VkDevice& device, VkDeviceSize size, 
                               VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                               VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        assert(false && "failed to create buffer!");
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        assert(false && "failed to allocate buffer memory!");
    }
    
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void vulkan_util::copyBuffer(const VkDevice& device, VkCommandPool& commandPool, VkQueue& commandQueue,
                             VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);
    
    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    
    endSingleTimeCommands(device, commandPool, commandBuffer, commandQueue);
}

void vulkan_util::createAndMapBuffer(const VkPhysicalDevice& physicalDevice, const VkDevice& device, void* bufferData, VkDeviceSize bufferSize,
                                     VkBufferUsageFlagBits BufferUsageFlagBits,
                                     VkBuffer& destinationBuffer,
                                     VkDeviceMemory& destinationBufferMemory,
                                     VkCommandPool& commandPool, VkQueue& commandQueue) {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, bufferData, (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
    
    createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | BufferUsageFlagBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, destinationBuffer, destinationBufferMemory);
    
    copyBuffer(device, commandPool, commandQueue, stagingBuffer, destinationBuffer, bufferSize);
    
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

uint32_t vulkan_util::findMemoryType(const VkPhysicalDevice& physicalDevice, 
                                     uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    assert(false && "failed to find suitable memory type!");
}