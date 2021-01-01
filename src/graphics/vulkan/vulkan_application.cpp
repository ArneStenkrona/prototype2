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
    createSwapchain();
    createSwapchainImageViews();
    prepareGraphicsPipelines();
    createCommandPool(commandPool, 0);
    createCommandPool(dynamicCommandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
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
    
void VulkanApplication::cleanupSwapchain() {
    vkDeviceWaitIdle(device);

    for (FramebufferAttachment & fba : framebufferAttachments) {
        if (fba.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, fba.imageView, nullptr);
            vkDestroyImage(device, fba.image, nullptr);
            vkFreeMemory(device, fba.memory, nullptr);
        }
    }

    for (CascadeShadowMap & map : shadowMaps) {
        for (auto & cascades : map.cascades) {
            for (auto & cascade : cascades) {
                vkDestroyImageView(device, cascade.imageView, nullptr);
            }
        }
    }
 
    for (auto & framebuffer : swapchain.swapchainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (CascadeShadowMap & map : shadowMaps) {
        for (auto & cascades : map.cascades) {
            for (auto & cascade : cascades) {
                vkDestroyFramebuffer(device, cascade.framebuffer, nullptr);
            }
        }
    }
 
    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    vkFreeCommandBuffers(device, dynamicCommandPool, static_cast<uint32_t>(dynamicCommandBuffers.size()), dynamicCommandBuffers.data());

    for (auto & graphicsPipeline : graphicsPipelines) {
        vkDestroyPipelineCache(device, graphicsPipeline.pipelineCache, nullptr);
        vkDestroyPipeline(device, graphicsPipeline.pipeline, nullptr);
        vkDestroyPipelineLayout(device, graphicsPipeline.pipelineLayout, nullptr);
    }

    for (RenderPass & pass : renderPasses) {
        if (pass.renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, pass.renderPass, nullptr);
        }
    }
 
    for (auto & imageView : swapchain.swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    for (auto & copies : swapchain.swapchainFBACopies) {
        for (SwapchainFBACopy & copy : copies) {
            vkDestroyBuffer(device, copy.buffer, nullptr);
            vkFreeMemory(device, copy.bufferMemory, nullptr);
        }
    }
 
    vkDestroySwapchainKHR(device, swapchain.swapchain, nullptr);

    for (auto & graphicsPipeline : graphicsPipelines) {
        vkDestroyDescriptorPool(device, graphicsPipeline.descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, graphicsPipeline.descriptorSetLayout, nullptr);
    }
    
}
    
void VulkanApplication::cleanup() {
    cleanupSwapchain();

    for (auto & uniformBufferData : uniformBufferDatas) {
        for (size_t i = 0; i < swapchain.swapchainImages.size(); i++) {
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
    vkDestroySampler(device, shadowMapSampler, nullptr);

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
    vkDestroyCommandPool(device, dynamicCommandPool, nullptr);
 
    vkDestroyDevice(device, nullptr);
 
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
 
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
 
    glfwDestroyWindow(_window);
 
    glfwTerminate();
}

void VulkanApplication::recreateSwapchain() {
    reprepareSwapchain();
    completeSwapchain();
}

void VulkanApplication::reprepareSwapchain() {
     _width = 0;
    _height = 0;
    while (_width == 0 || _height == 0) {
        glfwGetFramebufferSize(_window, &_width, &_height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);
 
    cleanupSwapchain();

    createSwapchain();
    createSwapchainImageViews();
}

void VulkanApplication::completeSwapchain() {
    createRenderPasses();
    prepareGraphicsPipelines();
    createFramebufferAttachments();
    createSwapchainFrameBuffers();
    createSwapchainFBACopies();
    createShadowMaps();
    createDescriptorPools();
    createDescriptorSets();
    createCommandBuffers();
    createDynamicCommandBuffers();
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
    
void VulkanApplication::createSwapchain() {
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice);
    
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);
    
    swapchain.swapchainImageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && swapchain.swapchainImageCount > swapchainSupport.capabilities.maxImageCount) {
        swapchain.swapchainImageCount = swapchainSupport.capabilities.maxImageCount;
    }

    swapchain.swapchainFBAindices.resize(swapchain.swapchainImageCount);
    swapchain.swapchainFBACopies.resize(swapchain.swapchainImageCount);
    
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    
    createInfo.minImageCount = swapchain.swapchainImageCount;
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
    
    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain.swapchain) != VK_SUCCESS) {
        assert(false && "failed to create swap chain!");
    }
    
    vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchain.swapchainImageCount, nullptr);
    swapchain.swapchainImages.resize(swapchain.swapchainImageCount);
    vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchain.swapchainImageCount, swapchain.swapchainImages.data());
    
    swapchain.swapchainImageFormat = surfaceFormat.format;
    swapchain.swapchainExtent = extent;
}

void VulkanApplication::createSwapchainImageViews() {
    swapchain.swapchainImageViews.resize(swapchain.swapchainImages.size());

    for (uint32_t i = 0; i < swapchain.swapchainImages.size(); i++) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchain.swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchain.swapchainImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        swapchain.swapchainImageViews[i] = createImageView(viewInfo);
    }
}

void VulkanApplication::createRenderPass(RenderPass & renderPass) {
    switch (renderPass.outputType) {
    case RenderPass::RENDER_OUTPUT_TYPE_SWAPCHAIN:
        renderPass.extent = swapchain.swapchainExtent;
        break;
    
    case RenderPass::RENDER_OUTPUT_TYPE_SHADOWMAP:
        renderPass.extent.width = shadowmapDimension;
        renderPass.extent.height = shadowmapDimension;
        break;
    }

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
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler.mipLodBias = 0.0f;
    sampler.maxAnisotropy = 1.0f;
    sampler.minLod = 0.0f;
    sampler.maxLod = 1.0f;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    if (vkCreateSampler(device, &sampler, nullptr, &shadowMapSampler) != VK_SUCCESS) {
        assert(false && "failed to create sampler for offscreen depth image!");
    }
}

void VulkanApplication::prepareGraphicsPipelines() {
    for (auto & pipeline : graphicsPipelines) {
        if (pipeline.type == PIPELINE_TYPE_OFFSCREEN) {
            pipeline.renderPassIndex = offscreenPassIndex;
        } else {
            pipeline.renderPassIndex = scenePassIndex;
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
    dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    
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
    rasterizer.cullMode = graphicsPipeline.cullModeFlags;
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
    depthStencil.depthCompareOp = graphicsPipeline.compareOp;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    
    prt::vector<VkPipelineColorBlendAttachmentState> & colorBlendAttachments = graphicsPipeline.colorBlendAttachments;

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
    pipelineInfo.renderPass = renderPasses[graphicsPipeline.renderPassIndex].renderPass;
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

void VulkanApplication::createSwapchainFrameBuffers() {
    swapchain.swapchainFramebuffers.resize(swapchain.swapchainImageViews.size());
    
    for (size_t i = 0; i < swapchain.swapchainImageViews.size(); i++) {
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPasses[scenePassIndex].renderPass;
        framebufferInfo.width = swapchain.swapchainExtent.width;
        framebufferInfo.height = swapchain.swapchainExtent.height;
        framebufferInfo.layers = 1;

        prt::vector<VkImageView> attachments;
        attachments.resize(swapchain.swapchainFBAindices[i].size() + 1);
        attachments[0] = swapchain.swapchainImageViews[i];
        for (size_t j = 0; j < swapchain.swapchainFBAindices[i].size(); ++j) {
            attachments[j + 1] = framebufferAttachments[swapchain.swapchainFBAindices[i][j]].imageView;
        }

        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchain.swapchainFramebuffers[i]) != VK_SUCCESS) {
            assert(false && "failed to create framebuffer!");
        }
    }
}

void VulkanApplication::createSwapchainFBACopies() {
    for (auto & copies : swapchain.swapchainFBACopies) {
        for (SwapchainFBACopy & copy : copies) {
            createBuffer(copy.bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                         copy.buffer, copy.bufferMemory);

            if (vkMapMemory(device, copy.bufferMemory, 0, VK_WHOLE_SIZE, 0, &copy.data ) != VK_SUCCESS) {
                assert(false && "failed to map memory!");
            }
        }
    }
}

void VulkanApplication::createCommandPool(VkCommandPool & pool, VkCommandPoolCreateFlags flags) {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
    
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = flags;
    
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        assert(false && "failed to create graphics command pool!");
    }
}

void VulkanApplication::createRenderPasses() {
    for (RenderPass & pass : renderPasses) {
        createRenderPass(pass);
    }
}

void VulkanApplication::createFramebufferAttachments() {
    for (FramebufferAttachment & fba : framebufferAttachments) {
        if (fba.swapchainAttachment) {
            fba.imageInfo.extent.width = swapchain.swapchainExtent.width;
            fba.imageInfo.extent.height = swapchain.swapchainExtent.height;
        }

        createImage(fba.imageInfo,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    fba.image,
                    fba.memory);
        
        fba.imageViewInfo.image = fba.image;
        fba.imageView = createImageView(fba.imageViewInfo);

        transitionImageLayout(fba.image, 
                              fba.imageInfo.format, 
                              fba.imageInfo.initialLayout, 
                              fba.imageLayout, 
                              fba.imageViewInfo.subresourceRange.levelCount, 
                              fba.imageViewInfo.subresourceRange.layerCount);
    }
}

void VulkanApplication::createShadowMaps() {
    for (CascadeShadowMap & map : shadowMaps) {
        createShadowMap(map);
    }
}

void VulkanApplication::createShadowMap(CascadeShadowMap & shadowMap) {
    for (size_t i = 0; i < shadowMap.cascades.size(); ++i) {
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
            viewInfo.image = framebufferAttachments[shadowMap.cascades[i][j].frameBufferIndex].image;
            if (vkCreateImageView(device, &viewInfo, nullptr, &shadowMap.cascades[i][j].imageView) != VK_SUCCESS) {
                assert(false && "failed to create image view for offscreen frame buffer!");
            }
            // Create frame buffer
            VkFramebufferCreateInfo fbufCreateInfo{};
            fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbufCreateInfo.renderPass = renderPasses[offscreenPassIndex].renderPass;
            fbufCreateInfo.attachmentCount = 1;
            fbufCreateInfo.pAttachments = &shadowMap.cascades[i][j].imageView;
            fbufCreateInfo.width = shadowmapDimension;
            fbufCreateInfo.height = shadowmapDimension;
            fbufCreateInfo.layers = 1;
            if (vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &shadowMap.cascades[i][j].framebuffer) != VK_SUCCESS) {
                assert(false && "failed to create offscreen frame buffer!");
            }
        }
    }
}

size_t VulkanApplication::pushBackRenderPass() {
    size_t index = renderPasses.size();
    renderPasses.push_back({});
    return index;
}

size_t VulkanApplication::pushBackFramebufferAttachment() {
    size_t index = framebufferAttachments.size();
    framebufferAttachments.push_back({});
    return index;
}

size_t VulkanApplication::addSwapchainFBA(size_t swapchainIndex, size_t fbaIndex) {
    swapchain.swapchainFBAindices[swapchainIndex].push_back(fbaIndex);
    return swapchain.swapchainFBAindices[swapchainIndex].size() - 1;
}

size_t VulkanApplication::addSwapchainFBACopy(size_t swapchainIndex, size_t swapchainFBAIndex, 
                                              uint32_t width, uint32_t height, VkImageAspectFlags aspectFlags) {
    swapchain.swapchainFBACopies[swapchainIndex].push_back({});

    SwapchainFBACopy & copy = swapchain.swapchainFBACopies[swapchainIndex].back();
    copy.swapchainAttachmentIndex = swapchainFBAIndex;

    copy.region.imageExtent.width = width;
    copy.region.imageExtent.height = height;
    copy.region.imageExtent.depth = 1;

    copy.region.imageSubresource.aspectMask = aspectFlags;
    copy.region.imageSubresource.mipLevel = 0;
    copy.region.imageSubresource.baseArrayLayer = 0;
    copy.region.imageSubresource.layerCount = 1;

    size_t pixelSize;
    switch (aspectFlags)
    {
    case VK_IMAGE_ASPECT_COLOR_BIT:
        pixelSize = 4 * 4;
        break;
        case VK_IMAGE_ASPECT_DEPTH_BIT:
        pixelSize = 4;
        break;
    
    default:
        pixelSize = 0;
        assert(false && "Error unhandled aspect flags!");
        break;
    }

    copy.bufferSize = width * height * pixelSize;

    return swapchain.swapchainFBACopies[swapchainIndex].size() - 1;
}


size_t VulkanApplication::pushBackShadowMap() {
    size_t index = shadowMaps.size();
    shadowMaps.push_back({});
    return index;
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

void VulkanApplication::createTextureImageView(VkImageView & imageView, VkImage &image, uint32_t mipLevels) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    imageView = createImageView(viewInfo);
}

void VulkanApplication::createCubeMapImageView(VkImageView& imageView, VkImage &image, uint32_t mipLevels) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    imageView = createImageView(viewInfo);
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

VkImageView VulkanApplication::createImageView(VkImageViewCreateInfo & viewInfo) {    
    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        assert(false && "failed to create texture image view!");
    }
    
    return imageView;
}

void VulkanApplication::createImage(VkImageCreateInfo & imageInfo, VkMemoryPropertyFlags properties, 
                                    VkImage & image, VkDeviceMemory & imageMemory) {
    
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

// TODO: remove/change this function and shift responsibility of transitioning to
//       caller 
void VulkanApplication::transitionImageLayout(VkImage image, VkFormat format, 
                                              VkImageLayout oldLayout, VkImageLayout newLayout, 
                                              uint32_t mipLevels, uint32_t layerCount) {

    if (oldLayout == newLayout) return;
    
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
        oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
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
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
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
    poolInfo.maxSets = static_cast<uint32_t>(swapchain.swapchainImages.size());
    
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &pipeline.descriptorPool) != VK_SUCCESS) {
        assert(false && "failed to create descriptor pool!");
    }
}

void VulkanApplication::createDescriptorSets() {
    for (auto & pipeline : graphicsPipelines) {
        prt::vector<VkDescriptorSetLayout> layout(swapchain.swapchainImages.size(), pipeline.descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pipeline.descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchain.swapchainImages.size());
        allocInfo.pSetLayouts = layout.data();

        pipeline.descriptorSets.resize(swapchain.swapchainImages.size());
        if (vkAllocateDescriptorSets(device, &allocInfo, pipeline.descriptorSets.data()) != VK_SUCCESS) {
            assert(false && "failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < swapchain.swapchainImages.size(); i++) {     
            prt::vector<VkDescriptorImageInfo> imDesc;
            imDesc.resize(pipeline.imageAttachments.size());
            for (size_t j = 0; j < pipeline.imageAttachments.size(); ++j) {
                ImageAttachment const & attach = pipeline.imageAttachments[j];
                imDesc[j].sampler = attach.sampler;
                imDesc[j].imageView = framebufferAttachments[attach.FBAIndices[i]].imageView;
                imDesc[j].imageLayout = attach.layout;

                pipeline.descriptorWrites[i][attach.descriptorIndex].pImageInfo = &imDesc[j];
            }

            prt::vector<VkDescriptorImageInfo> uboDesc;
            uboDesc.resize(pipeline.uboAttachments.size());
            for (size_t j = 0; j < pipeline.uboAttachments.size(); ++j) {
                pipeline.descriptorWrites[i][0].pBufferInfo = &pipeline.uboAttachments[j].descriptorBufferInfos[i];     
            }

            for (auto & descriptorWrite : pipeline.descriptorWrites[i]) {
                descriptorWrite.dstSet = pipeline.descriptorSets[i];
            }

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(pipeline.descriptorWrites[i].size()), 
                                   pipeline.descriptorWrites[i].data(), 0, nullptr);       
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
    uniformBufferData.mappedMemories.resize(swapchain.swapchainImages.size());
    uniformBufferData.uniformBuffers.resize(swapchain.swapchainImages.size());
    uniformBufferData.uniformBufferMemories.resize(swapchain.swapchainImages.size());

    size_t size = uniformBufferData.uboData.size();
    for (size_t i = 0; i < swapchain.swapchainImages.size(); i++) {
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
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    assert(false && "failed to find suitable memory type!");
    return 0;
}

void VulkanApplication::createCommandBuffers() {
    commandBuffers.resize(swapchain.swapchainFramebuffers.size());
    
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

void VulkanApplication::createDynamicCommandBuffers() {
    dynamicCommandBuffers.resize(swapchain.swapchainFramebuffers.size());
    
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = dynamicCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, dynamicCommandBuffers.data()) != VK_SUCCESS) {
        assert(false && "failed to allocate dynamic command buffers!");
    }
}

void VulkanApplication::createCommandBuffer(size_t const imageIndex) {    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    
    if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        assert(false && "failed to begin recording command buffer!");
    }
    
    for (RenderPass & renderPass : renderPasses) {
        createRenderPassCommands(imageIndex, renderPass);
    }

    // copy FBAs
    for (SwapchainFBACopy & copy : swapchain.swapchainFBACopies[imageIndex]) {
        size_t fbaIndex = swapchain.swapchainFBAindices[imageIndex][copy.swapchainAttachmentIndex];
        
        // Transition image
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = framebufferAttachments[fbaIndex].imageLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = framebufferAttachments[fbaIndex].image;
    
        if (barrier.oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
            barrier.newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            
            if (hasStencilComponent(framebufferAttachments[fbaIndex].imageInfo.format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
    
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        
        vkCmdPipelineBarrier(commandBuffers[imageIndex],
                             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
    }

    if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
        assert(false && "failed to record command buffer!");
    }
}

void VulkanApplication::updateDynamicCommandBuffer(size_t const imageIndex) {    
    vkResetCommandBuffer(dynamicCommandBuffers[imageIndex], 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    if (vkBeginCommandBuffer(dynamicCommandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        assert(false && "failed to begin recording command buffer!");
    }

    // copy FBAs
    for (SwapchainFBACopy & copy : swapchain.swapchainFBACopies[imageIndex]) {
        size_t fbaIndex = swapchain.swapchainFBAindices[imageIndex][copy.swapchainAttachmentIndex];

        vkCmdCopyImageToBuffer(dynamicCommandBuffers[imageIndex],
                               framebufferAttachments[fbaIndex].image,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               copy.buffer,
                               1,
                               &copy.region);
    }

    if (vkEndCommandBuffer(dynamicCommandBuffers[imageIndex]) != VK_SUCCESS) {
        assert(false && "failed to record command buffer!");
    }
}

void VulkanApplication::createRenderPassCommands(size_t const imageIndex, RenderPass & renderPass) {
    switch (renderPass.outputType) {
        case RenderPass::RENDER_OUTPUT_TYPE_SWAPCHAIN: {
            renderPass.framebuffers.resize(swapchain.swapchainImageCount);
            for (size_t i = 0; i < renderPass.framebuffers.size(); ++i) {
                renderPass.framebuffers[i].resize(1);
                renderPass.framebuffers[i][0] = swapchain.swapchainFramebuffers[i];
            }
        }
        break;
        case RenderPass::RENDER_OUTPUT_TYPE_SHADOWMAP: {
            renderPass.framebuffers.resize(swapchain.swapchainImageCount);
            for (size_t i = 0; i < swapchain.swapchainImageCount; ++i) {
                renderPass.framebuffers[i].resize(NUMBER_SHADOWMAP_CASCADES);
                for (size_t j = 0; j < NUMBER_SHADOWMAP_CASCADES; ++j) {
                    renderPass.framebuffers[i][j] = shadowMaps[renderPass.shadowMapIndex].cascades[i][j].framebuffer;
                }
            }
        }
        break;
    }

    for (size_t i = 0; i < renderPass.framebuffers[imageIndex].size(); ++i) {
        VkFramebuffer & framebuffer = renderPass.framebuffers[imageIndex][i];

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass.renderPass;
        renderPassBeginInfo.framebuffer = framebuffer;
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = renderPass.extent;
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(renderPass.clearValues.size());
        renderPassBeginInfo.pClearValues = renderPass.clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport;
        viewport.width = renderPass.extent.width;
        viewport.height = renderPass.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
    
        VkRect2D scissor;
        scissor.extent = renderPass.extent;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

        // Render pass is divided into sub passes
        assert(!renderPass.subpasses.empty() && "Subpasses can not be empty!");
        for (size_t j = 0; j < renderPass.subpasses.size(); ++j) {
            SubPass & sub = renderPass.subpasses[j];
            if (j  > 0) {
                vkCmdNextSubpass(commandBuffers[imageIndex], VK_SUBPASS_CONTENTS_INLINE);
            }

            for (size_t pipeIndex : sub.pipelineIndices) {
                GraphicsPipeline & pipeline = graphicsPipelines[pipeIndex];
                if (!checkMask(commandBufferRenderGroupMask, pipeline.renderGroup)) continue;

                if (renderPass.pushConstantFBIndexByteOffset != -1) {
                    for (auto & drawCall : pipeline.drawCalls) {
                        *reinterpret_cast<int32_t*>(&drawCall.pushConstants[renderPass.pushConstantFBIndexByteOffset]) = i;
                    }
                }
                
                createDrawCommands(imageIndex, pipeline);
            }
        }
        
        vkCmdEndRenderPass(commandBuffers[imageIndex]);
    }
}

void VulkanApplication::createDrawCommands(size_t const imageIndex, GraphicsPipeline & pipeline) {
    static constexpr VkDeviceSize offset = 0;
    
    vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    vkCmdSetDepthBias(commandBuffers[imageIndex],
                      pipeline.depthBiasConstant,
                      pipeline.depthBiasClamp,
                      pipeline.depthBiasSlope);

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
    imagesInFlight.resize(swapchain.swapchainImages.size(), VK_NULL_HANDLE);
    
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
    VkResult result = vkAcquireNextImageKHR(device, swapchain.swapchain, UINT64_MAX, 
                                            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        assert(false && "failed to acquire swap chain image!");
    }
    
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }     
    updateUniformBuffers(imageIndex);

    updateDynamicCommandBuffer(imageIndex);

    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    prt::array<VkCommandBuffer, 2> cmdBuffers = { commandBuffers[imageIndex], dynamicCommandBuffers[imageIndex]};
    
    submitInfo.commandBufferCount = cmdBuffers.size();
    submitInfo.pCommandBuffers = cmdBuffers.data();
    
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
    
    VkSwapchainKHR swapchains[] = {swapchain.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    
    presentInfo.pImageIndices = &imageIndex;
    
    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        assert(false && "failed to present swap chain image!");
    }
    
    currentFrame = (currentFrame + 1) % maxFramesInFlight;

    swapchain.previousImageIndex = imageIndex;
}

prt::vector<size_t> VulkanApplication::getPipelineIndicesByType(PipelineType type) {
    prt::vector<size_t> indices;
    for (size_t i = 0; i < graphicsPipelines.size(); ++i) {
        if (graphicsPipelines[i].type == type) indices.push_back(i);
    }
    return indices;
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

SwapchainSupportDetails VulkanApplication::querySwapchainSupport(VkPhysicalDevice device) {
    SwapchainSupportDetails details;
    
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
    
    bool swapchainAdequate = false;
    if (extensionsSupported) {
        SwapchainSupportDetails swapchainSupport = querySwapchainSupport(device);
        swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
    }
    
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    
    return indices.isComplete() && extensionsSupported && swapchainAdequate && supportedFeatures.samplerAnisotropy && supportedFeatures.independentBlend;
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

float VulkanApplication::depthToFloat(void * depthValue, VkFormat depthFormat) {    
    switch (depthFormat) {
    case VK_FORMAT_D32_SFLOAT:
        return *static_cast<float*>(depthValue);
        break;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        break;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        break;
    
    default:
        break;
    }
    assert(false && "Conversion for this depth format has not been implemented!");
    return 0.0f;
}
