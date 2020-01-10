#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include "src/graphics/geometry/model.h"

#include "imgui_application.h"

#include "src/system/input/input.h"

#include <vulkan/vulkan.h>

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

struct ModelUBO {
    alignas(16) glm::mat4 model[NUMBER_SUPPORTED_MODEL_MATRICES];
    alignas(16) glm::mat4 invTransposeModel[NUMBER_SUPPORTED_MODEL_MATRICES];
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 viewPosition;
};

struct SkyboxUBO {
		alignas(16) glm::mat4 projection;
		alignas(16) glm::mat4 model;
		//alignas(4) float lodBias = 0.0f;
};

class VulkanApplication {
public:
    VulkanApplication(Input& input);

    void initWindow();
    void initVulkan();
    void update(const prt::vector<glm::mat4>& modelMatrices, 
                const glm::mat4& viewMatrix, 
                const glm::mat4& projectionMatrix, 
                const glm::vec3 viewPosition,
                const glm::mat4& skyProjectionMatrix,
                float deltaTime);
    void cleanup();


    void bindScene(const prt::vector<Model>& models, const prt::vector<uint32_t>& modelIndices,
                   const prt::array<Texture, 6>& skybox);

    GLFWwindow* getWindow() const { return _window; }
    void getWindowSize(int& w, int& h) { w = width; h = height; };
    
    bool isWindowOpen() { return !glfwWindowShouldClose(_window); }
private:
    GLFWwindow* _window;
    int width, height;

    //ImGuiApplication _imGuiApplication;
    
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    
    VkPhysicalDevice physicalDevice;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkDevice device;
    
    //VkQueue copyQueue;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    
    VkSwapchainKHR swapChain;
    prt::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    prt::vector<VkImageView> swapChainImageViews;
    prt::vector<VkFramebuffer> swapChainFramebuffers;
    
    struct {
        VkDescriptorSetLayout skybox;
        VkDescriptorSetLayout model;
    } descriptorSetLayouts;

    VkRenderPass renderPass;
    struct {
        VkPipelineLayout skybox;
        VkPipelineLayout model;
    } pipelineLayouts;

    struct {
        VkPipelineCache skybox;
        VkPipelineCache model;
    } pipelineCaches;

    struct {
        VkPipeline skybox;
        VkPipeline model;
    } pipelines;
    
    VkCommandPool commandPool;
    
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;
    
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    
    // Model textures
    uint32_t mipLevels;
    prt::array<VkImage, NUMBER_SUPPORTED_TEXTURES> textureImage;
    prt::array<VkDeviceMemory, NUMBER_SUPPORTED_TEXTURES> textureImageMemory;
    prt::array<VkImageView, NUMBER_SUPPORTED_TEXTURES> textureImageView;

    // Cubemap texture
    VkImage cubeMapImage;
    VkDeviceMemory cubeMapImageMemory;
    VkImageView cubeMapImageView;

    // Sampler
    struct {
        VkSampler skybox;
        VkSampler model;
    } samplers;

    // Push constants
    prt::array<uint32_t, 2> pushConstants;
    // Entities
    //prt::vector<uint32_t> _modelIDs;

    struct RenderJob {
        uint32_t _modelMatrixIdx;
        uint32_t _imgIdx;
        uint32_t _firstIndex;
        uint32_t _indexCount;
    };
    prt::vector<RenderJob> _renderJobs;

    // Models data;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    // Skybox data
    VkBuffer skyboxVertexBuffer;
    VkDeviceMemory skyboxVertexBufferMemory;
    VkBuffer skyboxIndexBuffer;
    VkDeviceMemory skyboxIndexBufferMemory;
    // Uniform data
    struct {
        prt::vector<VkBuffer> skybox;
        prt::vector<VkDeviceMemory> skyboxMemory;

        prt::vector<VkBuffer> model;
        prt::vector<VkDeviceMemory> modelMemory;
    } uniformBuffers;
    
    // Descriptors
    struct {
        VkDescriptorPool skybox;
        VkDescriptorPool model;
    } descriptorPools;

    struct {
        prt::vector<VkDescriptorSet> skybox;
        prt::vector<VkDescriptorSet> model;
    } descriptorSets;

    // Commands
    prt::vector<VkCommandBuffer> commandBuffers;
    // Contains the indirect drawing commands
	VkBuffer indirectCommandBuffer;
    VkDeviceMemory indirectCommandBufferMemory;
    // Concurrency
    prt::vector<VkSemaphore> imageAvailableSemaphores;
    prt::vector<VkSemaphore> renderFinishedSemaphores;
    prt::vector<VkFence> inFlightFences;
    prt::vector<VkFence> imagesInFlight;
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
    
    void createDescriptorSetLayouts();

    void createPipelineCaches();

    void createGraphicsPipeline(VkVertexInputBindingDescription& bindingDescription,
                                prt::vector<VkVertexInputAttributeDescription>& attributeDescription,
                                VkDescriptorSetLayout& VkDescriptorSetLayout,
                                const std::string& vertShader, const std::string& fragShader,
                                VkPipeline& pipeline, VkPipelineCache& pipelineCache,
                                VkPipelineLayout& pipelineLayout);

    void createGraphicsPipelines();
    
    void createFramebuffers();
    
    void createCommandPool(); 

    void createColorResources();
    
    void createDepthResources();
    
    VkFormat findSupportedFormat(const prt::vector<VkFormat>& candidates, 
                                 VkImageTiling tiling, VkFormatFeatureFlags features);
    
    VkFormat findDepthFormat();
    
    bool hasStencilComponent(VkFormat format);
    
    void createTextureImage(VkImage& texImage, VkDeviceMemory& texImageMemory, const Texture& texture);
    void createCubeMapImage(VkImage& texImage, VkDeviceMemory& texImageMemory, const prt::array<Texture, 6>& textures);
    
    void createTextureImageView(VkImageView& imageView, VkImage &image);
    void createCubeMapImageView(VkImageView& imageView, VkImage &image);

    void generateMipmaps(VkImage image, VkFormat imageFormat, 
                         int32_t texWidth, int32_t texHeight, 
                         uint32_t mipLevels, uint32_t layerCount);
    
    VkSampleCountFlagBits getMaxUsableSampleCount();
    
    
    void createSamplers();
    
    VkImageView createImageView(VkImage image, VkFormat format, 
                                VkImageAspectFlags aspectFlags, 
                                VkImageViewType viewType,
                                uint32_t mipLevels,
                                uint32_t layerCount);
    
    void createImage(uint32_t width, uint32_t height, 
                     uint32_t mipLevels, uint32_t arrayLayers,
                     VkImageCreateFlags flags,
                     VkSampleCountFlagBits numSamples, VkFormat format, 
                     VkImageTiling tiling, VkImageUsageFlags usage, 
                     VkMemoryPropertyFlags properties, VkImage& image, 
                     VkDeviceMemory& imageMemory);
    
    void transitionImageLayout(VkImage image, VkFormat format, 
                               VkImageLayout oldLayout, VkImageLayout newLayout, 
                               uint32_t mipLevels, uint32_t layerCount);
    
    void copyBufferToImage(VkBuffer buffer, VkImage image, 
                           uint32_t width, uint32_t height,
                           uint32_t layerCount);
    
    void createVertexBuffer(const prt::vector<Model>& models);
    
    void createIndexBuffer(const prt::vector<Model>& models);

    void createSkyboxBuffers();

    void createDrawCommands(size_t imageIndex);
    
    void createUniformBuffers();
    
    void createDescriptorPools();
    
    void createDescriptorSets();

    void loadModels(const prt::vector<Model>& models);

    void loadSkybox(const prt::array<Texture, 6>& skybox);

    void createRenderJobs(const prt::vector<Model>& models, const prt::vector<uint32_t>& modelIndices);
    
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
    void createCommandBuffer(size_t imageIndex);

    void createSyncObjects();
    
    void updateUniformBuffers(uint32_t currentImage, 
                              const prt::vector<glm::mat4>& modelMatrices, 
                              const glm::mat4& viewMatrix, 
                              const glm::mat4& projectionMatrix, 
                              glm::vec3 viewPosition,
                              const glm::mat4& skyProjectionMatrix);
    
    void drawFrame(const prt::vector<glm::mat4>& modelMatrices, 
                   const glm::mat4& viewMatrix, 
                   const glm::mat4& projectionMatrix, 
                   glm::vec3 viewPosition,
                   const glm::mat4& skyProjectionMatrix);
    
    VkShaderModule createShaderModule(const char* filename);
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
    
    static prt::vector<char> readFile(const char* filename);
    
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/, 
                                                        VkDebugUtilsMessageTypeFlagsEXT /*messageType*/, 
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
                                                        void* /*pUserData*/);
    
};

#endif