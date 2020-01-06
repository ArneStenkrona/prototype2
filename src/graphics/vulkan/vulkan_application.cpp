#include "vulkan_application.h"

#include "src/config/prototype2Config.h"

#include "src/container/array.h"
#include "src/container/hash_set.h"

const int WIDTH = 800;
const int HEIGHT = 600;

constexpr unsigned int MAX_FRAMES_IN_FLIGHT = 2;

const prt::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const prt::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

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

VulkanApplication::VulkanApplication(Input& /*input*/)
    /*: _imGuiApplication(physicalDevice, device, input)*/ {
    initWindow();
    initVulkan();
}   

void VulkanApplication::initWindow() {
    glfwInit();
 
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
 
    _window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
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
    createRenderPass();
    createDescriptorSetLayouts();
    createPipelineCaches();
    createGraphicsPipelines();
    createCommandPool();

    // _imGuiApplication.init(float(WIDTH), float(HEIGHT));
    // _imGuiApplication.initResources(renderPass, commandPool, 
    //                                 graphicsQueue, msaaSamples);

    createColorResources();
    createDepthResources();
    createFramebuffers();
    createSamplers();
    createUniformBuffers();
    createDescriptorPools();
    createSyncObjects();
}
    
void VulkanApplication::update(const prt::vector<glm::mat4>& modelMatrices, 
                               const glm::mat4& viewMatrix, 
                               const glm::mat4& projectionMatrix, 
                               const glm::vec3 viewPosition,
                               const glm::mat4& skyProjectionMatrix,
                               float /*deltaTime*/) {
        glfwPollEvents();
        
        //_imGuiApplication.updateInput(float(width), float(height), deltaTime);
        drawFrame(modelMatrices, viewMatrix, projectionMatrix, viewPosition,
                  skyProjectionMatrix);    
}
    
void VulkanApplication::cleanupSwapChain() {
    vkDeviceWaitIdle(device);
    
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
 
    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);
 
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
 
    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
 
    vkDestroyPipeline(device, pipelines.skybox, nullptr);
    vkDestroyPipeline(device, pipelines.model, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayouts.skybox, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayouts.model, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
 
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
 
    vkDestroySwapchainKHR(device, swapChain, nullptr);
 
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers.skybox[i], nullptr);
        vkDestroyBuffer(device, uniformBuffers.model[i], nullptr);
        vkFreeMemory(device, uniformBuffers.skyboxMemory[i], nullptr);
        vkFreeMemory(device, uniformBuffers.modelMemory[i], nullptr);
    }
 
    vkDestroyDescriptorPool(device, descriptorPools.skybox, nullptr);
    vkDestroyDescriptorPool(device, descriptorPools.model, nullptr);
}
    
void VulkanApplication::cleanup() {
    cleanupSwapChain();
    //_imGuiApplication.cleanup();
    for (size_t i = 0; i < NUMBER_SUPPORTED_TEXTURES; i++) {
        vkDestroyImageView(device, textureImageView[i], nullptr);
        vkDestroyImage(device, textureImage[i], nullptr);
        vkFreeMemory(device, textureImageMemory[i], nullptr);
    }
    vkDestroyImageView(device, cubeMapImageView, nullptr);
    vkDestroyImage(device, cubeMapImage, nullptr);
    vkFreeMemory(device, cubeMapImageMemory, nullptr);

    vkDestroySampler(device, samplers.skybox, nullptr);
    vkDestroySampler(device, samplers.model, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.skybox, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.model, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyBuffer(device, skyboxVertexBuffer, nullptr);
    vkFreeMemory(device, skyboxVertexBufferMemory, nullptr);

    vkDestroyBuffer(device, skyboxIndexBuffer, nullptr);
    vkFreeMemory(device, skyboxIndexBufferMemory, nullptr);

    vkDestroyBuffer(device, indirectCommandBuffer, nullptr);
    vkFreeMemory(device, indirectCommandBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyPipelineCache(device, pipelineCaches.skybox, nullptr);
    vkDestroyPipelineCache(device, pipelineCaches.model, nullptr);
 
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
    width = 0;
    height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);
 
    //_imGuiApplication.cleanupSwapchain();

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createPipelineCaches();
    createGraphicsPipelines();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPools();
    createDescriptorSets();

    // _imGuiApplication.init(float(width), float(height));
    // _imGuiApplication.initResources(renderPass, commandPool,
    //                                 graphicsQueue, msaaSamples);

    createCommandBuffers();
}

void VulkanApplication::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        assert(false && "validation layers requested, but not available!");
    }
 
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
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
            msaaSamples = getMaxUsableSampleCount();
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

void VulkanApplication::createRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentDescription colorAttachmentResolve = {};
    colorAttachmentResolve.format = swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference colorAttachmentResolveRef = {};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;
    
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    prt::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve };
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        assert(false && "failed to create render pass!");
    }
}

void VulkanApplication::createDescriptorSetLayouts() {
    // skybox
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding cubeMapSamplerLayoutBinding = {};
    cubeMapSamplerLayoutBinding.descriptorCount = 1;
    cubeMapSamplerLayoutBinding.binding = 1;
    cubeMapSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    cubeMapSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    cubeMapSamplerLayoutBinding.pImmutableSamplers = nullptr;

    prt::array<VkDescriptorSetLayoutBinding, 2> skyboxBindings = {uboLayoutBinding, 
                                                                  cubeMapSamplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(skyboxBindings.size());
    layoutInfo.pBindings = skyboxBindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayouts.skybox) != VK_SUCCESS) {
        assert(false && "failed to create descriptor set layout!");
    }

    // model
    VkDescriptorSetLayoutBinding textureLayoutBinding = {};
    textureLayoutBinding.descriptorCount = NUMBER_SUPPORTED_TEXTURES;
    textureLayoutBinding.binding = 1;
    textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;;
    textureLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding modelSamplerLayoutBinding = {};
    modelSamplerLayoutBinding.descriptorCount = 1;
    modelSamplerLayoutBinding.binding = 2;
    modelSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    modelSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;;
    modelSamplerLayoutBinding.pImmutableSamplers = nullptr;

    prt::array<VkDescriptorSetLayoutBinding, 3> modelBindings = {uboLayoutBinding, 
                                                                 textureLayoutBinding, 
                                                                 modelSamplerLayoutBinding};
    layoutInfo.bindingCount = static_cast<uint32_t>(modelBindings.size());
    layoutInfo.pBindings = modelBindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayouts.model) != VK_SUCCESS) {
        assert(false && "failed to create descriptor set layout!");
    }
}

void VulkanApplication::createPipelineCaches()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	if(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCaches.skybox) != VK_SUCCESS) {
        assert(false && "failed to create pipeline cache");
    }
    if(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCaches.model) != VK_SUCCESS) {
        assert(false && "failed to create pipeline cache");
    }
}

void VulkanApplication::createGraphicsPipeline(VkVertexInputBindingDescription& bindingDescription,
                                               prt::vector<VkVertexInputAttributeDescription>& attributeDescription,
                                               VkDescriptorSetLayout& descriptorSetLayout,
                                               const std::string& vertShader, const std::string& fragShader,
                                               VkPipeline& pipeline, VkPipelineCache& pipelineCache,
                                               VkPipelineLayout& pipelineLayout) {
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescription.size();
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    
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
    rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = msaaSamples;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    VkPushConstantRange pushConstantRange = {};
	pushConstantRange.offset = 0;
	pushConstantRange.size = pushConstants.data_size();
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
    
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        assert(false && "failed to create pipeline layout!");
    }
    prt::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { VkPipelineShaderStageCreateInfo{},
                                                                    VkPipelineShaderStageCreateInfo{} };
    
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    VkShaderModule vertShaderModule = createShaderModule(vertShader.c_str());
    VkShaderModule fragShaderModule = createShaderModule(fragShader.c_str());

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main";
    
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main";

    
    if (vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        assert(false && "failed to create graphics pipeline!");
    }
    
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void VulkanApplication::createGraphicsPipelines() {
    // skybox    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkVertexInputBindingDescription skyboxBindingDescription{};
    skyboxBindingDescription.binding = 0;
    skyboxBindingDescription.stride = sizeof(glm::vec3);
    skyboxBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    prt::vector<VkVertexInputAttributeDescription> skyboxAttributeDescriptions{};
    skyboxAttributeDescriptions.resize(1);
    skyboxAttributeDescriptions[0].binding = 0;
    skyboxAttributeDescriptions[0].location = 0;
    skyboxAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    skyboxAttributeDescriptions[0].offset = 0;
    
    std::string skyboxVertPath = RESOURCE_PATH + std::string("shaders/skybox.vert.spv");
    std::string skyboxFragPath = RESOURCE_PATH + std::string("shaders/skybox.frag.spv");
    createGraphicsPipeline(skyboxBindingDescription, skyboxAttributeDescriptions,
                           descriptorSetLayouts.skybox,
                           skyboxVertPath, skyboxFragPath,
                           pipelines.skybox, pipelineCaches.skybox,
                           pipelineLayouts.skybox);
    

    // model
    
    auto modelBindingDescription = Vertex::getBindingDescription();
    auto attrib = Vertex::getAttributeDescriptions();
    prt::vector<VkVertexInputAttributeDescription> modelAttributeDescriptions = { attrib[0],
                                                                                  attrib[1],
                                                                                  attrib[2] };
    
    std::string modelVertPath = RESOURCE_PATH + std::string("shaders/model.vert.spv");
    std::string modelFragPath = RESOURCE_PATH + std::string("shaders/model.frag.spv");
    createGraphicsPipeline(modelBindingDescription, modelAttributeDescriptions,
                           descriptorSetLayouts.model,
                           modelVertPath, modelFragPath,
                           pipelines.model, pipelineCaches.model,
                           pipelineLayouts.model);
}

void VulkanApplication::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());
    
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            colorImageView,
            depthImageView,
            swapChainImageViews[i]
        };
        
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;
        
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
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        assert(false && "failed to create graphics command pool!");
    }
}

void VulkanApplication::createColorResources() {
    VkFormat colorFormat = swapChainImageFormat;
    
    createImage(swapChainExtent.width, swapChainExtent.height, 1, 1, 0, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
    colorImageView = createImageView(colorImage, colorFormat, 
                                     VK_IMAGE_ASPECT_COLOR_BIT, 
                                     VK_IMAGE_VIEW_TYPE_2D,
                                     1, 1);
    
    transitionImageLayout(colorImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 1);
}

void VulkanApplication::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    
    createImage(swapChainExtent.width, swapChainExtent.height, 1, 1, 0, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, 
                                     VK_IMAGE_ASPECT_DEPTH_BIT, 
                                     VK_IMAGE_VIEW_TYPE_2D,
                                     1, 1);
    
    transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);
}

VkFormat VulkanApplication::findSupportedFormat(const prt::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
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
}

VkFormat VulkanApplication::findDepthFormat() {
    return findSupportedFormat(
                               {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
                               );
}

bool VulkanApplication::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void VulkanApplication::createTextureImage(VkImage& texImage, VkDeviceMemory& texImageMemory, 
                                           const Texture& texture) {
    VkDeviceSize imageSize = texture.texWidth * texture.texHeight * 4;
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texture.texWidth, texture.texHeight)))) + 1;

    auto pixels = texture.pixelBuffer.data();
 
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 stagingBuffer, stagingBufferMemory);
 
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    createImage(texture.texWidth, texture.texHeight, 
                mipLevels, 1, 0,
                VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, 
                VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texImage, texImageMemory);
 
    transitionImageLayout(texImage, VK_FORMAT_R8G8B8A8_UNORM, 
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                          mipLevels, 1);

    copyBufferToImage(stagingBuffer, texImage, 
                      static_cast<uint32_t>(texture.texWidth), 
                      static_cast<uint32_t>(texture.texHeight),
                      1);

    //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
 
    generateMipmaps(texImage, VK_FORMAT_R8G8B8A8_UNORM, 
                    texture.texWidth, texture.texHeight, 
                    mipLevels, 1);
}

void VulkanApplication::createCubeMapImage(VkImage& texImage, VkDeviceMemory& texImageMemory, 
                                           const prt::array<Texture, 6>& textures) {
    VkDeviceSize layerSize = textures[0].texWidth * textures[0].texHeight * 4;
    VkDeviceSize imageSize = layerSize * textures.size();
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textures[0].texWidth, textures[0].texHeight)))) + 1;
 
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

    createImage(textures[0].texWidth, textures[0].texHeight, 
                mipLevels, textures.size(), 
                VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, 
                VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texImage, texImageMemory);
 
    transitionImageLayout(texImage, VK_FORMAT_R8G8B8A8_UNORM, 
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                          mipLevels, textures.size());

    copyBufferToImage(stagingBuffer, texImage, 
                      static_cast<uint32_t>(textures[0].texWidth), 
                      static_cast<uint32_t>(textures[0].texHeight),
                      textures.size());

    //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
 
    generateMipmaps(texImage, VK_FORMAT_R8G8B8A8_UNORM, 
                    textures[0].texWidth, textures[0].texHeight, 
                    mipLevels, textures.size());
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

void VulkanApplication::createTextureImageView(VkImageView& imageView, VkImage &image) {
    imageView = createImageView(image, VK_FORMAT_R8G8B8A8_UNORM, 
                                VK_IMAGE_ASPECT_COLOR_BIT, 
                                VK_IMAGE_VIEW_TYPE_2D,
                                mipLevels, 1);
}

void VulkanApplication::createCubeMapImageView(VkImageView& imageView, VkImage &image) {
    imageView = createImageView(image, VK_FORMAT_R8G8B8A8_UNORM, 
                                VK_IMAGE_ASPECT_COLOR_BIT, 
                                VK_IMAGE_VIEW_TYPE_CUBE,
                                mipLevels, 6);
}

void VulkanApplication::createSamplers() {
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.anisotropyEnable = VK_FALSE;
	samplerCreateInfo.maxAnisotropy = 0.0f;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = static_cast<float>(mipLevels);
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerCreateInfo, 0, &samplers.skybox) != VK_SUCCESS) {
        assert(false && "failed to create sampler!");
    }
    if (vkCreateSampler(device, &samplerCreateInfo, 0, &samplers.model) != VK_SUCCESS) {
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

void VulkanApplication::createImage(uint32_t width, uint32_t height, 
                                    uint32_t mipLevels, uint32_t arrayLayers,
                                    VkImageCreateFlags flags,
                                    VkSampleCountFlagBits numSamples, 
                                    VkFormat format, VkImageTiling tiling, 
                                    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
                                    VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = flags;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
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
    
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    
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
    
    vkCmdPipelineBarrier(
                         commandBuffer,
                         sourceStage, destinationStage,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier
                         );
    
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

void VulkanApplication::bindScene(const prt::vector<Model>& models, const prt::vector<uint32_t>& modelIndices,
                                  const prt::array<Texture, 6>& skybox) {
    loadModels(models);
    loadSkybox(skybox);
    createRenderJobs(models, modelIndices);
    recreateSwapChain();
}

void VulkanApplication::loadModels(const prt::vector<Model>& models) {
    assert(!models.empty());
    for (size_t i = 0; i < pushConstants.size(); i++) {
        pushConstants[i] = i;
    }
    createVertexBuffer(models);
    createIndexBuffer(models);

    createSkyboxBuffers();

    size_t numTex = 0;
    for (size_t i = 0; i < models.size(); i++) {
        for (size_t j = 0; j < models[i]._meshes.size(); j++) {
            createTextureImage(textureImage[numTex], textureImageMemory[numTex], models[i]._meshes[j]._texture);
            createTextureImageView(textureImageView[numTex], textureImage[numTex]);
            numTex++;
        }
    }

    for (size_t i = numTex; i < NUMBER_SUPPORTED_TEXTURES; i++) {
        createTextureImage(textureImage[i], textureImageMemory[i], models[0]._meshes.back()._texture);
        createTextureImageView(textureImageView[i], textureImage[0]);
    }
}

void VulkanApplication::loadSkybox(const prt::array<Texture, 6>& skybox) {    
    createCubeMapImage(cubeMapImage, cubeMapImageMemory, skybox);
    createCubeMapImageView(cubeMapImageView, cubeMapImage);
}

void VulkanApplication::createRenderJobs(const prt::vector<Model>& models, 
                                         const prt::vector<uint32_t>& modelIndices) {
    prt::vector<uint32_t> imgIdxOffsets = { 0 };
    prt::vector<uint32_t> indexOffsets = { 0 };
    imgIdxOffsets.resize(models.size());
    indexOffsets.resize(models.size());
    for (size_t i = 1; i < models.size(); i++) {
        imgIdxOffsets[i] = imgIdxOffsets[i-1] + models[i-1]._meshes.size();
        indexOffsets[i] = indexOffsets[i-1] + models[i-1]._indexBuffer.size();
    }

    for (size_t i = 0; i < modelIndices.size(); i++) {
        const Model& model = models[modelIndices[i]];
        for (size_t j = 0; j < model._meshes.size(); j++) {
            RenderJob renderJob;
            renderJob._modelMatrixIdx = i;
            renderJob._imgIdx = imgIdxOffsets[modelIndices[i]] + j;
            renderJob._firstIndex = indexOffsets[modelIndices[i]] + model._meshes[j].startIndex;
            renderJob._indexCount = model._meshes[j].numIndices;
            _renderJobs.push_back(renderJob);
        }
    }
}


void VulkanApplication::createVertexBuffer(const prt::vector<Model>& models) {
    prt::vector<Vertex> allVertices;
    for (size_t i = 0; i < models.size(); i++) {
        for (size_t j = 0; j < models[i]._vertexBuffer.size(); j++) {
            allVertices.push_back(models[i]._vertexBuffer[j]);
        }
    }
    createAndMapBuffer(allVertices.data(), sizeof(Vertex) * allVertices.size(),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       vertexBuffer, vertexBufferMemory);    
}

void VulkanApplication::createIndexBuffer(const prt::vector<Model>& models) {
    prt::vector<uint32_t> allIndices;
    size_t vertexOffset = 0;
    for (size_t i = 0; i < models.size(); i++) {
        for (size_t j = 0; j < models[i]._indexBuffer.size(); j++) {
            allIndices.push_back(models[i]._indexBuffer[j] + vertexOffset);
        }
        vertexOffset += models[i]._vertexBuffer.size();
    }

    createAndMapBuffer(allIndices.data(), sizeof(uint32_t) * allIndices.size(),
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       indexBuffer, indexBufferMemory);
}

void VulkanApplication::createSkyboxBuffers() {
    prt::vector<glm::vec3> vertices;

    vertices.resize(8);
    float w = 500.0f;
    vertices[7] = glm::vec3{-w, -w, -w};
    vertices[6] = glm::vec3{ w, -w, -w};
    vertices[5] = glm::vec3{-w,  w, -w};
    vertices[4] = glm::vec3{ w,  w, -w};

    vertices[3] = glm::vec3{-w, -w,  w};
    vertices[2] = glm::vec3{ w, -w,  w};
    vertices[1] = glm::vec3{-w,  w,  w};
    vertices[0] = glm::vec3{ w,  w,  w};

    createAndMapBuffer(vertices.data(), sizeof(glm::vec3) * vertices.size(),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       skyboxVertexBuffer, skyboxVertexBufferMemory);    

    prt::vector<uint32_t> indices = { 0, 2, 3,
                                      3, 1, 0,
                                      4, 5, 7,
                                      7, 6, 4,
                                      0, 1, 5,
                                      5, 4, 0,
                                      1, 3, 7,
                                      7, 5, 1,
                                      3, 2, 6,
                                      6, 7, 3,
                                      2, 0, 4,
                                      4, 6, 2 };
    
    createAndMapBuffer(indices.data(), sizeof(uint32_t) * indices.size(),
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    skyboxIndexBuffer, skyboxIndexBufferMemory);   
}

void VulkanApplication::createDrawCommands(size_t imageIndex) {

    VkBuffer vertexBuffers[] = { skyboxVertexBuffer, vertexBuffer};
    VkDeviceSize offsets[] = { 0, 0 };
    // skybox
    vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skybox);
    vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, &vertexBuffers[0], &offsets[0]);
    vkCmdBindIndexBuffer(commandBuffers[imageIndex], skyboxIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            pipelineLayouts.skybox, 0, 1, &descriptorSets.skybox[imageIndex], 0, nullptr);
    vkCmdDrawIndexed(commandBuffers[imageIndex],
                     36, 1, 0, 0, 0);
    // model
    vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.model);
    vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, &vertexBuffers[1], &offsets[1]);
    vkCmdBindIndexBuffer(commandBuffers[imageIndex], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            pipelineLayouts.model, 0, 1, &descriptorSets.model[imageIndex], 0, nullptr);

    for (size_t i = 0; i < _renderJobs.size(); i++) {
        vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayouts.model, 
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                            0, sizeof(uint32_t), (void *)&_renderJobs[i]._modelMatrixIdx );
        vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayouts.model, 
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                            sizeof(uint32_t)/*offset*/, sizeof(uint32_t), (void *)&_renderJobs[i]._imgIdx );

        vkCmdDrawIndexed(commandBuffers[imageIndex], 
                 _renderJobs[i]._indexCount,
                 1,
                 _renderJobs[i]._firstIndex,
                 0,
                 i);
    }
}

void VulkanApplication::createUniformBuffers() {
    VkDeviceSize skyboxBufferSize = sizeof(SkyboxUBO);
    VkDeviceSize modelBufferSize = sizeof(ModelUBO);
    
    uniformBuffers.skybox.resize(swapChainImages.size());
    uniformBuffers.skyboxMemory.resize(swapChainImages.size());

    uniformBuffers.model.resize(swapChainImages.size());
    uniformBuffers.modelMemory.resize(swapChainImages.size());
    
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        createBuffer(skyboxBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers.skybox[i], uniformBuffers.skyboxMemory[i]);
        createBuffer(modelBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers.model[i], uniformBuffers.modelMemory[i]);
    }
}

void VulkanApplication::createDescriptorPools() {
    // skybox
    prt::array<VkDescriptorPoolSize, 2> skyboxPoolSizes = {};
    skyboxPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    skyboxPoolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    skyboxPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    skyboxPoolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    VkDescriptorPoolCreateInfo skyboxPoolInfo = {};
    skyboxPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    skyboxPoolInfo.poolSizeCount = static_cast<uint32_t>(skyboxPoolSizes.size());
    skyboxPoolInfo.pPoolSizes = skyboxPoolSizes.data();
    skyboxPoolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());
    
    if (vkCreateDescriptorPool(device, &skyboxPoolInfo, nullptr, &descriptorPools.skybox) != VK_SUCCESS) {
        assert(false && "failed to create descriptor pool!");
    }
    // model
    prt::array<VkDescriptorPoolSize, 3> modelPoolSizes = {};
    modelPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    modelPoolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    modelPoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    modelPoolSizes[1].descriptorCount = static_cast<uint32_t>(NUMBER_SUPPORTED_TEXTURES * swapChainImages.size());
    modelPoolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    modelPoolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    VkDescriptorPoolCreateInfo modelPoolInfo = {};
    modelPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    modelPoolInfo.poolSizeCount = static_cast<uint32_t>(modelPoolSizes.size());
    modelPoolInfo.pPoolSizes = modelPoolSizes.data();
    modelPoolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());
    
    if (vkCreateDescriptorPool(device, &modelPoolInfo, nullptr, &descriptorPools.model) != VK_SUCCESS) {
        assert(false && "failed to create descriptor pool!");
    }
}

void VulkanApplication::createDescriptorSets() {
    
    prt::vector<VkDescriptorSetLayout> skyboxLayouts(swapChainImages.size(), descriptorSetLayouts.skybox);
    prt::vector<VkDescriptorSetLayout> modelLayouts(swapChainImages.size(), descriptorSetLayouts.model);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPools.skybox;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
    allocInfo.pSetLayouts = skyboxLayouts.data();
    
    descriptorSets.skybox.resize(swapChainImages.size());
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.skybox.data()) != VK_SUCCESS) {
        assert(false && "failed to allocate descriptor sets!");
    }
    descriptorSets.model.resize(swapChainImages.size());
    allocInfo.descriptorPool = descriptorPools.model;
    allocInfo.pSetLayouts = modelLayouts.data();
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.model.data()) != VK_SUCCESS) {
        assert(false && "failed to allocate descriptor sets!");
    }
    
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        // skybox
        VkDescriptorBufferInfo skyboxBufferInfo = {};
        skyboxBufferInfo.buffer = uniformBuffers.skybox[i];
        skyboxBufferInfo.offset = 0;
        skyboxBufferInfo.range = sizeof(SkyboxUBO);

        VkDescriptorImageInfo imageInfo;
        imageInfo.sampler = samplers.skybox;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = cubeMapImageView;

        prt::array<VkWriteDescriptorSet, 2> skyboxDescriptorWrites = { VkWriteDescriptorSet{}, 
                                                                       VkWriteDescriptorSet{} };
        
        skyboxDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        skyboxDescriptorWrites[0].dstSet = descriptorSets.skybox[i];
        skyboxDescriptorWrites[0].dstBinding = 0;
        skyboxDescriptorWrites[0].dstArrayElement = 0;
        skyboxDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        skyboxDescriptorWrites[0].descriptorCount = 1;
        skyboxDescriptorWrites[0].pBufferInfo = &skyboxBufferInfo;

        skyboxDescriptorWrites[1] = {};
        skyboxDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        skyboxDescriptorWrites[1].dstBinding = 1;
        skyboxDescriptorWrites[1].dstArrayElement = 0;
        skyboxDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skyboxDescriptorWrites[1].descriptorCount = 1;
        skyboxDescriptorWrites[1].dstSet = descriptorSets.skybox[i];
        skyboxDescriptorWrites[1].pBufferInfo = 0;
        skyboxDescriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(skyboxDescriptorWrites.size()), 
                               skyboxDescriptorWrites.data(), 0, nullptr);

        // model
        VkDescriptorBufferInfo modelBufferInfo = {};
        modelBufferInfo.buffer = uniformBuffers.model[i];
        modelBufferInfo.offset = 0;
        modelBufferInfo.range = sizeof(ModelUBO);
        
        prt::array<VkDescriptorImageInfo, NUMBER_SUPPORTED_TEXTURES> imageInfos;
        for (size_t j = 0; j < NUMBER_SUPPORTED_TEXTURES; j++) {
            imageInfos[j].sampler = samplers.model;
            imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[j].imageView = textureImageView[j];
        }
        
        prt::array<VkWriteDescriptorSet, 3> modelDescriptorWrites = { VkWriteDescriptorSet{}, 
                                                                      VkWriteDescriptorSet{}, 
                                                                      VkWriteDescriptorSet{} };
        
        modelDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        modelDescriptorWrites[0].dstSet = descriptorSets.model[i];
        modelDescriptorWrites[0].dstBinding = 0;
        modelDescriptorWrites[0].dstArrayElement = 0;
        modelDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        modelDescriptorWrites[0].descriptorCount = 1;
        modelDescriptorWrites[0].pBufferInfo = &modelBufferInfo;

        modelDescriptorWrites[1] = {};
        modelDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        modelDescriptorWrites[1].dstBinding = 1;
        modelDescriptorWrites[1].dstArrayElement = 0;
        modelDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        modelDescriptorWrites[1].descriptorCount = NUMBER_SUPPORTED_TEXTURES;
        modelDescriptorWrites[1].dstSet = descriptorSets.model[i];
        modelDescriptorWrites[1].pBufferInfo = 0;
        modelDescriptorWrites[1].pImageInfo = imageInfos.data();

        VkDescriptorImageInfo samplerInfo = {};
	    samplerInfo.sampler = samplers.model;

        modelDescriptorWrites[2] = {};
        modelDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        modelDescriptorWrites[2].dstBinding = 2;
        modelDescriptorWrites[2].dstArrayElement = 0;
        modelDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        modelDescriptorWrites[2].descriptorCount = 1;
        modelDescriptorWrites[2].dstSet = descriptorSets.model[i];
        modelDescriptorWrites[2].pBufferInfo = 0;
        modelDescriptorWrites[2].pImageInfo = &samplerInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(modelDescriptorWrites.size()), 
                               modelDescriptorWrites.data(), 0, nullptr);
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

void VulkanApplication::createCommandBuffer(size_t imageIndex) {
    // _imGuiApplication.newFrame(false);
    // _imGuiApplication.updateBuffers(inFlightFences.data(), static_cast<uint32_t>(inFlightFences.size()));
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    
    if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        assert(false && "failed to begin recording command buffer!");
    }
    
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;
    
    prt::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0 } };
    clearValues[1].depthStencil = { 1.0f, 0 };
    
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    createDrawCommands(imageIndex);
    //_imGuiApplication.drawFrame(commandBuffers[imageIndex]);

    vkCmdEndRenderPass(commandBuffers[imageIndex]);
    
    if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
        assert(false && "failed to record command buffer!");
    }
}

void VulkanApplication::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainImages.size()
    , VK_NULL_HANDLE);
    
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            assert(false && "failed to create synchronization objects for a frame!");
        }
    }
}

void VulkanApplication::updateUniformBuffers(uint32_t currentImage, 
                                             const prt::vector<glm::mat4>& modelMatrices, 
                                             const glm::mat4& viewMatrix, 
                                             const glm::mat4& projectionMatrix, 
                                             glm::vec3 viewPosition,
                                             const glm::mat4& skyProjectionMatrix) {
    // skybox
    SkyboxUBO skyboxUBO = {};

    glm::mat4 skyboxViewMatrix = viewMatrix;
    skyboxViewMatrix[3][0] = 0.0f;
    skyboxViewMatrix[3][1] = 0.0f;
    skyboxViewMatrix[3][2] = 0.0f;

    skyboxUBO.model = skyboxViewMatrix * glm::mat4(1.0f);
    skyboxUBO.projection = skyProjectionMatrix;
    skyboxUBO.projection[1][1] *= -1;

    void* skyboxData;
    vkMapMemory(device, uniformBuffers.skyboxMemory[currentImage], 0, sizeof(skyboxUBO), 0, &skyboxData);
    memcpy(skyboxData, &skyboxUBO, sizeof(skyboxUBO));
    vkUnmapMemory(device, uniformBuffers.skyboxMemory[currentImage]);                       
    // model                                           
    ModelUBO modelUBO = {};
    for (size_t i = 0; i < modelMatrices.size(); i++) {
        modelUBO.model[i] = modelMatrices[i];
    }
    modelUBO.view = viewMatrix;
    modelUBO.proj = projectionMatrix;
    modelUBO.proj[1][1] *= -1;
    modelUBO.viewPosition = viewPosition;
    
    void* modelData;
    vkMapMemory(device, uniformBuffers.modelMemory[currentImage], 0, sizeof(modelUBO), 0, &modelData);
    memcpy(modelData, &modelUBO, sizeof(modelUBO));
    vkUnmapMemory(device, uniformBuffers.modelMemory[currentImage]);
}

void VulkanApplication::drawFrame(const prt::vector<glm::mat4>& modelMatrices, 
                                  const glm::mat4& viewMatrix, 
                                  const glm::mat4& projectionMatrix, 
                                  glm::vec3 viewPosition,
                                  const glm::mat4& skyProjectionMatrix) {    
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        assert(false && "failed to acquire swap chain image!");
    }

    //createCommandBuffer(imageIndex);
    updateUniformBuffers(imageIndex, modelMatrices, viewMatrix, projectionMatrix, viewPosition,
                         skyProjectionMatrix);
    
    // if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
    //     vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());
    // }
    // imagesInFlight[imageIndex] = inFlightFences[currentFrame];
    
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
    
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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
            return availablePresentMode;
        } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = availablePresentMode;
        }
    }
    
    return bestMode;
}

VkExtent2D VulkanApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
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
    
    return indices.isComplete() && extensionsSupported && swapChainAdequate  && supportedFeatures.samplerAnisotropy;
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
        assert(false && ("failed to open file: " + filestring).c_str());
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