#ifndef IMGUI_APPLICATION_H
#define IMGUI_APPLICATION_H

#include "src/system/input/input.h"

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

class ImGuiApplication {
public:
	// UI params are set via push constants
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;

	ImGuiApplication(VkPhysicalDevice& physicalDevice, VkDevice& device, 
					 Input& input);
	
	~ImGuiApplication();
	
	void cleanup();
	void cleanupSwapchain();

	// Initialize styles, keys, etc.
	void init(float width, float height);

	// Initialize all Vulkan resources used by the ui
	void initResources(VkRenderPass renderPass, VkCommandPool& commandPool, 
					   VkQueue& copyQueue,
					   VkSampleCountFlagBits msaaSamples);


	// Starts a new imGui frame and sets up windows and ui elements
	void newFrame(bool updateFrameGraph);

	// Update vertex and index buffer containing the imGui elements when required
	void updateBuffers();

	void updateInput(float width, float height, float deltaTime);

	// Draw current imGui frame into a command buffer
	void drawFrame(VkCommandBuffer commandBuffer);
private:
	// Vulkan resources for rendering the UI
	VkSampler sampler;
	//vk::Buffer vertexBuffer;
	//vk::Buffer indexBuffer;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
	void* vertexBufferMapped = nullptr;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
	void* indexBufferMapped = nullptr;
	int32_t vertexCount = 0;
	int32_t indexCount = 0;

	VkDeviceMemory fontMemory = VK_NULL_HANDLE;
	VkImage fontImage = VK_NULL_HANDLE;
	VkImageView fontView = VK_NULL_HANDLE;
	VkPipelineCache pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkPhysicalDevice& _physicalDevice;
	VkDevice& _device;

	Input& _input;
};

#endif