#include "imgui_renderer.h"
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

ImGuiRenderer::ImGuiRenderer(VkPhysicalDevice physicalDevice, VkDevice device,
                             Input& /*input*/, float width, float height)
    : m_physicalDevice(physicalDevice), m_device(device)/*, m_input(input)*/ {
    ImGui::CreateContext(); 
    init(width, height);

    dpiScaleFactor = 2.0f;
}

ImGuiRenderer::~ImGuiRenderer() {
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
void ImGuiRenderer::init(float width, float height) {
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

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

// Initialize all Vulkan resources used by the ui
void ImGuiRenderer::initResources(VkCommandPool& commandPool, 
                                  VkQueue& copyQueue, size_t swapchainCount,
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

    createPipeline(renderGroup, renderPass, subpass, dynamicAssetIndex, pipeline);
}

void ImGuiRenderer::update(float /*width*/, float /*height*/,
                           float /*deltaTime*/,
                           size_t imageIndex, 
                           DynamicAssets & asset,
                           GraphicsPipeline & pipeline) {
    updateBuffers(imageIndex, asset);
    updateDrawCommands(pipeline.guiDrawCalls);
}

void ImGuiRenderer::createPipeline(unsigned int renderGroup,
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

    fontDescriptor.sampler = sampler;
    fontDescriptor.imageView = fontView;
    fontDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    for (size_t i = 0; i < swapchainImageCount; ++i) {
        pipeline.descriptorWrites[i].resize(1, VkWriteDescriptorSet{});
        
        pipeline.descriptorWrites[i][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
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

// Update vertex and index buffer containing the imGui elements when required
void ImGuiRenderer::updateBuffers(size_t imageIndex,
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

    assets.vertexData[imageIndex].vertexData.resize(vertexBufferSize);
    assets.vertexData[imageIndex].indexData.resize(vertexBufferSize);

    char * vtxDst = assets.vertexData[imageIndex].vertexData.data();
    char * idxDst = assets.vertexData[imageIndex].indexData.data();

    for (int n = 0; n < imDrawData->CmdListsCount; n++) {
        const ImDrawList* cmd_list = imDrawData->CmdLists[n];
        memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
        idxDst += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
    }
}

void ImGuiRenderer::updateDrawCommands(prt::vector<GUIDrawCall> & drawCalls) {
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