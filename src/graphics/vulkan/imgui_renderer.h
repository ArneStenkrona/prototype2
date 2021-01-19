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

class ImGuiRenderer
{
public:
    // UI params are set via push constants
    struct PushConstBlock
    {
        glm::vec2 scale;
        glm::vec2 translate;
    } pushConstBlock;

    ImGuiRenderer(VkPhysicalDevice physicalDevice, VkDevice device);

    ~ImGuiRenderer();

    // Initialize all Vulkan resources used by the ui
    void initResources(VkCommandPool &commandPool,
                       VkQueue &copyQueue, size_t swapchainCount,
                       size_t renderPass,
                       size_t subpass,
                       unsigned int renderGroup,
                       size_t dynamicAssetIndex,
                       GraphicsPipeline &pipeline);

    void update(float width, float height,
                float deltaTime,
                size_t imageIndex,
                DynamicAssets & asset,
                GraphicsPipeline & pipeline);

private:
    // Vulkan resources for rendering the UI
    VkSampler sampler;
    VkDescriptorImageInfo fontDescriptor;
    // TODO: set this in a dynamic, cross-platform manner 
    float dpiScaleFactor = 2.0f;

    size_t swapchainImageCount;

    VkDeviceMemory fontMemory = VK_NULL_HANDLE;
    VkImage fontImage = VK_NULL_HANDLE;
    VkImageView fontView = VK_NULL_HANDLE;

    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;

    void createPipeline(unsigned int renderGroup,
                        size_t renderPass,
                        size_t subpass,
                        size_t dynamicAssetIndex,
                        GraphicsPipeline & pipeline);

    void updateBuffers(size_t imageIndex,
                       DynamicAssets & asset);

    void updateDrawCommands(prt::vector<GUIDrawCall> & drawCalls);

};

#endif