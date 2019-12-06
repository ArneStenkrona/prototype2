#ifndef IMGUI_APPLICATION_H
#define IMGUI_APPLICATION_H

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

	ImGuiApplication();
	
	~ImGuiApplication();

	// Initialize styles, keys, etc.
	void init(float width, float height, vk::Device *device, VulkanApplication *vulkanApplication);

	// Initialize all Vulkan resources used by the ui
	void initResources(VkRenderPass renderPass, VkQueue copyQueue);

	// Starts a new imGui frame and sets up windows and ui elements
	void newFrame(/*VulkanExampleBase *example, */bool updateFrameGraph);

	// Update vertex and index buffer containing the imGui elements when required
	void updateBuffers();

	// Draw current imGui frame into a command buffer
	void drawFrame(VkCommandBuffer commandBuffer);
private:
	// Vulkan resources for rendering the UI
	VkSampler sampler;
	//vk::Buffer vertexBuffer;
	//vk::Buffer indexBuffer;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
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
	vk::Device *_device;
	//VulkanExampleBase *example;
    VulkanApplication *_vulkanApplication;
};

#endif