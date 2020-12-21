#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include "src/graphics/geometry/model.h"

#include "graphics_pipeline.h"

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
    prt::vector<VkDescriptorImageInfo> descriptorImageInfos;
};

struct UniformBufferData {
    prt::vector<char> uboData{prt::getAlignment(alignof(std::max_align_t))};
    prt::vector<void*> mappedMemories;
    prt::vector<VkBuffer> uniformBuffers;
    prt::vector<VkDeviceMemory> uniformBufferMemories;
};

struct Assets {
    VertexData vertexData;
    TextureImages textureImages;
};

struct FrameBufferAttachment {
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
};

struct Cascade {
    VkFramebuffer frameBuffer;
    VkDescriptorSet descriptorSet;
    VkImageView imageView;
    VkDescriptorImageInfo descriptors;
};

struct SubPass {
    VkPipelineBindPoint bindPoint;
    prt::vector<VkAttachmentReference> colorReferences;
    VkAttachmentReference depthReference = {0,VK_IMAGE_LAYOUT_END_RANGE}; // use VK_IMAGE_LAYOUT_END_RANGE to signify no depth reference
    prt::vector<VkAttachmentReference> inputReferences;
};

struct RenderPass {
    prt::vector<VkAttachmentDescription> attachments;
    prt::vector<SubPass> subpasses;
    prt::vector<VkSubpassDependency> dependencies;

    VkRenderPass renderPass;
};


enum RenderGroupFlags : uint16_t {
    RENDER_GROUP_FLAG_0 = 1<<0,
    RENDER_GROUP_FLAG_1 = 1<<1,
    RENDER_GROUP_FLAG_2 = 1<<2,
    RENDER_GROUP_FLAG_3 = 1<<3,
    RENDER_GROUP_FLAG_4 = 1<<4,
    RENDER_GROUP_FLAG_5 = 1<<5,
    RENDER_GROUP_FLAG_6 = 1<<6,
    RENDER_GROUP_FLAG_7 = 1<<7,
    RENDER_GROUP_FLAG_8 = 1<<8,
    RENDER_GROUP_FLAG_9 = 1<<9,
    RENDER_GROUP_FLAG_10 = 1<<10,
    RENDER_GROUP_FLAG_11 = 1<<11,
    RENDER_GROUP_FLAG_12 = 1<<12,
    RENDER_GROUP_FLAG_13 = 1<<13,
    RENDER_GROUP_FLAG_14 = 1<<14,
    RENDER_GROUP_FLAG_15 = 1<<15,
    RENDER_GROUP_FLAG_ALL = uint16_t(~0),
    RENDER_GROUP_FLAG_NONE = 0,
};

template<typename INT_T>
bool checkMask(uint16_t mask, INT_T i) {
    assert(i <= sizeof(mask)*CHAR_BIT);
    return mask & (1 << i);
}

class VulkanApplication {
public:
    VulkanApplication(unsigned int width, unsigned int height);
    ~VulkanApplication();

    VulkanApplication & operator=(const VulkanApplication&) = delete;
    VulkanApplication(const VulkanApplication&) = delete;

    void initWindow(unsigned int width, unsigned int height);
    void initVulkan();

    void render(uint16_t renderGroupMask = RENDER_GROUP_FLAG_ALL);
    
    GLFWwindow* getWindow() const { return _window; }
    void getWindowSize(int& w, int& h) { w = _width; h = _height; };
    
    bool isWindowOpen() { return !glfwWindowShouldClose(_window); }
protected:
    // command data
    VkCommandPool commandPool;
    uint16_t commandBufferRenderGroupMask = RENDER_GROUP_FLAG_ALL;
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

    prt::vector<RenderPass> renderPasses;
    size_t scenePassIndex;
    size_t offscreenPassIndex;

    prt::vector<FrameBufferAttachment> frameBufferAttachments;
    size_t colorFBAIndex;
    size_t depthFBAIndex;
    prt::vector<size_t> accumulationFBAIndices;
    prt::vector<size_t> revealageFBAIndices;
    prt::vector<size_t> offscreenFBAIndices;

    struct OffscreenPass {
        VkExtent2D extent;
        // prt::vector<FrameBufferAttachment> depths;
        // VkRenderPass renderPass;
        VkSampler depthSampler;
        prt::vector<VkDescriptorImageInfo> descriptors;
        VkDescriptorPool descriptorPool;
        VkDescriptorSetLayout descriptorSetLayout;

        prt::vector<prt::array<Cascade, NUMBER_SHADOWMAP_CASCADES> > cascades;
    } offscreenPass;

    prt::vector<GraphicsPipeline> graphicsPipelines;

    VkDevice& getDevice() { return device; }
    
    void createTextureImage(VkImage& texImage, VkDeviceMemory& texImageMemory, const Texture& texture);
    void createCubeMapImage(VkImage& texImage, VkDeviceMemory& texImageMemory, const prt::array<Texture, 6>& textures);
    
    void createTextureImageView(VkImageView& imageView, VkImage &image, uint32_t mipLevels);
    void createCubeMapImageView(VkImageView& imageView, VkImage &image, uint32_t mipLevels);

    void recreateSwapChain();
    void reprepareSwapChain();
    void completeSwapChain();
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
    static constexpr int32_t shadowmapDimension = 2048;
    static constexpr float depthBiasConstant = 0.01f;//1.25f;
    static constexpr float depthBiasSlope = 0.01f;//1.75f;

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
    void createRenderPass(RenderPass & renderpass);

    void createOffscreenSampler();
    void createOffscreenRenderPass();
    void createOffscreenFrameBuffer();

    void createTransparencyAttachments();

    void prepareGraphicsPipelines();
    
    void createDescriptorSetLayouts(prt::vector<GraphicsPipeline> & pipelines);
    void createDescriptorSetLayout(GraphicsPipeline & pipeline);

    void createPipelineCaches(prt::vector<GraphicsPipeline> & pipelines);
    void createPipelineCache(GraphicsPipeline & pipeline);

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
    
    void createDescriptorPools();
    void createDescriptorPool(GraphicsPipeline & pipeline);
    
    void createDescriptorSets();
    
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

    prt::vector<GraphicsPipeline*> getPipelinesByType(PipelineType type);
    
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