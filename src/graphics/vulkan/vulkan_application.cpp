#include "vulkan_application.h"

#include "src/config/prototype2Config.h"

#include "src/container/array.h"
#include "src/container/hash_set.h"

#include <chrono>
#include <thread>

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VulkanApplication::VulkanApplication(unsigned int width, unsigned int height) {
    initWindow(width, height);
    initVulkan();
}

VulkanApplication::~VulkanApplication() {
    cleanup();
}

void VulkanApplication::initWindow(unsigned int width, unsigned int height) {
    glfwInit();
 
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
 
    _window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}
    
void VulkanApplication::framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/) {
    auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}
    
void VulkanApplication::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createOffscreenFrameBuffer();
    prepareGraphicsPipelines();
    createCommandPool();
    createTextureSampler();
    createOffscreenSampler();
    createSyncObjects();
    createDescriptorPools();
}

void VulkanApplication::render(uint16_t renderGroupMask) {
    if (renderGroupMask != commandBufferRenderGroupMask) {
        commandBufferRenderGroupMask = renderGroupMask;
        /* rebuild command buffers */
        vkDeviceWaitIdle(device); // could perhaps do something more performant than vkDeviceWaitIdle()
        vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        createCommandBuffers();

    }
    drawFrame();
}
    
void VulkanApplication::cleanupSwapChain() {
    vkDeviceWaitIdle(device);

    for (FrameBufferAttachment & fba : frameBufferAttachments) {
        vkDestroyImageView(device, fba.imageView, nullptr);
        vkDestroyImage(device, fba.image, nullptr);
        vkFreeMemory(device, fba.memory, nullptr);
    }
    frameBufferAttachments.resize(0);

    // for (auto & depth : offscreenPass.depths) {
    //     vkDestroyImageView(device, depth.imageView, nullptr);
    //     vkDestroyImage(device, depth.image, nullptr);
    //     vkFreeMemory(device, depth.memory, nullptr);
    // }

    for (auto & cascades : shadowMap.cascades) {
        for (auto & cascade : cascades) {
            vkDestroyImageView(device, cascade.imageView, nullptr);
        }
    }
 
    for (auto & framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto & cascades : shadowMap.cascades) {
        for (auto & cascade : cascades) {
            vkDestroyFramebuffer(device, cascade.frameBuffer, nullptr);
        }
    }
 
    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    for (auto & graphicsPipeline : graphicsPipelines) {
        vkDestroyPipelineCache(device, graphicsPipeline.pipelineCache, nullptr);
        vkDestroyPipeline(device, graphicsPipeline.pipeline, nullptr);
        vkDestroyPipelineLayout(device, graphicsPipeline.pipelineLayout, nullptr);
    }

    for (RenderPass & pass : renderPasses) {
        vkDestroyRenderPass(device, pass.renderPass, nullptr);
    }
    renderPasses.resize(0);
 
    for (auto & imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
 
    vkDestroySwapchainKHR(device, swapChain, nullptr);

    for (auto & graphicsPipeline : graphicsPipelines) {
        vkDestroyDescriptorPool(device, graphicsPipeline.descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, graphicsPipeline.descriptorSetLayout, nullptr);
    }
    
}
    
void VulkanApplication::cleanup() {
    cleanupSwapChain();

    for (auto & uniformBufferData : uniformBufferDatas) {
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vkUnmapMemory(device, uniformBufferData.uniformBufferMemories[i]);
            vkDestroyBuffer(device, uniformBufferData.uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBufferData.uniformBufferMemories[i], nullptr);
        }
    }

    for (auto & ass : assets) {
        for (size_t i = 0; i < ass.textureImages.imageViews.size(); i++) {
            vkDestroyImageView(device, ass.textureImages.imageViews[i], nullptr);
            vkDestroyImage(device, ass.textureImages.images[i], nullptr);
            vkFreeMemory(device, ass.textureImages.imageMemories[i], nullptr);
        }
    }

    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroySampler(device, shadowMap.depthSampler, nullptr);

    for (auto & ass : assets) {
        vkDestroyBuffer(device, ass.vertexData.vertexBuffer, nullptr);
        vkFreeMemory(device, ass.vertexData.vertexBufferMemory, nullptr);

        vkDestroyBuffer(device, ass.vertexData.indexBuffer, nullptr);
        vkFreeMemory(device, ass.vertexData.indexBufferMemory, nullptr);
    }

    for (unsigned int i = 0; i < maxFramesInFlight; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
 
    vkDestroyCommandPool(device, commandPool, nullptr);
 
    vkDestroyDevice(device, nullptr);
 
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
 
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
 
    glfwDestroyWindow(_window);
 
    glfwTerminate();
}

void VulkanApplication::recreateSwapChain() {
    reprepareSwapChain();
    completeSwapChain();
}

void VulkanApplication::reprepareSwapChain() {
     _width = 0;
    _height = 0;
    while (_width == 0 || _height == 0) {
        glfwGetFramebufferSize(_window, &_width, &_height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);
 
    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createScenePass();
    createOffscreenFrameBuffer();
    createTransparencyAttachments();
}

void VulkanApplication::completeSwapChain() {
    prepareGraphicsPipelines();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    createDescriptorPools();
    createDescriptorSets();
    createCommandBuffers();
}

void VulkanApplication::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        assert(false && "validation layers requested, but not available!");
    }
 
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Prototype2";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
 
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
 
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
 
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
     
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
     
        createInfo.pNext = nullptr;
    }
 
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        assert(false && "failed to create instance!");
    }
}
    
void VulkanApplication::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void VulkanApplication::setupDebugMessenger() {
    if (!enableValidationLayers) return;
    
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    
    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        assert(false && "failed to set up debug messenger!");
    }
}

void VulkanApplication::createSurface() {
    if (glfwCreateWindowSurface(instance, _window, nullptr, &surface) != VK_SUCCESS) {
        assert(false && "failed to create window surface!");
    }
}

void VulkanApplication::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        assert(false && "failed to find GPUs with Vulkan support!");
    }
    
    prt::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            msaaSamples = enableMsaa ? getMaxUsableSampleCount() : VK_SAMPLE_COUNT_1_BIT;
            break;
        }
    }
    
    if (!physicalDevice) {
        assert(false && "failed to find a suitable GPU!");
    }
}

void VulkanApplication::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    
    prt::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    prt::hash_set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    
    float queuePriority = 1.0f;
    for (auto it = uniqueQueueFamilies.begin(); it != uniqueQueueFamilies.end(); it++) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = it->value();
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
        
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.independentBlend = VK_TRUE;
    
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    
    createInfo.pEnabledFeatures = &deviceFeatures;
    
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        assert(false && "failed to create logical device!");
    }
    
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}
    
void VulkanApplication::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
    
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
    
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        assert(false && "failed to create swap chain!");
    }
    
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
    
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VulkanApplication::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    
    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, 
                                                 VK_IMAGE_ASPECT_COLOR_BIT, 
                                                 VK_IMAGE_VIEW_TYPE_2D,
                                                 1, 1);
    }
}

void VulkanApplication::createScenePass() {
    // push back new render pass
    scenePassIndex = renderPasses.size();
    renderPasses.push_back({});
    RenderPass & scenePass = renderPasses[scenePassIndex];

    scenePass.extent = swapChainExtent;
    // populate scene pass
    // attachments
    scenePass.attachments.resize(6);
    // colour
    scenePass.attachments[0].format = swapChainImageFormat;
    scenePass.attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    scenePass.attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    scenePass.attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    scenePass.attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    scenePass.attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    scenePass.attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    scenePass.attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // depth
    scenePass.attachments[1].format = findDepthFormat();
    scenePass.attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    scenePass.attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    scenePass.attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    scenePass.attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    scenePass.attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    scenePass.attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    scenePass.attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    // accumulation
    scenePass.attachments[2].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    scenePass.attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    scenePass.attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    scenePass.attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    scenePass.attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    scenePass.attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    scenePass.attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    scenePass.attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // revealage
    scenePass.attachments[3].format = VK_FORMAT_R8_UNORM;
    scenePass.attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
    scenePass.attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    scenePass.attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    scenePass.attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    scenePass.attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    scenePass.attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    scenePass.attachments[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // input accumulation
    scenePass.attachments[4].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    scenePass.attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
    scenePass.attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    scenePass.attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    scenePass.attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    scenePass.attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    scenePass.attachments[4].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    scenePass.attachments[4].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // input revealage
    scenePass.attachments[5].format = VK_FORMAT_R8_UNORM;
    scenePass.attachments[5].samples = VK_SAMPLE_COUNT_1_BIT;
    scenePass.attachments[5].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    scenePass.attachments[5].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    scenePass.attachments[5].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    scenePass.attachments[5].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    scenePass.attachments[5].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    scenePass.attachments[5].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // subpasses
    scenePass.subpasses.resize(3);
    scenePass.subpasses[0].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    scenePass.subpasses[0].colorReferences.resize(1);
    scenePass.subpasses[0].colorReferences[0].attachment = 0;
    scenePass.subpasses[0].colorReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    scenePass.subpasses[0].depthReference.attachment = 1;
    scenePass.subpasses[0].depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    scenePass.subpasses[1].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    scenePass.subpasses[1].colorReferences.resize(2);
    scenePass.subpasses[1].colorReferences[0].attachment = 2;
    scenePass.subpasses[1].colorReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    scenePass.subpasses[1].colorReferences[1].attachment = 3;
    scenePass.subpasses[1].colorReferences[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    scenePass.subpasses[1].depthReference.attachment = 1;
    scenePass.subpasses[1].depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    scenePass.subpasses[2].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    scenePass.subpasses[2].colorReferences.resize(1);
    scenePass.subpasses[2].colorReferences[0].attachment = 0;
    scenePass.subpasses[2].colorReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    scenePass.subpasses[2].inputReferences.resize(2);
    scenePass.subpasses[2].inputReferences[0].attachment = 4;
    scenePass.subpasses[2].inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    scenePass.subpasses[2].inputReferences[1].attachment = 5;
    scenePass.subpasses[2].inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    scenePass.dependencies.resize(3);
    scenePass.dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    scenePass.dependencies[0].dstSubpass = 0;
    scenePass.dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    scenePass.dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    scenePass.dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    scenePass.dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    scenePass.dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    scenePass.dependencies[1].srcSubpass = 0;
    scenePass.dependencies[1].dstSubpass = 1;
    scenePass.dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    scenePass.dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    scenePass.dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    scenePass.dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    scenePass.dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    scenePass.dependencies[2].srcSubpass = 1;
    scenePass.dependencies[2].dstSubpass = 2;
    scenePass.dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    scenePass.dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    scenePass.dependencies[2].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    scenePass.dependencies[2].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    scenePass.dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    createRenderPass(scenePass);
}

void VulkanApplication::createRenderPass(RenderPass & renderPass) {
    prt::vector<VkSubpassDescription> subpasses;
    subpasses.resize(renderPass.subpasses.size());
    for (size_t i = 0; i < subpasses.size(); ++i) {
        subpasses[i].pipelineBindPoint = renderPass.subpasses[i].bindPoint;

        subpasses[i].colorAttachmentCount = renderPass.subpasses[i].colorReferences.size();
        subpasses[i].pColorAttachments = renderPass.subpasses[i].colorReferences.data();

        subpasses[i].inputAttachmentCount = renderPass.subpasses[i].inputReferences.size();
        subpasses[i].pInputAttachments = renderPass.subpasses[i].inputReferences.data();

        subpasses[i].pDepthStencilAttachment = renderPass.subpasses[i].depthReference.layout != VK_IMAGE_LAYOUT_END_RANGE ?
                                               &renderPass.subpasses[i].depthReference :
                                               nullptr;
    }

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(renderPass.attachments.size());
    renderPassInfo.pAttachments = renderPass.attachments.data();
    renderPassInfo.subpassCount = renderPass.subpasses.size();
    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.dependencyCount = renderPass.dependencies.size();
    renderPassInfo.pDependencies = renderPass.dependencies.data();

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass.renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void VulkanApplication::createOffscreenSampler() {
    // Create sampler to sample from depth attachment
    // Used to sample in the fragment shder for shadowed rendering
    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.mipLodBias = 0.0f;
    sampler.maxAnisotropy = 1.0f;
    sampler.minLod = 0.0f;
    sampler.maxLod = 1.0f;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    if (vkCreateSampler(device, &sampler, nullptr, &shadowMap.depthSampler) != VK_SUCCESS) {
        assert(false && "failed to create sampler for offscreen depth image!");
    }
}

void VulkanApplication::createOffscreenRenderPass() {
    // push back new render pass
    offscreenPassIndex = renderPasses.size();
    renderPasses.push_back({});
    RenderPass & offscreen = renderPasses[offscreenPassIndex];

    offscreen.attachments.resize(1);
    offscreen.attachments[0].format = findDepthFormat();
    offscreen.attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    offscreen.attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    offscreen.attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    offscreen.attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    offscreen.attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    offscreen.attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    offscreen.attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    offscreen.subpasses.resize(1);
    offscreen.subpasses[0].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    offscreen.subpasses[0].depthReference.attachment = 0;
    offscreen.subpasses[0].depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    offscreen.dependencies.resize(2);
    offscreen.dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    offscreen.dependencies[0].dstSubpass = 0;
    offscreen.dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    offscreen.dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    offscreen.dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    offscreen.dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    offscreen.dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    offscreen.dependencies[1].srcSubpass = 0;
    offscreen.dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    offscreen.dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    offscreen.dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    offscreen.dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    offscreen.dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    offscreen.dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    createRenderPass(offscreen);
}

void VulkanApplication::createOffscreenFrameBuffer() {
    createOffscreenRenderPass();

    RenderPass & offscreen = renderPasses[offscreenPassIndex];
    offscreen.extent.width = shadowmapDimension;
    offscreen.extent.height = shadowmapDimension;

    offscreenFBAIndices.resize(swapChainImages.size());
    shadowMap.cascades.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        offscreenFBAIndices[i] = frameBufferAttachments.size();
        frameBufferAttachments.push_back({});
        FrameBufferAttachment & depthFBA = frameBufferAttachments.back();
        
        // shadpow map image
        VkImageCreateInfo image{};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.extent.width = offscreen.extent.width;
        image.extent.height = offscreen.extent.height;
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = NUMBER_SHADOWMAP_CASCADES;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.format = findDepthFormat();
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (vkCreateImage(device, &image, nullptr, &depthFBA.image) != VK_SUCCESS) {
            assert(false && "failed to create offscreen depth image!");
        }

        // depth memory
        VkMemoryAllocateInfo memAlloc{};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, depthFBA.image, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(device, &memAlloc, nullptr, &depthFBA.memory) != VK_SUCCESS) {
            assert(false && "failed to allocate memory for offscreen depth image!");
        }
        if (vkBindImageMemory(device, depthFBA.image, depthFBA.memory, 0) != VK_SUCCESS) {
            assert(false && "failed to bind memory for offscreen depth image!");
        }

        // depth image view
        VkImageViewCreateInfo depthStencilImageView{};
        depthStencilImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthStencilImageView.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        depthStencilImageView.format = findDepthFormat();
        depthStencilImageView.subresourceRange = {};
        depthStencilImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthStencilImageView.subresourceRange.baseMipLevel = 0;
        depthStencilImageView.subresourceRange.levelCount = 1;
        depthStencilImageView.subresourceRange.baseArrayLayer = 0;
        depthStencilImageView.subresourceRange.layerCount = NUMBER_SHADOWMAP_CASCADES;
        depthStencilImageView.image = depthFBA.image;
        if (vkCreateImageView(device, &depthStencilImageView, nullptr, &depthFBA.imageView) != VK_SUCCESS) {
            assert(false && "failed to create image view for offscreen depth image!");
        }

        // One image and framebuffer per cascade
        for (unsigned int j = 0; j < NUMBER_SHADOWMAP_CASCADES; ++j) {
            // Image view for this cascade's layer (inside the depth map)
            // This view is used to render to that specific depth image layer
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            viewInfo.format = findDepthFormat();
            viewInfo.subresourceRange = {};
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = j;
            viewInfo.subresourceRange.layerCount = 1;
            viewInfo.image = depthFBA.image;
            if (vkCreateImageView(device, &viewInfo, nullptr, &shadowMap.cascades[i][j].imageView) != VK_SUCCESS) {
                assert(false && "failed to create image view for offscreen frame buffer!");
            }
            // Create frame buffer
            VkFramebufferCreateInfo fbufCreateInfo{};
            fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbufCreateInfo.renderPass = renderPasses[offscreenPassIndex].renderPass;
            fbufCreateInfo.attachmentCount = 1;
            fbufCreateInfo.pAttachments = &shadowMap.cascades[i][j].imageView;
            fbufCreateInfo.width = offscreen.extent.width;
            fbufCreateInfo.height = offscreen.extent.height;
            fbufCreateInfo.layers = 1;
            if (vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &shadowMap.cascades[i][j].frameBuffer) != VK_SUCCESS) {
                assert(false && "failed to create offscreen frame buffer!");
            }

            shadowMap.cascades[i][j].descriptors.sampler = shadowMap.depthSampler;
            shadowMap.cascades[i][j].descriptors.imageView = depthFBA.imageView;
            shadowMap.cascades[i][j].descriptors.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        }
    }
}

void VulkanApplication::createTransparencyAttachments() {
    accumulationFBAIndices.resize(swapChainImages.size());
    revealageFBAIndices.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        accumulationFBAIndices[i] = frameBufferAttachments.size();
        frameBufferAttachments.push_back({});
        revealageFBAIndices[i] = frameBufferAttachments.size();
        frameBufferAttachments.push_back({});
        FrameBufferAttachment & accumFBA = frameBufferAttachments[accumulationFBAIndices[i]];
        FrameBufferAttachment & revFBA = frameBufferAttachments[revealageFBAIndices[i]];


        // accumulation image
        VkImageCreateInfo image{};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.extent.width = swapChainExtent.width;
        image.extent.height = swapChainExtent.height;
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;

        image.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        if (vkCreateImage(device, &image, nullptr, &accumFBA.image) != VK_SUCCESS) {
            assert(false && "failed to create accumulation image!");
        }
        // revealage image
        image.format = VK_FORMAT_R8_UNORM;
        if (vkCreateImage(device, &image, nullptr, &revFBA.image) != VK_SUCCESS) {
            assert(false && "failed to create revealage image!");
        }

        
        // accumulation memory
         VkMemoryAllocateInfo memAlloc{};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, accumFBA.image, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &memAlloc, nullptr, &accumFBA.memory) != VK_SUCCESS) {
            assert(false && "failed to allocate memory for accumulation image!");
        }
        if (vkBindImageMemory(device, accumFBA.image, accumFBA.memory, 0) != VK_SUCCESS) {
            assert(false && "failed to bind memory for accumulation image!");
        }
        // revealage memory
        vkGetImageMemoryRequirements(device, revFBA.image, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(device, &memAlloc, nullptr, &revFBA.memory) != VK_SUCCESS) {
            assert(false && "failed to allocate memory for revealage image!");
        }
        if (vkBindImageMemory(device, revFBA.image, revFBA.memory, 0) != VK_SUCCESS) {
            assert(false && "failed to bind memory for revealage image!");
        }

        transitionImageLayout(accumFBA.image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 1);
        transitionImageLayout(revFBA.image, VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 1);

        // accumulation view
        VkImageViewCreateInfo accumulationImageView{};
        accumulationImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        accumulationImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        accumulationImageView.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        accumulationImageView.subresourceRange = {};
        accumulationImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        accumulationImageView.subresourceRange.baseMipLevel = 0;
        accumulationImageView.subresourceRange.levelCount = 1;
        accumulationImageView.subresourceRange.baseArrayLayer = 0;
        accumulationImageView.subresourceRange.layerCount = 1;
        accumulationImageView.image = accumFBA.image;
        if (vkCreateImageView(device, &accumulationImageView, nullptr, &accumFBA.imageView) != VK_SUCCESS) {
            assert(false && "failed to create image view for accumulation image!");
        }
        
        // revealage view
        VkImageViewCreateInfo revealageImageView{};
        revealageImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        revealageImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        revealageImageView.format = VK_FORMAT_R8_UNORM;
        revealageImageView.subresourceRange = {};
        revealageImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        revealageImageView.subresourceRange.baseMipLevel = 0;
        revealageImageView.subresourceRange.levelCount = 1;
        revealageImageView.subresourceRange.baseArrayLayer = 0;
        revealageImageView.subresourceRange.layerCount = 1;
        revealageImageView.image = revFBA.image;
        if (vkCreateImageView(device, &revealageImageView, nullptr, &revFBA.imageView) != VK_SUCCESS) {
            assert(false && "failed to create image view for revealage image!");
        }
    }
}

void VulkanApplication::prepareGraphicsPipelines() {
    for (auto & pipeline : graphicsPipelines) {
        if (pipeline.type == PIPELINE_TYPE_OFFSCREEN) {
            pipeline.renderpass = renderPasses[offscreenPassIndex].renderPass;
        } else {
            pipeline.renderpass = renderPasses[scenePassIndex].renderPass;
        }
    }
    createDescriptorSetLayouts(graphicsPipelines);
    createPipelineCaches(graphicsPipelines);
    createGraphicsPipelines(graphicsPipelines);

}

void VulkanApplication::createDescriptorSetLayouts(prt::vector<GraphicsPipeline> & pipelines) {
    for (auto & pipeline : pipelines) {
        createDescriptorSetLayout(pipeline);
    }
}

void VulkanApplication::createDescriptorSetLayout(GraphicsPipeline & pipeline) {
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(pipeline.descriptorSetLayoutBindings.size());
    layoutInfo.pBindings = pipeline.descriptorSetLayoutBindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &pipeline.descriptorSetLayout) != VK_SUCCESS) {
        assert(false && "failed to create descriptor set layout!");
    }
}

void VulkanApplication::createPipelineCaches(prt::vector<GraphicsPipeline> & pipelines) {
    for (auto & pipeline : pipelines) {
        createPipelineCache(pipeline);
    }
}

void VulkanApplication::createPipelineCache(GraphicsPipeline & pipeline) {
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    if(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipeline.pipelineCache) != VK_SUCCESS) {
        assert(false && "failed to create pipeline cache");
    }
}

void VulkanApplication::createGraphicsPipeline(GraphicsPipeline & graphicsPipeline) {
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = graphicsPipeline.vertexInputAttributes.size();
    vertexInputInfo.pVertexBindingDescriptions = &graphicsPipeline.vertexInputBinding;
    vertexInputInfo.pVertexAttributeDescriptions = graphicsPipeline.vertexInputAttributes.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    prt::vector<VkDynamicState> dynamicStates;
    dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) graphicsPipeline.extent.width;
    viewport.height = (float) graphicsPipeline.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = graphicsPipeline.extent;
    
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    if (graphicsPipeline.enableDepthBias) {
        rasterizer.depthBiasEnable = VK_TRUE;
        dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    } else {
        rasterizer.depthBiasEnable = VK_FALSE;
    }

    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = msaaSamples;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = graphicsPipeline.type == PIPELINE_TYPE_TRANSPARENT ? VK_FALSE : VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    
    prt::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
    switch (graphicsPipeline.type) {
        case PIPELINE_TYPE_TRANSPARENT: {
            colorBlendAttachments.resize(2);
            colorBlendAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachments[0].blendEnable = VK_TRUE;
            colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

            colorBlendAttachments[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT;// | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachments[1].blendEnable = VK_TRUE;
            colorBlendAttachments[1].colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachments[1].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachments[1].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachments[1].alphaBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachments[1].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            colorBlendAttachments[1].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            break;
        }
        case PIPELINE_TYPE_COMPOSITION: {
            colorBlendAttachments.resize(1);
            colorBlendAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachments[0].blendEnable = VK_TRUE;
            colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            break;
        }
        default: {
            colorBlendAttachments.resize(1);
            colorBlendAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachments[0].blendEnable = VK_FALSE;

            colorBlending.blendConstants[0] = 0.0f;
            colorBlending.blendConstants[1] = 0.0f;
            colorBlending.blendConstants[2] = 0.0f;
            colorBlending.blendConstants[3] = 0.0f;
            break;
        }
    }
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = colorBlendAttachments.size();
    colorBlending.pAttachments = colorBlendAttachments.data();
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &graphicsPipeline.descriptorSetLayout;

    VkPushConstantRange pushConstantRange = {};
	pushConstantRange.offset = 0;
	pushConstantRange.size = DrawCall::PushConstants::DataSize;
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
    
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipeline.pipelineLayout) != VK_SUCCESS) {
        assert(false && "failed to create pipeline layout!");
    }

    prt::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
    shaderStageCreateInfos.resize(graphicsPipeline.shaderStages.size());
    
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStageCreateInfos.size();
    pipelineInfo.pStages = shaderStageCreateInfos.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = graphicsPipeline.pipelineLayout;
    pipelineInfo.renderPass = graphicsPipeline.renderpass;
    pipelineInfo.subpass = graphicsPipeline.subpass;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.pDynamicState = &dynamicStateCreateInfo;

    for (size_t i = 0; i < graphicsPipeline.shaderStages.size(); ++i) {
        shaderStageCreateInfos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfos[i].stage = graphicsPipeline.shaderStages[i].stage;
        shaderStageCreateInfos[i].pName = graphicsPipeline.shaderStages[i].pName;
        shaderStageCreateInfos[i].module = createShaderModule(graphicsPipeline.shaderStages[i].shader);
    }
    
    if (vkCreateGraphicsPipelines(device, graphicsPipeline.pipelineCache, 1, 
                                  &pipelineInfo, nullptr, &graphicsPipeline.pipeline) != VK_SUCCESS) {
        assert(false && "failed to create graphics pipeline!");
    }

    for (auto & shaderStageCreateInfo : shaderStageCreateInfos) {
        vkDestroyShaderModule(device, shaderStageCreateInfo.module, nullptr);
    }
}

void VulkanApplication::createGraphicsPipelines(prt::vector<GraphicsPipeline> const & pipelines) {
    for (auto & graphicsPipeline : pipelines) {
        createGraphicsPipeline(graphicsPipeline);
    }
}

void VulkanApplication::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());
    
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPasses[scenePassIndex].renderPass;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (enableMsaa) {
            assert(false && "msaa currently not supported!");
        } else {
            prt::array<VkImageView, 6> attachments = {
                swapChainImageViews[i],
                frameBufferAttachments[depthFBAIndex].imageView,
                frameBufferAttachments[accumulationFBAIndices[i]].imageView,
                frameBufferAttachments[revealageFBAIndices[i]].imageView,
                frameBufferAttachments[accumulationFBAIndices[i]].imageView,
                frameBufferAttachments[revealageFBAIndices[i]].imageView,
            };
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
        }

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            assert(false && "failed to create framebuffer!");
        }
    }
}

void VulkanApplication::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
    
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        assert(false && "failed to create graphics command pool!");
    }
}

void VulkanApplication::createColorResources() {
    VkFormat colorFormat = swapChainImageFormat;

    colorFBAIndex = frameBufferAttachments.size();
    frameBufferAttachments.push_back({});
    
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = 0;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapChainExtent.width;
    imageInfo.extent.height = swapChainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = colorFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.samples = msaaSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    createImage(imageInfo, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                frameBufferAttachments[colorFBAIndex].image, frameBufferAttachments[colorFBAIndex].memory);

    frameBufferAttachments[colorFBAIndex].imageView = createImageView(frameBufferAttachments[colorFBAIndex].image, colorFormat, 
                                                                      VK_IMAGE_ASPECT_COLOR_BIT, 
                                                                      VK_IMAGE_VIEW_TYPE_2D,
                                                                      1, 1);
    
    transitionImageLayout(frameBufferAttachments[colorFBAIndex].image, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 1);
}

void VulkanApplication::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    depthFBAIndex = frameBufferAttachments.size();
    frameBufferAttachments.push_back({});

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = 0;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapChainExtent.width;
    imageInfo.extent.height = swapChainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = msaaSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    createImage(imageInfo, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                frameBufferAttachments[depthFBAIndex].image, frameBufferAttachments[depthFBAIndex].memory);

    frameBufferAttachments[depthFBAIndex].imageView = createImageView(frameBufferAttachments[depthFBAIndex].image, depthFormat, 
                                                                      VK_IMAGE_ASPECT_DEPTH_BIT, 
                                                                      VK_IMAGE_VIEW_TYPE_2D,
                                                                      1, 1);
    
    transitionImageLayout(frameBufferAttachments[depthFBAIndex].image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);
}

VkFormat VulkanApplication::findSupportedFormat(prt::vector<VkFormat> const & candidates, 
                                                VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    
    assert(false && "failed to find supported format!");
    return VK_FORMAT_UNDEFINED;
}

VkFormat VulkanApplication::findDepthFormat() {
    static VkFormat format = findSupportedFormat(
                               {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
                               );
    return format;
}

bool VulkanApplication::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void VulkanApplication::createTextureImage(VkImage& texImage, VkDeviceMemory& texImageMemory, 
                                           const Texture& texture) {
    VkDeviceSize imageSize = texture.texWidth * texture.texHeight * 4;

    unsigned char* pixels = texture.pixelBuffer.data();
 
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 stagingBuffer, stagingBufferMemory);
 
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = 0;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = texture.texWidth;
    imageInfo.extent.height = texture.texHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = texture.mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    createImage(imageInfo, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texImage, texImageMemory);
 
    transitionImageLayout(texImage, VK_FORMAT_R8G8B8A8_UNORM, 
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                          texture.mipLevels, 1);

    copyBufferToImage(stagingBuffer, texImage, 
                      static_cast<uint32_t>(texture.texWidth), 
                      static_cast<uint32_t>(texture.texHeight),
                      1);

    // transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
 
    generateMipmaps(texImage, VK_FORMAT_R8G8B8A8_UNORM, 
                    texture.texWidth, texture.texHeight, 
                    texture.mipLevels, 1);
}

void VulkanApplication::createCubeMapImage(VkImage& texImage, VkDeviceMemory& texImageMemory, 
                                           const prt::array<Texture, 6>& textures) {
    VkDeviceSize layerSize = textures[0].texWidth * textures[0].texHeight * 4;
    VkDeviceSize imageSize = layerSize * textures.size();
 
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    for (size_t i = 0; i < textures.size(); i++) {
        memcpy(static_cast<void*>(static_cast<char*>(data) + (layerSize * i)),
               textures[i].pixelBuffer.data(), static_cast<size_t>(layerSize));
    }
    vkUnmapMemory(device, stagingBufferMemory);

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = textures[0].texWidth;
    imageInfo.extent.height = textures[0].texHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = textures[0].mipLevels;
    imageInfo.arrayLayers = textures.size();
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    createImage(imageInfo, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texImage, texImageMemory);
 
    transitionImageLayout(texImage, VK_FORMAT_R8G8B8A8_UNORM, 
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                          textures[0].mipLevels, textures.size());

    copyBufferToImage(stagingBuffer, texImage, 
                      static_cast<uint32_t>(textures[0].texWidth), 
                      static_cast<uint32_t>(textures[0].texHeight),
                      textures.size());

    //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
 
    generateMipmaps(texImage, VK_FORMAT_R8G8B8A8_UNORM, 
                    textures[0].texWidth, textures[0].texHeight, 
                    textures[0].mipLevels, textures.size());
}
    
void VulkanApplication::generateMipmaps(VkImage image, VkFormat imageFormat, 
                                        int32_t texWidth, int32_t texHeight, 
                                        uint32_t mipLevels, uint32_t layerCount) {

    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
    
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        assert(false && "texture image format does not support linear blitting!");
    }
    
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;
    barrier.subresourceRange.levelCount = 1;
    
    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;
    
    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        
        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
        
        VkImageBlit blit = {};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = layerCount;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = layerCount;
        
        vkCmdBlitImage(commandBuffer,
                       image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit,
                       VK_FILTER_LINEAR);
        
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
        
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }
    
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
    
    endSingleTimeCommands(commandBuffer);
}

VkSampleCountFlagBits VulkanApplication::getMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    
    VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
    
    return VK_SAMPLE_COUNT_1_BIT;
}

void VulkanApplication::createTextureImageView(VkImageView& imageView, VkImage &image, uint32_t mipLevels) {
    imageView = createImageView(image, VK_FORMAT_R8G8B8A8_UNORM, 
                                VK_IMAGE_ASPECT_COLOR_BIT, 
                                VK_IMAGE_VIEW_TYPE_2D,
                                mipLevels, 1);
}

void VulkanApplication::createCubeMapImageView(VkImageView& imageView, VkImage &image, uint32_t mipLevels) {
    imageView = createImageView(image, VK_FORMAT_R8G8B8A8_UNORM, 
                                VK_IMAGE_ASPECT_COLOR_BIT, 
                                VK_IMAGE_VIEW_TYPE_CUBE,
                                mipLevels, 6);
}

void VulkanApplication::createTextureSampler() {
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.anisotropyEnable = VK_FALSE;
	samplerCreateInfo.maxAnisotropy = 0.0f;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = static_cast<float>(12/*mipLevels*/);
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerCreateInfo, 0, &textureSampler) != VK_SUCCESS) {
        assert(false && "failed to create sampler!");
    }
}

VkImageView VulkanApplication::createImageView(VkImage image, VkFormat format, 
                                               VkImageAspectFlags aspectFlags, 
                                               VkImageViewType viewType,
                                               uint32_t mipLevels, uint32_t layerCount) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = layerCount;
    
    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        assert(false && "failed to create texture image view!");
    }
    
    return imageView;
}

void VulkanApplication::createImage(VkImageCreateInfo & imageInfo, VkMemoryPropertyFlags properties, 
                                    VkImage& image, VkDeviceMemory& imageMemory) {
    // VkImageCreateInfo imageInfo = {};
    // imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    // imageInfo.flags = flags;
    // imageInfo.imageType = VK_IMAGE_TYPE_2D;
    // imageInfo.extent.width = width;
    // imageInfo.extent.height = height;
    // imageInfo.extent.depth = 1;
    // imageInfo.mipLevels = mipLevels;
    // imageInfo.arrayLayers = arrayLayers;
    // imageInfo.format = format;
    // imageInfo.tiling = tiling;
    // imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // imageInfo.usage = usage;
    // imageInfo.samples = numSamples;
    // imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        assert(false && "failed to create image!");
    }
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        assert(false && "failed to allocate image memory!");
    }
    
    vkBindImageMemory(device, image, imageMemory, 0);
}

void VulkanApplication::transitionImageLayout(VkImage image, VkFormat format, 
                                              VkImageLayout oldLayout, VkImageLayout newLayout, 
                                              uint32_t mipLevels, uint32_t layerCount) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        
        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;
    
    VkPipelineStageFlags sourceStage = 0;
    VkPipelineStageFlags destinationStage = 0;
    
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else {
        assert(false && "unsupported layout transition!");
    }
    
    vkCmdPipelineBarrier(commandBuffer,
                         sourceStage, destinationStage,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
    
    endSingleTimeCommands(commandBuffer);
}

void VulkanApplication::copyBufferToImage(VkBuffer buffer, VkImage image, 
                                          uint32_t width, uint32_t height,
                                          uint32_t layerCount) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    prt::vector<VkBufferImageCopy> regions{};
    regions.resize(layerCount);
    uint32_t offset = 0;
    uint32_t layerSize = width * height * 4;
    for (size_t i = 0; i < layerCount; i++) {    
        regions[i].bufferOffset = offset;
        regions[i].bufferRowLength = 0;
        regions[i].bufferImageHeight = 0;
        regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        regions[i].imageSubresource.mipLevel = 0;
        regions[i].imageSubresource.baseArrayLayer = i;
        regions[i].imageSubresource.layerCount = 1;
        regions[i].imageOffset = {0, 0, 0};
        regions[i].imageExtent = {
            width,
            height,
            1
        };
        offset += layerSize;
    }
    
    vkCmdCopyBufferToImage(commandBuffer, buffer, 
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                           regions.size(), regions.data());
    
    endSingleTimeCommands(commandBuffer);
}

void VulkanApplication::createDescriptorPools() {
    for (auto & pipeline : graphicsPipelines) {
        createDescriptorPool(pipeline);
    }
}

void VulkanApplication::createDescriptorPool(GraphicsPipeline & pipeline) {
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(pipeline.descriptorPoolSizes.size());
    poolInfo.pPoolSizes = pipeline.descriptorPoolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());
    
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &pipeline.descriptorPool) != VK_SUCCESS) {
        assert(false && "failed to create descriptor pool!");
    }
}

void VulkanApplication::createDescriptorSets() {
    for (auto & pipeline : graphicsPipelines) {
        prt::vector<VkDescriptorSetLayout> layout(swapChainImages.size(), pipeline.descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pipeline.descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
        allocInfo.pSetLayouts = layout.data();

        pipeline.descriptorSets.resize(swapChainImages.size());
        if (vkAllocateDescriptorSets(device, &allocInfo, pipeline.descriptorSets.data()) != VK_SUCCESS) {
            assert(false && "failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < swapChainImages.size(); i++) {     
                Assets& asset = assets[pipeline.assetsIndex];
            switch (pipeline.type) {
                case PipelineType::PIPELINE_TYPE_OFFSCREEN: {
                    /* Offscreen */
                    for (auto & descriptorWrite : pipeline.descriptorWrites[i]) {
                        descriptorWrite.dstSet = pipeline.descriptorSets[i];
                    }
                    pipeline.descriptorWrites[i][0].pBufferInfo = &pipeline.descriptorBufferInfos[i];     

                    vkUpdateDescriptorSets(device, static_cast<uint32_t>(pipeline.descriptorWrites[i].size()), 
                                           pipeline.descriptorWrites[i].data(), 0, nullptr);    

                    break;
                }
                case PipelineType::PIPELINE_TYPE_OPAQUE: {}
                case PipelineType::PIPELINE_TYPE_TRANSPARENT: {
                    /* Opaque and transparent */

                    for (auto & descriptorWrite : pipeline.descriptorWrites[i]) {
                        descriptorWrite.dstSet = pipeline.descriptorSets[i];
                    }
                    pipeline.descriptorWrites[i][0].pBufferInfo = &pipeline.descriptorBufferInfos[i];
                    for (size_t j = 0; j < asset.textureImages.descriptorImageInfos.size(); ++j) {
                        asset.textureImages.descriptorImageInfos[j].sampler = textureSampler;
                        asset.textureImages.descriptorImageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        asset.textureImages.descriptorImageInfos[j].imageView = asset.textureImages.imageViews[j];
                    }
                    // THIS IS REALLY AD HOC, SHOULD BE DONE MORE DYNAMICALLY
                    if (pipeline.descriptorWrites[i].size() >= 2) {
                        pipeline.descriptorWrites[i][1].pImageInfo = asset.textureImages.descriptorImageInfos.data();
                    }

                    // THIS IS REALLY AD HOC, SHOULD BE DONE MORE DYNAMICALLY
                    VkDescriptorImageInfo shadowDesc{};
                    if (pipeline.descriptorWrites[i].size() >= 4) {
                        shadowDesc.sampler = shadowMap.depthSampler;
                        shadowDesc.imageView = frameBufferAttachments[offscreenFBAIndices[i]].imageView;
                        shadowDesc.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

                        pipeline.descriptorWrites[i][3].pImageInfo = &shadowDesc;
                    }

                    vkUpdateDescriptorSets(device, static_cast<uint32_t>(pipeline.descriptorWrites[i].size()), 
                                           pipeline.descriptorWrites[i].data(), 0, nullptr);

                    break;
                }
                case PipelineType::PIPELINE_TYPE_COMPOSITION: {
                    /* Composition */
                    for (auto & descriptorWrite : pipeline.descriptorWrites[i]) {
                        descriptorWrite.dstSet = pipeline.descriptorSets[i];
                    }

                    FrameBufferAttachment & accFBA = frameBufferAttachments[accumulationFBAIndices[i]];
                    FrameBufferAttachment & revFBA = frameBufferAttachments[revealageFBAIndices[i]];

                    VkDescriptorImageInfo accDesc{};
                    accDesc.imageView = accFBA.imageView;
                    accDesc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    VkDescriptorImageInfo revDesc{};
                    revDesc.imageView = revFBA.imageView;
                    revDesc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    pipeline.descriptorWrites[i][0].pImageInfo = &accDesc;
                    pipeline.descriptorWrites[i][1].pImageInfo = &revDesc;

                    vkUpdateDescriptorSets(device, static_cast<uint32_t>(pipeline.descriptorWrites[i].size()), 
                                        pipeline.descriptorWrites[i].data(), 0, nullptr);
                    break;
                }
            }      
        }
    }
}

void VulkanApplication::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {    
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
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        assert(false && "failed to allocate buffer memory!");
    }
    
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void VulkanApplication::createAndMapBuffer(void* bufferData, VkDeviceSize bufferSize,
                                           VkBufferUsageFlagBits bufferUsageFlagBits,
                                           VkBuffer& destinationBuffer,
                                           VkDeviceMemory& destinationBufferMemory) {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, bufferData, (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
    
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsageFlagBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, destinationBuffer, destinationBufferMemory);
    
    copyBuffer(stagingBuffer, destinationBuffer, bufferSize);
    
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

size_t VulkanApplication::pushBackAssets() {
    size_t index = assets.size();
    assets.push_back({});
    return index;
}

size_t VulkanApplication::pushBackUniformBufferData(size_t uboSize) {
    size_t index = uniformBufferDatas.size();
    uniformBufferDatas.push_back({});
    UniformBufferData& uniformBufferData = uniformBufferDatas.back();
    uniformBufferData.uboData.resize(uboSize);
    uniformBufferData.mappedMemories.resize(swapChainImages.size());
    uniformBufferData.uniformBuffers.resize(swapChainImages.size());
    uniformBufferData.uniformBufferMemories.resize(swapChainImages.size());

    size_t size = uniformBufferData.uboData.size();
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        createBuffer(size, 
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                     uniformBufferData.uniformBuffers[i], 
                     uniformBufferData.uniformBufferMemories[i]);
        vkMapMemory(device, uniformBufferData.uniformBufferMemories[i], 0, size, 0, &uniformBufferData.mappedMemories[i]);
    }

    return index;
}

VkCommandBuffer VulkanApplication::beginSingleTimeCommands() {
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

void VulkanApplication::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanApplication::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    
    endSingleTimeCommands(commandBuffer);
}

uint32_t VulkanApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    assert(false && "failed to find suitable memory type!");
    return 0;
}

void VulkanApplication::createCommandBuffers() {
    commandBuffers.resize(swapChainFramebuffers.size());
    
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();
    
    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        assert(false && "failed to allocate command buffers!");
    }
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        createCommandBuffer(i);
    }
}

void VulkanApplication::createCommandBuffer(size_t const imageIndex) {    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    
    if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        assert(false && "failed to begin recording command buffer!");
    }
    
    createOffscreenCommands(imageIndex);
    createSceneCommands(imageIndex);

    if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
        assert(false && "failed to record command buffer!");
    }
}

void VulkanApplication::createOffscreenCommands(size_t const imageIndex) {
    VkClearValue clearValue = {};
    clearValue.depthStencil = { 1.0f, 0 };

    RenderPass & offscreen = renderPasses[offscreenPassIndex];

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = offscreen.renderPass;
    renderPassBeginInfo.renderArea.extent = offscreen.extent;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;


    VkViewport viewport;
    viewport.width = offscreen.extent.width;
    viewport.height = offscreen.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.x = 0;
    viewport.y = 0;
    vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);

    VkRect2D scissor;
    scissor.extent = offscreen.extent;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

    vkCmdSetDepthBias(commandBuffers[imageIndex],
                    depthBiasConstant,
                    0.0f,
                    depthBiasSlope);
    prt::vector<GraphicsPipeline*> offscreenPipelines = getPipelinesByType(PipelineType::PIPELINE_TYPE_OFFSCREEN);
    for (unsigned int i = 0; i < NUMBER_SHADOWMAP_CASCADES; ++i) {
        renderPassBeginInfo.framebuffer = shadowMap.cascades[imageIndex][i].frameBuffer;
        vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        for (GraphicsPipeline * pipeline : offscreenPipelines) {
            if (!checkMask(commandBufferRenderGroupMask, pipeline->renderGroup)) continue;
            // set cascade index push constant (this really should be handled in a prettier way)
            for (auto & drawCall : pipeline->drawCalls) {
                *reinterpret_cast<int32_t*>(&drawCall.pushConstants[4]) = i;
            }
            createDrawCommands(imageIndex, *pipeline);
        }
        vkCmdEndRenderPass(commandBuffers[imageIndex]);
    }
}

void VulkanApplication::createSceneCommands(size_t const imageIndex) {
    prt::array<VkClearValue, 4> clearValues = {};
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0 } };
    clearValues[1].depthStencil = { 1.0f, 0 };
    clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0 } };
    clearValues[3].color = { { 1.0f, 1.0f, 1.0f, 1.0 } };

    RenderPass & scenePass = renderPasses[scenePassIndex];

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = scenePass.renderPass;
    renderPassBeginInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = scenePass.extent;
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport;
    viewport.width = scenePass.extent.width;
    viewport.height = scenePass.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.x = 0;
    viewport.y = 0;
    vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
 
    VkRect2D scissor;
    scissor.extent = scenePass.extent;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);
    // render opaque geometry
    prt::vector<GraphicsPipeline*> opaque = getPipelinesByType(PipelineType::PIPELINE_TYPE_OPAQUE);
    for (GraphicsPipeline * pipeline : opaque) {
        if (!checkMask(commandBufferRenderGroupMask, pipeline->renderGroup)) continue;
        createDrawCommands(imageIndex, *pipeline);
    }
    // render transparent geometry
    vkCmdNextSubpass(commandBuffers[imageIndex], VK_SUBPASS_CONTENTS_INLINE);
    prt::vector<GraphicsPipeline*> transparent = getPipelinesByType(PipelineType::PIPELINE_TYPE_TRANSPARENT);
    for (GraphicsPipeline * pipeline : transparent) {
        if (!checkMask(commandBufferRenderGroupMask, pipeline->renderGroup)) continue;
        createDrawCommands(imageIndex, *pipeline);
    }
    // composite render data
    vkCmdNextSubpass(commandBuffers[imageIndex], VK_SUBPASS_CONTENTS_INLINE);
    prt::vector<GraphicsPipeline*> composition = getPipelinesByType(PipelineType::PIPELINE_TYPE_COMPOSITION);
    for (GraphicsPipeline * pipeline : composition) {
        if (!checkMask(commandBufferRenderGroupMask, pipeline->renderGroup)) continue;
        createDrawCommands(imageIndex, *pipeline);
    }
    vkCmdEndRenderPass(commandBuffers[imageIndex]);
}

void VulkanApplication::createDrawCommands(size_t const imageIndex, GraphicsPipeline & pipeline) {
    static constexpr VkDeviceSize offset = 0;
    
    vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    Assets& asset = assets[pipeline.assetsIndex];
    vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, &asset.vertexData.vertexBuffer, &offset);
    vkCmdBindIndexBuffer(commandBuffers[imageIndex], asset.vertexData.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            pipeline.pipelineLayout, 0, 1, &pipeline.descriptorSets[imageIndex], 0, nullptr);
    for (auto const & drawCall : pipeline.drawCalls) {
        vkCmdPushConstants(commandBuffers[imageIndex], pipeline.pipelineLayout, 
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                            0, 
                            drawCall.pushConstants.size() * sizeof(drawCall.pushConstants[0]), 
                            (void *)drawCall.pushConstants.data());

        vkCmdDrawIndexed(commandBuffers[imageIndex], 
                drawCall.indexCount,
                1,
                drawCall.firstIndex,
                0,
                0);
    }
}

void VulkanApplication::createSyncObjects() {
    imageAvailableSemaphores.resize(maxFramesInFlight);
    renderFinishedSemaphores.resize(maxFramesInFlight);
    inFlightFences.resize(maxFramesInFlight);
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
    
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (unsigned int i = 0; i < maxFramesInFlight; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            assert(false && "failed to create synchronization objects!");
        }
    }
}

void VulkanApplication::updateUniformBuffers(uint32_t currentImage) {
    for (auto & uniformBufferData : uniformBufferDatas) {
        size_t size = uniformBufferData.uboData.size();
        memcpy(uniformBufferData.mappedMemories[currentImage], uniformBufferData.uboData.data(), size);
    }
}

void VulkanApplication::drawFrame() {    
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, 
                                            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        assert(false && "failed to acquire swap chain image!");
    }
    
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }     
    updateUniformBuffers(imageIndex);
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        assert(false && "failed to submit draw command buffer!");
    }
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    
    presentInfo.pImageIndices = &imageIndex;
    
    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        assert(false && "failed to present swap chain image!");
    }
    
    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

prt::vector<GraphicsPipeline*> VulkanApplication::getPipelinesByType(PipelineType type) {
    prt::vector<GraphicsPipeline*> pipelines;
    for (auto & pipeline : graphicsPipelines) {
        if (pipeline.type == type) pipelines.push_back(&pipeline);
    }
    return pipelines;
}

VkShaderModule VulkanApplication::createShaderModule(const char* filename) {
    prt::vector<char> code = readFile(filename);
    return createShaderModule(code);
}

VkShaderModule VulkanApplication::createShaderModule(const prt::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        assert(false && "failed to create shader module!");
    }
    
    return shaderModule;
}

VkSurfaceFormatKHR VulkanApplication::chooseSwapSurfaceFormat(const prt::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    
    return availableFormats[0];
}

VkPresentModeKHR VulkanApplication::chooseSwapPresentMode(const prt::vector<VkPresentModeKHR>& availablePresentModes) {
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
    
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            assert(false);
            return availablePresentMode;
        } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = availablePresentMode;
        }
    }
    
    return bestMode;
}

VkExtent2D VulkanApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);
        
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        
        return actualExtent;
    }
}

SwapChainSupportDetails VulkanApplication::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }
    
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }
    
    return details;
}

bool VulkanApplication::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    
    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy && supportedFeatures.independentBlend;
}

bool VulkanApplication::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    
    prt::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
    prt::hash_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    
    return requiredExtensions.empty();
}

QueueFamilyIndices VulkanApplication::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    prt::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        
        if (queueFamily.queueCount > 0 && presentSupport) {
            indices.presentFamily = i;
        }
        
        if (indices.isComplete()) {
            break;
        }
        
        i++;
    }
    
    return indices;
}

prt::vector<const char*> VulkanApplication::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    prt::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    return extensions;
}

bool VulkanApplication::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    prt::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        
        if (!layerFound) {
            return false;
        }
    }
    
    return true;
}

prt::vector<char> VulkanApplication::readFile(const char* filename) {
    std::string filestring = std::string(filename);
    std::ifstream file(filestring, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cout << "failed to open file: " + filestring << std::endl;;
        assert(false && "failed to open file");
    }
    
    size_t fileSize = (size_t) file.tellg();
    prt::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    
    file.close();
    
    return buffer;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/, VkDebugUtilsMessageTypeFlagsEXT /*messageType*/, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    
    return VK_FALSE;
}