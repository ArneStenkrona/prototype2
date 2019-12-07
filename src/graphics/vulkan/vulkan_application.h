#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include "src/graphics/geometry/model.h"

#include "imgui_application.h"

#include <vulkan/vulkan.h>

#include <vulkan/vulkan.hpp>

#include "src/container/vector.h"
#include "src/container/array.h"
#include "src/container/optional.h"
#include "src/config/prototype2Config.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>

#include <cstring>
#include <cstdlib>

extern const int WIDTH;
extern const int HEIGHT;

extern const unsigned int MAX_FRAMES_IN_FLIGHT;

extern const prt::vector<const char*> validationLayers;

extern const prt::vector<const char*> deviceExtensions;

extern const bool enableValidationLayers;

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

struct QueueFamilyIndices {
    prt::optional<uint32_t> graphicsFamily;
    prt::optional<uint32_t> presentFamily;
    
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    prt::vector<VkSurfaceFormatKHR> formats;
    prt::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model[MAXIMUM_MODEL_ENTITIES];
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 viewPosition;
};

class VulkanApplication {
public:
    VulkanApplication();

    void initWindow();
    void initVulkan();
    void update(const prt::vector<glm::mat4>& modelMatrices, glm::mat4& viewMatrix, glm::mat4& projectionMatrix, glm::vec3 viewPosition);
    void cleanup();

    void loadModels(prt::vector<Model>& models);
    void bindStaticEntities(const prt::vector<uint32_t>& modelIDs);

    GLFWwindow* getWindow() const { return window; }
    
    bool isWindowOpen() { return !glfwWindowShouldClose(window); }
private:
    GLFWwindow* window;

    ImGuiApplication _imGuiApplication;
    
    vk::Instance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    
    vk::PhysicalDevice physicalDevice;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    vk::Device device;
    
    //VkQueue copyQueue;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    
    VkSwapchainKHR swapChain;
    prt::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    prt::vector<VkImageView> swapChainImageViews;
    prt::vector<VkFramebuffer> swapChainFramebuffers;
    
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    
    VkCommandPool commandPool;
    
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;
    
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    
    // Textures
    uint32_t mipLevels;
    prt::array<VkImage, NUMBER_SUPPORTED_TEXTURES> textureImage;
    prt::array<VkDeviceMemory, NUMBER_SUPPORTED_TEXTURES> textureImageMemory;
    prt::array<VkImageView, NUMBER_SUPPORTED_TEXTURES> textureImageView;
    // Sampler
    VkSampler sampler;

    // Push constants
    prt::array<uint32_t, 2> pushConstants;
    // Entities
    prt::vector<uint32_t> _modelIDs;
    // Models data;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    // Uniform data
    prt::vector<VkBuffer> uniformBuffers;
    prt::vector<VkDeviceMemory> uniformBuffersMemory;
    // Descriptors
    VkDescriptorPool descriptorPool;
    prt::vector<VkDescriptorSet> descriptorSets;
    // Commands
    prt::vector<VkCommandBuffer> commandBuffers;
    // Indirect commands
    //prt::vector<VkDrawIndexedIndirectCommand> indirectCommands;
    // Contains the indirect drawing commands
	VkBuffer indirectCommandBuffer;
    VkDeviceMemory indirectCommandBufferMemory;
    // Concurrency
    prt::vector<VkSemaphore> imageAvailableSemaphores;
    prt::vector<VkSemaphore> renderFinishedSemaphores;
    prt::vector<VkFence> inFlightFences;
    size_t currentFrame = 0;
    
    bool framebufferResized = false;
    
    static void framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/);
    
    void cleanupSwapChain();
    
    void recreateSwapChain();
    
    void createInstance();
    
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    
    void setupDebugMessenger();
    
    void createSurface();
    
    void pickPhysicalDevice();
    
    void createLogicalDevice();
    
    void createSwapChain();
    
    void createImageViews();
    
    void createRenderPass();
    
    void createDescriptorSetLayout();
    
    void createGraphicsPipeline();
    
    void createFramebuffers();
    
    void createCommandPool(); 

    void createColorResources();
    
    void createDepthResources();
    
    VkFormat findSupportedFormat(const prt::vector<VkFormat>& candidates, 
                                 VkImageTiling tiling, VkFormatFeatureFlags features);
    
    VkFormat findDepthFormat();
    
    bool hasStencilComponent(VkFormat format);
    
    void createTextureImage(VkImage& texImage, VkDeviceMemory& texImageMemory, const Texture& texture);
    
    void generateMipmaps(VkImage image, VkFormat imageFormat, 
                         int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    
    VkSampleCountFlagBits getMaxUsableSampleCount();
    
    void createTextureImageView(VkImageView& texImageView, VkImage &texIm);
    
    void createSampler();
    
    VkImageView createImageView(VkImage image, VkFormat format, 
                                VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, 
                     VkSampleCountFlagBits numSamples, VkFormat format, 
                     VkImageTiling tiling, VkImageUsageFlags usage, 
                     VkMemoryPropertyFlags properties, VkImage& image, 
                     VkDeviceMemory& imageMemory);
    
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                               VkImageLayout newLayout, uint32_t mipLevels);
    
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    
    void createVertexBuffer(prt::vector<Model>& models);

    
    void createIndexBuffer(prt::vector<Model>& models);

    void createIndirectCommandBuffer(prt::vector<Model>& models);
    
    void createUniformBuffers();
    
    void createDescriptorPool();
    
    void createDescriptorSets();
    
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                      VkMemoryPropertyFlags properties, VkBuffer& buffer, 
                      VkDeviceMemory& bufferMemory);

    void createAndMapBuffer(void* bufferData, VkDeviceSize bufferSize, VkBufferUsageFlagBits bufferUsageFlagBits,
                            VkBuffer& destinationBuffer, VkDeviceMemory& destinationBufferMemory);
    
    VkCommandBuffer beginSingleTimeCommands();
    
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    void createCommandBuffers();
    
    void createSyncObjects();
    
    void updateUniformBuffer(uint32_t currentImage, const prt::vector<glm::mat4>& modelMatrices, 
                             glm::mat4& viewMatrix, glm::mat4& projectionMatrix, glm::vec3 viewPosition);
    
    void drawFrame(const prt::vector<glm::mat4>& modelMatrices, glm::mat4& viewMatrix, 
                   glm::mat4& projectionMatrix, glm::vec3 viewPosition);
    
    VkShaderModule createShaderModule(const prt::vector<char>& code);
    
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const prt::vector<VkSurfaceFormatKHR>& availableFormats);
    
    VkPresentModeKHR chooseSwapPresentMode(const prt::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    bool isDeviceSuitable(VkPhysicalDevice device);
    
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    
    prt::vector<const char*> getRequiredExtensions();
    
    bool checkValidationLayerSupport();
    
    static prt::vector<char> readFile(const std::string& filename);
    
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/, 
                                                        VkDebugUtilsMessageTypeFlagsEXT /*messageType*/, 
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
                                                        void* /*pUserData*/);
    
    friend class VulkanPRTGame;
    friend class ImGuiApplication;
};

#endif