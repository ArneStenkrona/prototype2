// #include "render_pipeline.h"

// void RenderPipeline::createDrawCommands(VkCommandBuffer const & commandBuffer) {
//     VkDeviceSize offset = 0;

//     vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

//     vkCmdBindVertexBuffers(commandBuffer, 0, 1, &drawData[0].vertexBuffer, &offset);
//     vkCmdBindIndexBuffer(commandBuffer, modelData.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//     vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
//                             pipelineLayout, 0, 1, &descriptorSets.model[imageIndex], 0, nullptr);

//     for (size_t i = 0; i < drawCalls.size(); i++) {
//         auto const & drawCall = renderPipeline.drawCalls[i];
//         vkCmdPushConstants(commandBuffer, pipelineLayout, 
//                             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
//                             0, sizeof(uint32_t), (void *)&drawCall.modelMatrixIndex );
//         vkCmdPushConstants(commandBuffer, pipelineLayout, 
//                             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
//                             sizeof(uint32_t)/*offset*/, sizeof(uint32_t), (void *)&drawCall.textureIndex );

//         vkCmdDrawIndexed(commandBuffer, 
//                 drawCall.indexCount,
//                 1,
//                 drawCall.firstIndex,
//                 0,
//                 i);
//     }
// }