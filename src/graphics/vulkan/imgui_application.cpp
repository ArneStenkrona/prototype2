#include "imgui_application.h"
#include "vulkan_application.h"
#include "vulkan_util.h"
#include "src/util/io_util.h"

// Options and values to display/toggle from the UI
struct UISettings {
	bool displayModels = true;
	bool displayLogos = true;
	bool displayBackground = true;
	bool animateLight = false;
	float lightSpeed = 0.25f;
	std::array<float, 50> frameTimes{};
	float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
	float lightTimer = 0.0f;
} uiSettings;

void setImageLayout(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask);

ImGuiApplication::ImGuiApplication(VkPhysicalDevice physicalDevice, VkDevice device,
                                   Input& input, float width, float height)
    : m_physicalDevice(physicalDevice), m_device(device), m_input(input) {
    ImGui::CreateContext(); 
    init(width, height);

    dpiScaleFactor = 2.0f;
}

ImGuiApplication::~ImGuiApplication() {
    vkDeviceWaitIdle(m_device);
    if (fontImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, fontImage, nullptr);
        vkDestroyImageView(m_device, fontView, nullptr);
        vkFreeMemory(m_device, fontMemory, nullptr);
        vkDestroySampler(m_device, sampler, nullptr);
    }
    
    ImGui::DestroyContext();
}

// Initialize styles, keys, etc.
void ImGuiApplication::init(float width, float height) {
    // Color scheme
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    // Dimensions
    ImGuiIO& io = ImGui::GetIO();
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    io.DisplaySize = ImVec2{width / dpiScaleFactor, height / dpiScaleFactor};
}

// Initialize all Vulkan resources used by the ui
void ImGuiApplication::initResources(VkCommandPool& commandPool, 
                                     VkQueue& copyQueue, size_t swapchainCount,
                                     VkSampleCountFlagBits /*msaaSamples*/,
                                     size_t renderPass,
                                     size_t subpass,
                                     unsigned int renderGroup,
                                     size_t dynamicAssetIndex,
                                     GraphicsPipeline & pipeline) {
    swapchainImageCount = swapchainCount;

    ImGuiIO& io = ImGui::GetIO();

    // Create font texture
    unsigned char* fontData;
    int texWidth, texHeight;
    io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
    VkDeviceSize uploadSize = texWidth*texHeight * 4 * sizeof(char);

    // Create target image for copy
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent.width = texWidth;
    imageInfo.extent.height = texHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(m_device, &imageInfo, nullptr, &fontImage) != VK_SUCCESS) {
        assert(false && "failed to create texture image!");
    }
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(m_device, fontImage, &memReqs);
    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = vkutil::findMemoryType(m_physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if(vkAllocateMemory(m_device, &memAllocInfo, nullptr, &fontMemory) != VK_SUCCESS) {
        assert(false && "failed to allocate memory!");
    }
    if (vkBindImageMemory(m_device, fontImage, fontMemory, 0) != VK_SUCCESS) {
        assert(false && "failed to bind image memory!");
    }

    // Image view
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = fontImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(m_device, &viewInfo, nullptr, &fontView) != VK_SUCCESS) {
        assert(false && "failed to create texture image view!");
    }

    // Staging buffers for font data upload
    //vk::Buffer stagingBuffer;

    /*if(m_device->createBuffer(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer,
        uploadSize) != VK_SUCCESS) {
        assert(false && "failed to create buffer!");
    }

    stagingBuffer.map();
    memcpy(stagingBuffer.mapped, fontData, uploadSize);
    stagingBuffer.unmap();*/

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vkutil::createBuffer(m_physicalDevice, m_device, 
                         uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                         stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, uploadSize, 0, &data);
    memcpy(data, fontData, (size_t) uploadSize);
    vkUnmapMemory(m_device, stagingBufferMemory);

    
    // Copy buffer data to font image
    //VkCommandBuffer copyCmd = m_device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    // Begin command buffer
    VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer copyCmd;
    if (vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &copyCmd) != VK_SUCCESS) {
        assert(false && "failed to create command buffer!");
    }
    VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(copyCmd, &cmdBufferBeginInfo) != VK_SUCCESS) {
        assert(false && "failed to begin command buffer!");
    }

    // Prepare for transfer
    setImageLayout(
        copyCmd,
        fontImage,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    // Copy
    VkBufferImageCopy bufferCopyRegion = {};
    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegion.imageSubresource.layerCount = 1;
    bufferCopyRegion.imageExtent.width = texWidth;
    bufferCopyRegion.imageExtent.height = texHeight;
    bufferCopyRegion.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(
        copyCmd,
        stagingBuffer,
        fontImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &bufferCopyRegion
    );

    // Prepare for shader read
    setImageLayout(
        copyCmd,
        fontImage,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    //m_device->flushCommandBuffer(copyCmd, copyQueue, true);
    // flush command buffer
    if(vkEndCommandBuffer(copyCmd) != VK_SUCCESS) {
        assert(false && "failed to end command buffer!");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCmd;

    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0;
    VkFence fence;
    if (vkCreateFence(m_device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
        assert(false && "failed to create fence!");
    }
    
    // Submit to the queue
    if (vkQueueSubmit(copyQueue, 1, &submitInfo, fence) != VK_SUCCESS) {
        assert(false && "failed to submit fence to queue!");
    }
    // Wait for the fence to signal that command buffer has finished executing
    if (vkWaitForFences(m_device, 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max()) != VK_SUCCESS) {
        assert(false && "failed to wait for fence!");
    }

    vkDestroyFence(m_device, fence, nullptr);

    vkFreeCommandBuffers(m_device, commandPool, 1, &copyCmd);

    //stagingBuffer.destroy();
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    // Font texture Sampler
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler)  != VK_SUCCESS) {
        assert(false && "failed to create sampler!");
    }

    // // Descriptor pool
    // VkDescriptorPoolSize poolSize = {};
    // poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // poolSize.descriptorCount = 1;
    // prt::vector<VkDescriptorPoolSize> poolSizes = {
    //     poolSize
    // };
    // VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    // descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    // descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    // descriptorPoolInfo.pPoolSizes = poolSizes.data();
    // descriptorPoolInfo.maxSets = 2;
    // if (vkCreateDescriptorPool(m_device, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    //     assert(false && "failed to create descriptor pool!");
    // }

    // Descriptor set layout
    // VkDescriptorSetLayoutBinding setLayoutBinding = {};
    // setLayoutBinding.binding = 0;
    // setLayoutBinding.descriptorCount = 1;
    // setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // setLayoutBinding.pImmutableSamplers = nullptr;
    // setLayoutBinding.stageFlags =   VK_SHADER_STAGE_FRAGMENT_BIT;
    // prt::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
    //     setLayoutBinding
    // };
    // VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
    // descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();
    // descriptorSetLayoutCreateInfo.bindingCount = setLayoutBindings.size();
    // if (vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    //     assert(false && "failed to create descriptor set layout!");
    // }

    // // Descriptor set
    // VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {};
    // descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    // descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    // descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
    // descriptorSetAllocateInfo.descriptorSetCount = 1;
    // if(vkAllocateDescriptorSets(m_device, &descriptorSetAllocateInfo, &descriptorSet) != VK_SUCCESS) {
    //     assert(false && "failed to create descriptor sets!");
    // }
    // VkDescriptorImageInfo fontDescriptor {};
    // fontDescriptor.sampler = sampler;
    // fontDescriptor.imageView = fontView;
    // fontDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // // vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &fontDescriptor)
    // VkWriteDescriptorSet writeDescriptorSet {};
    // writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    // writeDescriptorSet.dstSet = descriptorSet;
    // writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // writeDescriptorSet.dstBinding = 0;
    // writeDescriptorSet.pImageInfo = &fontDescriptor;
    // writeDescriptorSet.descriptorCount = 1;
    // prt::vector<VkWriteDescriptorSet> writeDescriptorSets = {
    //     writeDescriptorSet
    // };

    // vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

    createPipeline(renderGroup, renderPass, subpass, dynamicAssetIndex, pipeline);
    
    // // Pipeline cache
    // VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    // pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    // if (vkCreatePipelineCache(m_device, &pipelineCacheCreateInfo, nullptr, &pipelineCache) != VK_SUCCESS) {
    //     assert(false && "failed to create pipeline cache!");
    // }

    // // Pipeline layout
    // // Push constants for UI rendering parameters
    // VkPushConstantRange pushConstantRange {};
    // pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // pushConstantRange.offset = 0;
    // pushConstantRange.size = sizeof(PushConstBlock);
    // VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    // pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // pipelineLayoutCreateInfo.setLayoutCount = 1;
    // pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    // pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    // pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    // if (vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    //     assert(false && "failed to create pipeline layout!");
    // }

    // // Setup graphics pipeline for UI rendering
    // VkPipelineInputAssemblyStateCreateInfo inputAssemblyState {};
    // inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    // inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // inputAssemblyState.flags = 0;
    // inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    // VkPipelineRasterizationStateCreateInfo rasterizationState {};
    // rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    // rasterizationState.cullMode = VK_CULL_MODE_NONE;
    // rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // rasterizationState.flags = 0;
    // rasterizationState.depthClampEnable = VK_FALSE;
    // rasterizationState.lineWidth = 1.0f;

    // // Enable blending
    // VkPipelineColorBlendAttachmentState blendAttachmentState{};
    // blendAttachmentState.blendEnable = VK_TRUE;
    // blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    // blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    // blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    // blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    // VkPipelineColorBlendStateCreateInfo colorBlendState {};
    // colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // colorBlendState.attachmentCount = 1;
    // colorBlendState.pAttachments = &blendAttachmentState;

    // VkPipelineDepthStencilStateCreateInfo depthStencilState {};
    // depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    // depthStencilState.depthTestEnable = VK_FALSE;
    // depthStencilState.depthWriteEnable = VK_FALSE;
    // depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    // depthStencilState.front = depthStencilState.back;
    // depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

    // VkPipelineViewportStateCreateInfo viewportState {};
    // viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    // viewportState.viewportCount = 1;
    // viewportState.scissorCount = 1;
    // viewportState.flags = 0;

    // VkPipelineMultisampleStateCreateInfo multisampleState {};
    // multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // multisampleState.rasterizationSamples = msaaSamples;//VK_SAMPLE_COUNT_1_BIT;
    // multisampleState.flags = 0;

    // prt::vector<VkDynamicState> dynamicStateEnables = {
    //     VK_DYNAMIC_STATE_VIEWPORT,
    //     VK_DYNAMIC_STATE_SCISSOR
    // };
    // VkPipelineDynamicStateCreateInfo dynamicState {};
    // dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    // dynamicState.pDynamicStates = dynamicStateEnables.data();
    // dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    // dynamicState.flags = 0;

    // prt::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

    // VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    // pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // pipelineCreateInfo.layout = pipelineLayout;
    // pipelineCreateInfo.renderPass = renderPass;
    // pipelineCreateInfo.flags = 0;
    // pipelineCreateInfo.basePipelineIndex = -1;
    // pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

    // pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    // pipelineCreateInfo.pRasterizationState = &rasterizationState;
    // pipelineCreateInfo.pColorBlendState = &colorBlendState;
    // pipelineCreateInfo.pMultisampleState = &multisampleState;
    // pipelineCreateInfo.pViewportState = &viewportState;
    // pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    // pipelineCreateInfo.pDynamicState = &dynamicState;
    // pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    // pipelineCreateInfo.pStages = shaderStages.data();

    // // Vertex bindings an attributes based on ImGui vertex definition
    // VkVertexInputBindingDescription vInputBindDescription {};
    // vInputBindDescription.binding = 0;
    // vInputBindDescription.stride = sizeof(ImDrawVert);
    // vInputBindDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    // prt::vector<VkVertexInputBindingDescription> vertexInputBindings = {
    //     vInputBindDescription
    // };
    // VkVertexInputAttributeDescription vInputAttribDescription0 {};
    // vInputAttribDescription0.binding = 0;
    // vInputAttribDescription0.location = 0;
    // vInputAttribDescription0.format = VK_FORMAT_R32G32_SFLOAT;
    // vInputAttribDescription0.offset = offsetof(ImDrawVert, pos);
    // VkVertexInputAttributeDescription vInputAttribDescription1 {};
    // vInputAttribDescription1.binding = 0;
    // vInputAttribDescription1.location = 1;
    // vInputAttribDescription1.format = VK_FORMAT_R32G32_SFLOAT;
    // vInputAttribDescription1.offset = offsetof(ImDrawVert, uv);
    // VkVertexInputAttributeDescription vInputAttribDescription2 {};
    // vInputAttribDescription2.binding = 0;
    // vInputAttribDescription2.location = 2;
    // vInputAttribDescription2.format = VK_FORMAT_R8G8B8A8_UNORM;
    // vInputAttribDescription2.offset = offsetof(ImDrawVert, col);
    // prt::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
    //     vInputAttribDescription0,	// Location 0: Position
    //     vInputAttribDescription1,	// Location 1: UV
    //     vInputAttribDescription2	// Location 0: Color
    // };
    // VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    // vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    // vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
    // vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
    // vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
    // vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

    // pipelineCreateInfo.pVertexInputState = &vertexInputState;

    // //shaderStages[0] = example->loadShader(ASSET_PATH "shaders/imgui/ui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    // //shaderStages[1] = example->loadShader(ASSET_PATH "shaders/imgui/ui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    // auto vertShaderCode = io_util::readFile(RESOURCE_PATH + std::string("shaders/imgui.vert.spv"));
    // auto fragShaderCode = io_util::readFile(RESOURCE_PATH + std::string("shaders/imgui.frag.spv"));

    // VkShaderModule vertShaderModule = vkutil::createShaderModule(m_device, vertShaderCode);
    // VkShaderModule fragShaderModule = vkutil::createShaderModule(m_device, fragShaderCode);
    
    // VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    // vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    // vertShaderStageInfo.module = vertShaderModule;
    // vertShaderStageInfo.pName = "main";
    
    // VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    // fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    // fragShaderStageInfo.module = fragShaderModule;
    // fragShaderStageInfo.pName = "main";

    // shaderStages[0] = vertShaderStageInfo;
    // shaderStages[1] = fragShaderStageInfo;
    
    // if (vkCreateGraphicsPipelines(m_device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
    //     assert(false && "failed to create graphics pipeline!");
    // }

    // vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    // vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
}

void ImGuiApplication::update(float width, float height,
                              float deltaTime,
                              size_t imageIndex, VkCommandPool commandPool, VkQueue queue,
                            //   VkFence *pFence, uint32_t nFence, 
                              DynamicAssets & asset,
                              GraphicsPipeline & pipeline) {
    updateInput(width, height, deltaTime);
    newFrame(!initFrameGraph);
    initFrameGraph = true;

    updateBuffers(imageIndex, commandPool, queue, asset);

    updateDrawCommands(pipeline.guiDrawCalls);

}

void ImGuiApplication::createPipeline(unsigned int renderGroup,
                                      size_t renderPass,
                                      size_t subpass,
                                      size_t dynamicAssetIndex,
                                      GraphicsPipeline & pipeline) {

    pipeline.type = PIPELINE_TYPE_GUI;
    pipeline.renderGroup = renderGroup;
    pipeline.renderPassIndex = renderPass;
    pipeline.subpass = subpass; 

    pipeline.dynamicAssetsIndex = dynamicAssetIndex;

    /* Descriptor set layout */
    // sampler
    VkDescriptorSetLayoutBinding setLayoutBinding = {};
    setLayoutBinding.binding = 0;
    setLayoutBinding.descriptorCount = 1;
    setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    setLayoutBinding.pImmutableSamplers = nullptr;
    setLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    
    pipeline.descriptorSetLayoutBindings.resize(1);
    pipeline.descriptorSetLayoutBindings[0] = setLayoutBinding;

    // Descriptor pools
    pipeline.descriptorPoolSizes.resize(1);
    pipeline.descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pipeline.descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(swapchainImageCount);
   
    // Descriptor sets
    pipeline.descriptorSets.resize(swapchainImageCount);
    pipeline.descriptorWrites.resize(swapchainImageCount);

    // pipeline.uboAttachments.resize(1);
    // pipeline.uboAttachments[0].descriptorBufferInfos.resize(swapchainImageCount);
    // pipeline.uboAttachments[0].descriptorIndex = 0;

    fontDescriptor.sampler = sampler;
    fontDescriptor.imageView = fontView;
    fontDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    for (size_t i = 0; i < swapchainImageCount; ++i) {
        // pipeline.uboAttachments[0].descriptorBufferInfos[i].buffer = uniformBufferData.uniformBuffers[i];
        // pipeline.uboAttachments[0].descriptorBufferInfos[i].offset = 0;
        // pipeline.uboAttachments[0].descriptorBufferInfos[i].range = uniformBufferData.uboData.size();
        

        pipeline.descriptorWrites[i].resize(1, VkWriteDescriptorSet{});
        
        pipeline.descriptorWrites[i][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // pipeline.descriptorWrites[i][0].dstSet = descriptorSet;
        pipeline.descriptorWrites[i][0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pipeline.descriptorWrites[i][0].dstBinding = 0;
        pipeline.descriptorWrites[i][0].pImageInfo = &fontDescriptor;
        pipeline.descriptorWrites[i][0].descriptorCount = 1;
    }

    // Vertex input
    pipeline.vertexInputBinding.binding = 0;
    pipeline.vertexInputBinding.stride = sizeof(ImDrawVert);
    pipeline.vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vInputAttribDescription0 {};
    vInputAttribDescription0.binding = 0;
    vInputAttribDescription0.location = 0;
    vInputAttribDescription0.format = VK_FORMAT_R32G32_SFLOAT;
    vInputAttribDescription0.offset = offsetof(ImDrawVert, pos);
    VkVertexInputAttributeDescription vInputAttribDescription1 {};
    vInputAttribDescription1.binding = 0;
    vInputAttribDescription1.location = 1;
    vInputAttribDescription1.format = VK_FORMAT_R32G32_SFLOAT;
    vInputAttribDescription1.offset = offsetof(ImDrawVert, uv);
    VkVertexInputAttributeDescription vInputAttribDescription2 {};
    vInputAttribDescription2.binding = 0;
    vInputAttribDescription2.location = 2;
    vInputAttribDescription2.format = VK_FORMAT_R8G8B8A8_UNORM;
    vInputAttribDescription2.offset = offsetof(ImDrawVert, col);
    
    pipeline.vertexInputAttributes = {
        vInputAttribDescription0,	// Location 0: Position
        vInputAttribDescription1,	// Location 1: UV
        vInputAttribDescription2	// Location 0: Color
    };

    auto & shaderStages = pipeline.shaderStages;
    shaderStages.resize(2);

    char vertexShader[256] = {0};
    strcpy(vertexShader, RESOURCE_PATH);
    strcat(vertexShader, "shaders/imgui.vert.spv");

    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    strcpy(shaderStages[0].pName, "main");
    strcpy(shaderStages[0].shader, vertexShader);

    char fragmentShader[256] = {0};
    strcpy(fragmentShader, RESOURCE_PATH);
    strcat(fragmentShader, "shaders/imgui.frag.spv");

    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    strcpy(shaderStages[1].pName, "main");
    strcpy(shaderStages[1].shader, fragmentShader);

    pipeline.extent = {1,1}; // will be set by render pass
    pipeline.useColorAttachment = true;
    pipeline.enableDepthBias = false;

    pipeline.cullModeFlags = VK_CULL_MODE_NONE;

    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.blendEnable = VK_TRUE;
    blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    pipeline.colorBlendAttachments.push_back(blendAttachmentState);

    pipeline.depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline.depthStencilState.depthTestEnable = VK_FALSE;
    pipeline.depthStencilState.depthWriteEnable = VK_FALSE;
    pipeline.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipeline.depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
}

// Starts a new imGui frame and sets up windows and ui elements
void ImGuiApplication::newFrame(bool updateFrameGraph)
{
    ImGui::NewFrame();

    // Init imGui windows and elements

    //ImVec4 clear_color = ImColor(114, 144, 154);
    //static float f = 0.0f;
    ImGui::TextUnformatted("IMGUI TEST");
    ImGui::TextUnformatted("IMGUI TEST");

    // Update frame time display
    if (updateFrameGraph) {
        std::rotate(uiSettings.frameTimes.begin(), uiSettings.frameTimes.begin() + 1, uiSettings.frameTimes.end());
        float frameTime = 1.0f;//1000.0f / (example->frameTimer * 1000.0f);
        uiSettings.frameTimes.back() = frameTime;
        if (frameTime < uiSettings.frameTimeMin) {
            uiSettings.frameTimeMin = frameTime;
        }
        if (frameTime > uiSettings.frameTimeMax) {
            uiSettings.frameTimeMax = frameTime;
        }
    }

    ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "", uiSettings.frameTimeMin, uiSettings.frameTimeMax, ImVec2(0, 80));

    //ImGui::Text("Camera");
    //ImGui::InputFloat3("position", &example->camera.position.x, 2);
    //ImGui::InputFloat3("rotation", &example->camera.rotation.x, 2);

    ImGui::SetNextWindowSize(ImVec2(200, 200), (1 << 2)/*ImGuiSetCond_FirstUseEver*/);
    ImGui::Begin("Example settings");
    ImGui::Checkbox("Render models", &uiSettings.displayModels);
    ImGui::Checkbox("Display logos", &uiSettings.displayLogos);
    ImGui::Checkbox("Display background", &uiSettings.displayBackground);
    ImGui::Checkbox("Animate light", &uiSettings.animateLight);
    ImGui::SliderFloat("Light speed", &uiSettings.lightSpeed, 0.1f, 1.0f);
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(650, 20), (1 << 2)/*ImGuiSetCond_FirstUseEver*/);
    ImGui::ShowDemoWindow();

    // Render to generate draw buffers
    ImGui::Render();
}

// Update vertex and index buffer containing the imGui elements when required
void ImGuiApplication::updateBuffers(size_t imageIndex, VkCommandPool /*commandPool*/, VkQueue /*queue*/,
                                    //  VkFence* pFence, uint32_t nFence, 
                                     DynamicAssets & assets) {
    assert(sizeof(ImDrawIdx) == sizeof(uint16_t) && "short length mismatch!");

    assets.vertexData[imageIndex].updated = false;

    ImDrawData* imDrawData = ImGui::GetDrawData();

    // Note: Alignment is done inside buffer creation
    VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
    VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

    if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
        return;
    }
    // Update buffers only if vertex or index count has been changed compared to current buffer size
    // auto & vertexBuffer = assets.vertexData[imageIndex].vertexBuffer;
    // auto & vertexMemory = assets.vertexData[imageIndex].vertexBufferMemory;
    
    // auto & indexBuffer = assets.vertexData[imageIndex].indexBuffer;
    // auto & indexMemory = assets.vertexData[imageIndex].indexBufferMemory;

    // copy data to linear memory range
    // prt::vector<char> vertexBufferData;
    // prt::vector<char> indexBufferData;
    assets.vertexData[imageIndex].vertexData.resize(vertexBufferSize);
    assets.vertexData[imageIndex].indexData.resize(vertexBufferSize);
    // vertexBufferData.resize(vertexBufferSize);
    // indexBufferData.resize(indexBufferSize);

    char * vtxDst = assets.vertexData[imageIndex].vertexData.data();
    char * idxDst = assets.vertexData[imageIndex].indexData.data();

    for (int n = 0; n < imDrawData->CmdListsCount; n++) {
        const ImDrawList* cmd_list = imDrawData->CmdLists[n];
        memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
        idxDst += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
    }

    // // Vertex buffer
    // if ((vertexBuffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) {
    //     // vkWaitForFences(m_device, nFence, pFence, VK_TRUE, std::numeric_limits<uint64_t>::max());

    //     if (vertexBuffer != VK_NULL_HANDLE) {
    //         vkDestroyBuffer(m_device, vertexBuffer, nullptr);
    //         vkFreeMemory(m_device, vertexMemory, nullptr);
    //     }

    //     vkutil::createAndMapBuffer(m_physicalDevice, m_device,
    //                                commandPool, queue,
    //                                vertexBufferData.data(), vertexBufferData.size(),
    //                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    //                                vertexBuffer,
    //                                vertexMemory);

    //     vertexCount = imDrawData->TotalVtxCount;
    // }

    // // Index buffer
    // if ((indexBuffer == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount)) {
    //     // vkWaitForFences(m_device, nFence, pFence, VK_TRUE, std::numeric_limits<uint64_t>::max());

    //     if (indexBuffer != VK_NULL_HANDLE) {
    //         vkDestroyBuffer(m_device, indexBuffer, nullptr);
    //         vkFreeMemory(m_device, indexMemory, nullptr);
    //     }

    //     vkutil::createAndMapBuffer(m_physicalDevice, m_device,
    //                                commandPool, queue,
    //                                indexBufferData.data(), indexBufferData.size(),
    //                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    //                                indexBuffer,
    //                                indexMemory);

    //     indexCount = imDrawData->TotalIdxCount;
    // }
}

void ImGuiApplication::updateInput(float width, float height, float deltaTime) {
    // Update imGui
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)width, (float)height);
    io.DeltaTime = deltaTime;

    double mouseX, mouseY;
    m_input.getCursorPos(mouseX, mouseY);
    io.MousePos = ImVec2(mouseX / dpiScaleFactor, mouseY / dpiScaleFactor);
    io.MouseDown[0] = m_input.getKeyPress(INPUT_KEY::KEY_MOUSE_LEFT) == GLFW_PRESS;
    io.MouseDown[1] = m_input.getKeyPress(INPUT_KEY::KEY_MOUSE_RIGHT) == GLFW_PRESS;
}

void ImGuiApplication::updateDrawCommands(prt::vector<GUIDrawCall> & drawCalls) {
    ImGuiIO& io = ImGui::GetIO();

    // Render commands
    ImDrawData* imDrawData = ImGui::GetDrawData();
    int32_t vertexOffset = 0;
    int32_t indexOffset = 0;

    size_t nDrawCalls = 0;
    
    ImDrawData * data = ImGui::GetDrawData();
    data->ScaleClipRects(data->FramebufferScale);

    for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
            const ImDrawList* cmd_list = imDrawData->CmdLists[i];

            nDrawCalls += cmd_list->CmdBuffer.Size;
    }

    drawCalls.resize(nDrawCalls);
    size_t callIndex = 0;

    if (imDrawData->CmdListsCount > 0) {
        for (int32_t i = 0; i < imDrawData->CmdListsCount; ++i) {
            const ImDrawList* cmd_list = imDrawData->CmdLists[i];
            for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; ++j) {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];

                GUIDrawCall & drawCall = drawCalls[callIndex];

                drawCall.indexOffset = indexOffset;
                drawCall.indexCount = pcmd->ElemCount;
                drawCall.vertexOffset = vertexOffset;

                drawCall.scissor.offset.x = std::max((int32_t)(dpiScaleFactor * pcmd->ClipRect.x), 0);
                drawCall.scissor.offset.y = std::max((int32_t)(dpiScaleFactor * pcmd->ClipRect.y), 0);
                drawCall.scissor.extent.width = (uint32_t)(dpiScaleFactor * (pcmd->ClipRect.z - pcmd->ClipRect.x));
                drawCall.scissor.extent.height = (uint32_t)(dpiScaleFactor * (pcmd->ClipRect.w - pcmd->ClipRect.y));

                PushConstBlock & pc = *reinterpret_cast<PushConstBlock*>(drawCall.pushConstants.data());
                pc.scale = glm::vec2(dpiScaleFactor * 2.0f / io.DisplaySize.x, dpiScaleFactor * 2.0f / io.DisplaySize.y);
                pc.translate = glm::vec2(-1.0f, -1.0f);

                indexOffset += pcmd->ElemCount;

                ++callIndex;
            }
            vertexOffset += cmd_list->VtxBuffer.Size;
        }
    }
}

// Draw current imGui frame into a command buffer
// void ImGuiApplication::drawFrame(VkCommandBuffer commandBuffer)
// {
//     ImGuiIO& io = ImGui::GetIO();

//     vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
//     vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

//     //VkViewport viewport = vks::initializers::viewport(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f);
//     VkViewport viewport {};
//     viewport.width = ImGui::GetIO().DisplaySize.x;
//     viewport.height = ImGui::GetIO().DisplaySize.y;
//     viewport.minDepth = 0.0f;
//     viewport.maxDepth = 1.0f;
//     vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

//     // UI scale and translate via push constants
//     pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
//     pushConstBlock.translate = glm::vec2(-1.0f);
//     vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

//     // Render commands
//     ImDrawData* imDrawData = ImGui::GetDrawData();
//     int32_t vertexOffset = 0;
//     int32_t indexOffset = 0;

//     if (imDrawData->CmdListsCount > 0) {

//         VkDeviceSize offsets[1] = { 0 };
//         vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
//         vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

//         for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
//         {
//             const ImDrawList* cmd_list = imDrawData->CmdLists[i];
//             for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
//             {
//                 const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
//                 VkRect2D scissorRect;
//                 scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
//                 scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
//                 scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
//                 scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
//                 vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
//                 vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
//                 indexOffset += pcmd->ElemCount;
//             }
//             vertexOffset += cmd_list->VtxBuffer.Size;
//         }
//     }
// }

void setImageLayout(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask)
{
    // Create an image barrier object
    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldImageLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageMemoryBarrier.srcAccessMask = 0;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source 
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newImageLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Put barrier inside setup command buffer
    vkCmdPipelineBarrier(
        cmdbuffer,
        srcStageMask,
        dstStageMask,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);
}

void setImageLayout(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask)
{
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = aspectMask;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 1;
    setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
}