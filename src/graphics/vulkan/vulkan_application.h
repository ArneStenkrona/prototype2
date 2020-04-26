#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include "src/graphics/geometry/model.h"

#include "graphics_pipeline.h"

#include "src/system/input/input.h"

#include "src/game/scene/scene.h"

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

struct VertexData {
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
};

struct TextureImages {
    prt::vector<VkImage> images;
    prt::vector<VkDeviceMemory> imageMemories;
    prt::vector<VkImageView> imageViews;
};

struct UniformBufferData {
    prt::vector<char> uboData{prt::getAlignment(alignof(std::max_align_t))};
    prt::vector<void*> mappedMemories;
    prt::vector<VkBuffer> uniformBuffers;
    prt::vector<VkDeviceMemory> uniformBufferMemories;
};

struct DrawCall {
    uint32_t firstIndex;
    uint32_t indexCount;
    // typedef prt::array<uint32_t, 2> ConstantType;
    using PushConstants = prt::array<unsigned char, 32>;
    PushConstants pushConstants;
};

struct Assets {
    VertexData vertexData;
    TextureImages textureImages;
    prt::vector<DrawCall> drawCalls;
};
    // UniformBufferData uniformBufferData;

struct FrameBufferAttachment {
		VkImage image;
		VkDeviceMemory memory;
		VkImageView imageView;
};

class VulkanApplication {
public:
    VulkanApplication(unsigned int width, unsigned int height);
    ~VulkanApplication();

    VulkanApplication & operator=(const VulkanApplication&) = delete;
    VulkanApplication(const VulkanApplication&) = delete;

    void initWindow(unsigned int width, unsigned int height);
    void initVulkan();
    
    GLFWwindow* getWindow() const { return _window; }
    void getWindowSize(int& w, int& h) { w = _width; h = _height; };
    
    bool isWindowOpen() { return !glfwWindowShouldClose(_window); }
protected:
    // command data
    VkCommandPool commandPool;
    prt::vector<VkCommandBuffer> commandBuffers;
    
    // Sampler
    VkSampler textureSampler;

    // Swapchain data
    VkSwapchainKHR swapChain;
    prt::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    prt::vector<VkImageView> swapChainImageViews;
    prt::vector<VkFramebuffer> swapChainFramebuffers;
    
    VkRenderPass scenePass;

    struct OffscreenPass {
        VkExtent2D extent;
        prt::vector<VkFramebuffer> frameBuffers;
        prt::vector<FrameBufferAttachment> depths;
        VkRenderPass renderPass;
        VkSampler depthSampler;
        VkDescriptorImageInfo descriptor;
    } offscreenPass;
    
    struct {
        prt::vector<GraphicsPipeline> offscreen;
        prt::vector<GraphicsPipeline> scene;
    } graphicsPipelines;

    void update();

    VkDevice& getDevice() { return device; }
    
    void createTextureImage(VkImage& texImage, VkDeviceMemory& texImageMemory, const Texture& texture);
    void createCubeMapImage(VkImage& texImage, VkDeviceMemory& texImageMemory, const prt::array<Texture, 6>& textures);
    
    void createTextureImageView(VkImageView& imageView, VkImage &image, uint32_t mipLevels);
    void createCubeMapImageView(VkImageView& imageView, VkImage &image, uint32_t mipLevels);

    void recreateSwapChain();
    void cleanupSwapChain();

    void createAndMapBuffer(void* bufferData, VkDeviceSize bufferSize, VkBufferUsageFlagBits bufferUsageFlagBits,
                        VkBuffer& destinationBuffer, VkDeviceMemory& destinationBufferMemory);
    
    size_t pushBackAssets();

    inline Assets& getAssets(size_t index) { return assets[index]; } 
    inline Assets const & getAssets(size_t index) const { return assets[index]; } 

    size_t pushBackUniformBufferData(size_t uboSize);

    inline UniformBufferData& getUniformBufferData(size_t index) { return uniformBufferDatas[index]; } 
    inline UniformBufferData const & getUniformBufferData(size_t index) const { return uniformBufferDatas[index]; } 

private:
    static constexpr int32_t shadowmapDimension = 1024;
    static constexpr float depthBiasConstant = 1.25f;
    static constexpr float depthBiasSlope = 1.75f;

    static constexpr unsigned int maxFramesInFlight = 2;

    static constexpr prt::array<const char*, 1> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    static constexpr prt::array<const char*, 1> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    #ifdef NDEBUG
    static constexpr bool enableValidationLayers = false;
    #else
    static constexpr bool enableValidationLayers = true;
    #endif

    GLFWwindow* _window;
    int _width, _height;
    
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    
    VkPhysicalDevice physicalDevice;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkDevice device;
    
    VkQueue graphicsQueue;
    VkQueue presentQueue;
        
    FrameBufferAttachment colorAttachment;
    FrameBufferAttachment depthAttachment;

    // Synchronization
    prt::vector<VkSemaphore> imageAvailableSemaphores;
    prt::vector<VkSemaphore> renderFinishedSemaphores;
    prt::vector<VkFence> inFlightFences;
    prt::vector<VkFence> imagesInFlight;
    unsigned int currentFrame = 0;

    prt::vector<Assets> assets;
    prt::vector<UniformBufferData> uniformBufferDatas;
    
    bool framebufferResized = false;

    static constexpr bool enableMsaa = false;

    void cleanup();
    
    static void framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/);
            
    void createInstance();
    
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    
    void setupDebugMessenger();
    
    void createSurface();
    
    void pickPhysicalDevice();
    
    void createLogicalDevice();
    
    void createSwapChain();
    
    void createImageViews();
    
    void createScenePass();
    void createScenePassMsaa();
    void createScenePassNoMsaa();

    void createOffscreenSampler();
    void createOffscreenRenderPass();
    void createOffscreenFrameBuffer();
    
    void connectRenderPasses();

    void createDescriptorSetLayouts(prt::vector<GraphicsPipeline> const & pipelines);

    void createPipelineCaches(prt::vector<GraphicsPipeline> const & pipelines);

    void createShadowMapPipeline();
    void createGraphicsPipelines(prt::vector<GraphicsPipeline> const & pipelines);
    void createGraphicsPipeline(GraphicsPipeline & materialPipeline);
    
    void createFramebuffers();
    
    void createCommandPool(); 
    void createCommandBuffers();
    void createCommandBuffer(size_t const imageIndex);
    void createOffscreenCommands(size_t const imageIndex);
    void createSceneCommands(size_t const imageIndex);
    void createDrawCommands(size_t const imageIndex, GraphicsPipeline & pipeline);

    void createColorResources();
    
    void createDepthResources();
    
    VkFormat findSupportedFormat(prt::vector<VkFormat> const & candidates, 
                                 VkImageTiling tiling, VkFormatFeatureFlags features);
    
    VkFormat findDepthFormat();
    
    bool hasStencilComponent(VkFormat format);

    void generateMipmaps(VkImage image, VkFormat imageFormat, 
                         int32_t texWidth, int32_t texHeight, 
                         uint32_t mipLevels, uint32_t layerCount);
    
    VkSampleCountFlagBits getMaxUsableSampleCount();
    
    
    void createTextureSampler();
    
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
    
    void createDescriptorPools(prt::vector<GraphicsPipeline> const & pipelines);
    
    void createDescriptorSets(/*prt::vector<GraphicsPipeline> const & pipelines*/);
    
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                      VkMemoryPropertyFlags properties, VkBuffer& buffer, 
                      VkDeviceMemory& bufferMemory);


    VkCommandBuffer beginSingleTimeCommands();
    
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createSyncObjects();

    void updateUniformBuffers(uint32_t currentImage);
    
    void drawFrame();
    
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