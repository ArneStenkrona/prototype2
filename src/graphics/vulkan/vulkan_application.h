#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include "src/graphics/vulkan/render_pass.h"

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

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;
    prt::vector<VkSurfaceFormatKHR> formats;
    prt::vector<VkPresentModeKHR>   presentModes;
};

struct VertexData {
    VkBuffer       vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer       indexBuffer;
    VkDeviceMemory indexBufferMemory;
};

struct TextureImages {
    prt::vector<VkImage>               images;
    prt::vector<VkDeviceMemory>        imageMemories;
    prt::vector<VkImageView>           imageViews;
    prt::vector<VkDescriptorImageInfo> descriptorImageInfos;
};

struct UniformBufferData {
    prt::vector<char>           uboData{prt::getAlignment(alignof(std::max_align_t))};
    prt::vector<void*>          mappedMemories;
    prt::vector<VkBuffer>       uniformBuffers;
    prt::vector<VkDeviceMemory> uniformBufferMemories;
};

struct Assets {
    VertexData vertexData;
    TextureImages textureImages;
};

struct DynamicAssets {
    prt::vector<VertexData> vertexData;
    prt::vector<TextureImages> textureImages;
};

struct FramebufferAttachment {
    VkImageLayout         imageLayout;
    VkImageCreateInfo     imageInfo;
    VkImageViewCreateInfo imageViewInfo;
    VkMemoryAllocateInfo  memoryInfo;
    VkImage               image = VK_NULL_HANDLE;
    VkDeviceMemory        memory = VK_NULL_HANDLE;
    VkImageView           imageView = VK_NULL_HANDLE;

    /*
     * TODO: find better solution to
     * swapchain recreation issues
     **/
    bool swapchainAttachment = false;
};

struct Cascade {
    size_t frameBufferIndex;
    VkFramebuffer framebuffer;
    VkImageView imageView;
};

struct CascadeShadowMap {
    prt::vector<prt::array<Cascade, NUMBER_SHADOWMAP_CASCADES> > cascades;
    size_t renderPassIndex;
};

struct SwapchainFBACopy {
    size_t               swapchainAttachmentIndex;
    VkBufferImageCopy    region;
    VkBuffer             buffer;
    VkDeviceMemory       bufferMemory;
    size_t               bufferSize;
    void *               data;
};

// TODO: remove redundant "swapchain" from all
// member field names
struct Swapchain {
    // Swapchain data
    uint32_t                                    swapchainImageCount;
    uint32_t                                    previousImageIndex = 0;
    VkSwapchainKHR                              swapchain;
    prt::vector<VkImage>                        swapchainImages;
    VkFormat                                    swapchainImageFormat;
    VkExtent2D                                  swapchainExtent;
    prt::vector<VkImageView>                    swapchainImageViews;
    prt::vector<VkFramebuffer>                  swapchainFramebuffers;
    prt::vector<prt::vector<size_t> >           swapchainFBAindices;
    prt::vector<prt::vector<SwapchainFBACopy> > swapchainFBACopies;

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

    // void render(uint16_t renderGroupMask = RENDER_GROUP_FLAG_ALL);

    void updateRenderGroupMask(int16_t renderGroupMask);
    
    GLFWwindow* getWindow() const { return _window; }
    void getWindowSize(int& w, int& h) { w = _width; h = _height; };
    
    bool isWindowOpen() { return !glfwWindowShouldClose(_window); }
protected:
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    /*
     * TODO: Move this to where appropriate
     **/
    static constexpr int32_t shadowmapDimension = 2048;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    // render group mask
    uint16_t commandBufferRenderGroupMask = RENDER_GROUP_FLAG_ALL;

    // command data
    VkCommandPool commandPool; // TODO: rename to reflect that pool is for static commands
    VkCommandPool dynamicCommandPool;
    prt::vector<VkCommandBuffer> commandBuffers; // TODO: rename to reflect that these are static command buffers
    prt::vector<VkCommandBuffer> dynamicCommandBuffers;
    
    // Samplers
    VkSampler textureSampler;
    VkSampler shadowMapSampler;

    // Swapchain
    Swapchain swapchain;

    // Render passes
    prt::vector<RenderPass> renderPasses;
    size_t presentPassIndex;

    /*
     * TODO: Differentiate between swapchain-dependent FBAs
     * and regular FBAs so that not all FBAs need to be rebuilt
     * when the swapchain is recreated
     **/
    prt::vector<FramebufferAttachment> framebufferAttachments;

    prt::vector<CascadeShadowMap> shadowMaps;

    uint32_t waitForNextImageIndex();
    void drawFrame(uint32_t imageIndex);

    VkDevice & getDevice() { return device; }
    VkPhysicalDevice & getPhysicalDevice() { return physicalDevice; }

    size_t pushBackFramebufferAttachment();

    size_t addSwapchainFBA(size_t swapchainIndex, size_t fbaIndex);
    size_t addSwapchainFBACopy(size_t swapchainIndex, size_t swapchainFBAIndex,
                               uint32_t width, uint32_t height, VkImageAspectFlags aspectFlags);

    size_t pushBackRenderPass(bool isPresentPass = false);
    size_t pushBackShadowMap(size_t renderPassIndex);
    
    void createTextureImage(VkImage& texImage, VkDeviceMemory& texImageMemory, const Texture& texture);
    void createCubeMapImage(VkImage& texImage, VkDeviceMemory& texImageMemory, const prt::array<Texture, 6>& textures);
    
    void createTextureImageView(VkImageView& imageView, VkImage &image, uint32_t mipLevels);
    void createCubeMapImageView(VkImageView& imageView, VkImage &image, uint32_t mipLevels);

    void recreateSwapchain();
    void reprepareSwapchain();
    void completeSwapchain();
    void cleanupSwapchain();

    void createAndMapBuffer(void* bufferData, VkDeviceSize bufferSize, VkBufferUsageFlagBits bufferUsageFlagBits,
                            VkBuffer& destinationBuffer, VkDeviceMemory& destinationBufferMemory);
    
    size_t pushBackPipeline();

    inline GraphicsPipeline & getPipeline(size_t index) { return graphicsPipelines[index]; }
    inline GraphicsPipeline const & getPipeline(size_t index) const { return graphicsPipelines[index]; }

    size_t pushBackAssets();

    inline Assets & getAssets(size_t index) { return assets[index]; } 
    inline Assets const & getAssets(size_t index) const { return assets[index]; } 

    size_t pushBackDynamicAssets();

    inline DynamicAssets& getDynamicAssets(size_t index) { return dynamicAssets[index]; } 
    inline DynamicAssets const & getDynamicAssets(size_t index) const { return dynamicAssets[index]; } 

    size_t pushBackUniformBufferData(size_t uboSize);

    inline UniformBufferData& getUniformBufferData(size_t index) { return uniformBufferDatas[index]; } 
    inline UniformBufferData const & getUniformBufferData(size_t index) const { return uniformBufferDatas[index]; } 
    
    prt::vector<size_t> getPipelineIndicesByType(PipelineType type);
    prt::vector<size_t> getPipelineIndicesBySubPass(RenderPass const & renderPass, unsigned int subpass);
    
    VkFormat findDepthFormat();
    float depthToFloat(void * depthValue, VkFormat depthFormat);

private:
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
    VkDevice device;

    // Synchronization
    prt::vector<VkSemaphore> imageAvailableSemaphores;
    prt::vector<VkSemaphore> renderFinishedSemaphores;
    prt::vector<VkFence> inFlightFences;
    prt::vector<VkFence> imagesInFlight;
    unsigned int currentFrame = 0;

    prt::vector<GraphicsPipeline> graphicsPipelines;

    prt::vector<Assets> assets;
    prt::vector<DynamicAssets> dynamicAssets;
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
    
    void createSwapchain();
    
    void createSwapchainImageViews();
    
    void createShadowMapSampler();

    void prepareGraphicsPipelines();
    
    void createDescriptorSetLayouts(prt::vector<GraphicsPipeline> & pipelines);
    void createDescriptorSetLayout(GraphicsPipeline & pipeline);

    void createPipelineCaches(prt::vector<GraphicsPipeline> & pipelines);
    void createPipelineCache(GraphicsPipeline & pipeline);

    void createGraphicsPipelines(prt::vector<GraphicsPipeline> const & pipelines);
    void createGraphicsPipeline(GraphicsPipeline & materialPipeline);
    
    void createCommandPool(VkCommandPool & pool, VkCommandPoolCreateFlags flags); 
    void createCommandBuffers();
    void createDynamicCommandBuffers();
    void createCommandBuffer(size_t const imageIndex);
    void updateDynamicCommandBuffer(size_t const imageIndex);

    void createSwapchainFrameBuffers();
    void createSwapchainFBACopies();
    
    void createDrawCommands(size_t const imageIndex, GraphicsPipeline & pipeline);
    void createRenderPassCommands(size_t const imageIndex, RenderPass & renderPass);

    void createRenderPasses();
    void createRenderPass(RenderPass & renderpass);
    
    void createFramebufferAttachments();

    void createShadowMaps();
    void createShadowMap(CascadeShadowMap & shadowMap);
    
    VkFormat findSupportedFormat(prt::vector<VkFormat> const & candidates, 
                                 VkImageTiling tiling, VkFormatFeatureFlags features);
        
    bool hasStencilComponent(VkFormat format);

    void generateMipmaps(VkImage image, VkFormat imageFormat, 
                         int32_t texWidth, int32_t texHeight, 
                         uint32_t mipLevels, uint32_t layerCount);
    
    VkSampleCountFlagBits getMaxUsableSampleCount();
    
    
    void createTextureSampler();
    
    VkImageView createImageView(VkImageViewCreateInfo & viewInfo);
    
    void createImage(VkImageCreateInfo & imageInfo, 
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
    
    VkShaderModule createShaderModule(const char* filename);
    VkShaderModule createShaderModule(const prt::vector<char>& code);
    
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const prt::vector<VkSurfaceFormatKHR>& availableFormats);
    
    VkPresentModeKHR chooseSwapPresentMode(const prt::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);

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