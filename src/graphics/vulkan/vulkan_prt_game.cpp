#include "vulkan_prt_game.h"

VulkanPRTGame::VulkanPRTGame(VulkanApplication& vulkanApplication)
    : _vulkanApplication(vulkanApplication) {}

// void VulkanPrtGame::drawFrame(const prt::vector<glm::mat4>& modelMatrices, glm::mat4& viewMatrix, glm::mat4& projectionMatrix, glm::vec3 viewPosition) {
//     vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    
//     uint32_t imageIndex;
//     VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    
//     if (result == VK_ERROR_OUT_OF_DATE_KHR) {
//         recreateSwapChain();
//         return;
//     } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
//         throw std::runtime_error("failed to acquire swap chain image!");
//     }
    
//     updateUniformBuffer(imageIndex, modelMatrices, viewMatrix, projectionMatrix, viewPosition);
    
//     VkSubmitInfo submitInfo = {};
//     submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
//     VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
//     VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
//     submitInfo.waitSemaphoreCount = 1;
//     submitInfo.pWaitSemaphores = waitSemaphores;
//     submitInfo.pWaitDstStageMask = waitStages;
    
//     submitInfo.commandBufferCount = 1;
//     submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    
//     VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
//     submitInfo.signalSemaphoreCount = 1;
//     submitInfo.pSignalSemaphores = signalSemaphores;
    
//     vkResetFences(device, 1, &inFlightFences[currentFrame]);
    
//     if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
//         throw std::runtime_error("failed to submit draw command buffer!");
//     }
    
//     VkPresentInfoKHR presentInfo = {};
//     presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    
//     presentInfo.waitSemaphoreCount = 1;
//     presentInfo.pWaitSemaphores = signalSemaphores;
    
//     VkSwapchainKHR swapChains[] = {swapChain};
//     presentInfo.swapchainCount = 1;
//     presentInfo.pSwapchains = swapChains;
    
//     presentInfo.pImageIndices = &imageIndex;
    
//     result = vkQueuePresentKHR(presentQueue, &presentInfo);
    
//     if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
//         framebufferResized = false;
//         recreateSwapChain();
//     } else if (result != VK_SUCCESS) {
//         throw std::runtime_error("failed to present swap chain image!");
//     }
    
//     currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
// }

// void createVertexBuffer(prt::vector<Model>& models);
