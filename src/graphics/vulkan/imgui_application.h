#ifndef IMGUI_APPLICATION_H
#define IMGUI_APPLICATION_H

#include "src/system/input/input.h"

#include "graphics_pipeline.h"

#include <imgui/imgui.h>

#include <vulkan/vulkan.h>

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

// ----------------------------------------------------------------------------
// ImGuiApplication class, https://github.com/SaschaWillems/Vulkan/blob/master/examples/imgui/main.cpp
// ----------------------------------------------------------------------------
class VulkanApplication;

struct DynamicAssets;

class ImGuiApplication
{
public:
    // UI params are set via push constants
    struct PushConstBlock
    {
        glm::vec2 scale;
        glm::vec2 translate;
    } pushConstBlock;

    ImGuiApplication(VkPhysicalDevice physicalDevice, VkDevice device,
                     Input &input, float width, float height);

    ~ImGuiApplication();

    // Initialize all Vulkan resources used by the ui
    void initResources(VkCommandPool &commandPool,
                       VkQueue &copyQueue, size_t swapchainCount,
                       VkSampleCountFlagBits msaaSamples,
                       size_t renderPass,
                       size_t subpass,
                       unsigned int renderGroup,
                       size_t dynamicAssetIndex,
                       GraphicsPipeline &pipeline);

    void update(float width, float height,
                float deltaTime,
                size_t imageIndex, VkCommandPool commandPool, VkQueue queue,
                // VkFence *pFence, uint32_t nFence, 
                DynamicAssets & asset,
                GraphicsPipeline & pipeline);

private:
    // Vulkan resources for rendering the UI
    VkSampler sampler;
    VkDescriptorImageInfo fontDescriptor;
    //vk::Buffer vertexBuffer;
    //vk::Buffer indexBuffer;
    // VkBuffer vertexBuffer;
    // VkDeviceMemory vertexBufferMemory;
    // void *vertexBufferMapped = nullptr;
    // VkBuffer indexBuffer;
    // VkDeviceMemory indexBufferMemory;
    // void *indexBufferMapped = nullptr;
    // int32_t vertexCount = 0;
    // int32_t indexCount = 0;
    float dpiScaleFactor = 1.0f;

    bool initFrameGraph = false;

    size_t swapchainImageCount;
    // VkExtent2D swapchainExtent;

    VkDeviceMemory fontMemory = VK_NULL_HANDLE;
    VkImage fontImage = VK_NULL_HANDLE;
    VkImageView fontView = VK_NULL_HANDLE;
    // VkPipelineCache pipelineCache;
    // VkPipelineLayout pipelineLayout;
    // VkPipeline pipeline;
    // VkDescriptorPool descriptorPool;
    // VkDescriptorSetLayout descriptorSetLayout;
    // VkDescriptorSet descriptorSet;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;

    Input &m_input;

    // Initialize styles, keys, etc.
    void init(float width, float height);

    void createPipeline(unsigned int renderGroup,
                        size_t renderPass,
                        size_t subpass,
                        size_t dynamicAssetIndex,
                        GraphicsPipeline & pipeline);

        // Starts a new imGui frame and sets up windows and ui elements
    void newFrame(bool updateFrameGraph);

    // Update vertex and index buffer containing the imGui elements when required
    void updateBuffers(size_t imageIndex, VkCommandPool commandPool, VkQueue queue,
                    //    VkFence *pFence, uint32_t nFence, 
                       DynamicAssets & asset);

    void updateInput(float width, float height, float deltaTime);

    // Draw current imGui frame into a command buffer
    // void drawFrame(VkCommandBuffer commandBuffer);

    void updateDrawCommands(prt::vector<GUIDrawCall> & drawCalls);

};

#endif