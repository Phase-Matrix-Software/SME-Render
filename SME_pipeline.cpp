#include "SME_pipeline.h"
#include "SME_render.h"
#include "SME_VkUtil.h"
#include <SME_window.h>
#include <iostream>

VkRenderPass SME::Pipeline::getRenderPass(){
    return renderPass;
}

void SME::Pipeline::attachFramebuffer(VkFramebuffer framebuffer){
    attachedFramebuffers.push_back(framebuffer);
}

void SME::Pipeline::recordCommandBuffers(VkCommandBuffer commandBuffer, int framebufferIndex){
    VkClearValue clearValue = {{0.0f, 0.0f, 0.0f, 1.0f}};
    VkRenderPassBeginInfo renderPassBeginInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,       // sType
        nullptr,                                        // *pNext
        renderPass,                                     // renderPass
        attachedFramebuffers[framebufferIndex],         //framebuffer
        {                                               // renderArea
            {                                           // offset
                0,                                      // x
                0                                       // y
            },
            {                                           // extent
                static_cast<uint32_t>(SME::Window::getWidth()),           // width
                static_cast<uint32_t>(SME::Window::getHeight()),          // height
            }
        },
        1,                                              // clearValueCount
        &clearValue                                     // *pClearValues
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
    recordDrawCommands(commandBuffer, framebufferIndex);

    vkCmdEndRenderPass(commandBuffer);
}

SME::Pipeline::~Pipeline(){
    if(pipeline != VK_NULL_HANDLE){
        vkDestroyPipeline(SME::Render::getLogicalDevice(), pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    
    if(renderPass != VK_NULL_HANDLE){
        vkDestroyRenderPass(SME::Render::getLogicalDevice(), renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }
}

void SME::TestPipeline::recordDrawCommands(VkCommandBuffer commandBuffer, int framebufferIndex){
    model.draw(commandBuffer);
}

bool SME::TestPipeline::createRenderPass(){  
    if(!model.loadModel()){
        fprintf(stderr, "Failed loading test model!\n");
        return false;
    }
    SME::Render::SwapChain swapchain = SME::Render::getSwapChain();
    
    VkAttachmentDescription attachmentDescriptions[] = {
        {
            0,                                              //flags
            swapchain.surfaceFormat.format,    //format
            VK_SAMPLE_COUNT_1_BIT,                          //samples
            VK_ATTACHMENT_LOAD_OP_CLEAR,                    //loadOp
            VK_ATTACHMENT_STORE_OP_STORE,                   //storeOp
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,                //stencilLoadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE,               //stencilLoadOp
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                //initialLayout
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR                 //finalLayout               
        }
    };
    
    //subpass description
    
    VkAttachmentReference colorAttachmentReferences[] = {
        {
            0,                                          //attachmentIndex
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL    //layout
        }
    };
    
    VkSubpassDescription subpassDescriptions[] = {
        {
            0,                                  //flags
            VK_PIPELINE_BIND_POINT_GRAPHICS,    //pipelineBindPoint
            0,                                  //inputAttachmentCount
            nullptr,                            //pInputAttachments
            1,                                  //collorAttachmentCount
            colorAttachmentReferences,          //pColorAttachments
            nullptr,                            //pResolveAttachments
            nullptr,                            //pDepthStencilAttachment
            0,                                  //preserveAttachmentCount
            nullptr                             //pPReserveAttachments
        }
    };
    
    //renderpass description
    
    VkRenderPassCreateInfo renderPassInfo;
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.flags = 0;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = attachmentDescriptions;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = subpassDescriptions;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = nullptr;
    
    VkResult result = vkCreateRenderPass(SME::Render::getLogicalDevice(), &renderPassInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed creating renderpass: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    return true;
}

bool SME::TestPipeline::createPipeline(){
    if(!SME::VkUtil::createShaderModule(&vertexShader, SME::Render::getLogicalDevice(), "shadersrc/vert.spv")){
        fprintf(stderr, "There was an error while loading the vertex shader!");
        return false;
    }
    
    if(!SME::VkUtil::createShaderModule(&fragmentShader, SME::Render::getLogicalDevice(), "shadersrc/frag.spv")){
        fprintf(stderr, "There was an error while loading the fragment shader!");
        return false;
    }
    
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos = {
        { //Vertex Shader
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,    //sType
            nullptr,                                                //pNext
            0,                                                      //flags
            VK_SHADER_STAGE_VERTEX_BIT,                             //stage
            vertexShader,                                           //module
            "main",                                                 //pName
            nullptr                                                 //pSpecializationInfo
        },
        { //Fragment Shader
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,    //sType
            nullptr,                                                //pNext
            0,                                                      //flags
            VK_SHADER_STAGE_FRAGMENT_BIT,                           //stage
            fragmentShader,                                          //module
            "main",                                                 //pName
            nullptr                                                 //pSpecializationInfo            
        }
    };
    
    VkVertexInputBindingDescription vertexBindingDescription = {
        0,                                              // binding
        8 * sizeof(float),                              // stride
        VK_VERTEX_INPUT_RATE_VERTEX                     // inputRate
    };

    VkVertexInputAttributeDescription vertexAttributeDescriptions[] = {
        {
            0,                                          // location
            vertexBindingDescription.binding,         // binding
            VK_FORMAT_R32G32B32A32_SFLOAT,              // format
            0                                           // offset
        },
        {
            1,                                          // location
            vertexBindingDescription.binding,         // binding
            VK_FORMAT_R32G32B32A32_SFLOAT,              // format
            4 * sizeof(float)                           // offset
        }
    };
    
    //vertex input description
    
    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,  //sType
        nullptr,                                                    //pNext
        0,                                                          //flags
        1,                                                          //vertexBindingDescriptionCount
        &vertexBindingDescription,                                  //pVertexbindingDescriptions
        2,                                                          //vertexAttributeDescriptionCount
        vertexAttributeDescriptions                                 //pVertexAttributeDescriptions
    };  
    
    //input assembly description
    
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,    //sType
        nullptr,                                                        //pNext
        0,                                                              //flags
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,                           //topology
        VK_FALSE                                                        //primitveRestartEnable
    };
    
    //viewport description
    //TODO: add cameras and stuff
    
    VkViewport viewport = {
        0.0f,                               //x
        0.0f,                               //y
        (float) SME::Window::getWidth(),    //width
        (float) SME::Window::getHeight(),   //height
        0.0f,                               //minDepth
        1.0f                                //maxDepth
    };
        
    VkRect2D scissor = {
        {//offset
            0,  //x
            0   //y
        },//extent
        {
            (uint32_t) SME::Window::getWidth(),
            (uint32_t) SME::Window::getHeight()
        }
    };
    
    VkPipelineViewportStateCreateInfo viewportStateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,  //sType
        nullptr,                                                //pNext
        0,                                                      //flags
        1,                                                      //viewportCount
        &viewport,                                              //pViewports
        1,                                                      //scissorCount
        &scissor                                                //pScissors
    };
    
    //resterization description
    
    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,   // sType
        nullptr,                                                      // *pNext
        0,                                                            // flags
        VK_FALSE,                                                     // depthClampEnable
        VK_FALSE,                                                     // rasterizerDiscardEnable
        VK_POLYGON_MODE_FILL,                                         // polygonMode
        VK_CULL_MODE_BACK_BIT,                                        // cullMode
        VK_FRONT_FACE_COUNTER_CLOCKWISE,                              // frontFace
        VK_FALSE,                                                     // depthBiasEnable
        0.0f,                                                         // depthBiasConstantFactor
        0.0f,                                                         // depthBiasClamp
        0.0f,                                                         // depthBiasSlopeFactor
        1.0f                                                          // lineWidth
    };
    
    //multisampling description
    
    VkPipelineMultisampleStateCreateInfo multisampleInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,     // sType
        nullptr,                                                      // *pNext
        0,                                                            // flags
        VK_SAMPLE_COUNT_1_BIT,                                        // rasterizationSamples
        VK_FALSE,                                                     // sampleShadingEnable
        1.0f,                                                         // minSampleShading
        nullptr,                                                      // *pSampleMask
        VK_FALSE,                                                     // alphaToCoverageEnable
        VK_FALSE                                                      // alphaToOneEnable
    };
    
    //blending description
    
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
        VK_FALSE,                                                     // blendEnable
        VK_BLEND_FACTOR_ONE,                                          // srcColorBlendFactor
        VK_BLEND_FACTOR_ZERO,                                         // dstColorBlendFactor
        VK_BLEND_OP_ADD,                                              // colorBlendOp
        VK_BLEND_FACTOR_ONE,                                          // srcAlphaBlendFactor
        VK_BLEND_FACTOR_ZERO,                                         // dstAlphaBlendFactor
        VK_BLEND_OP_ADD,                                              // alphaBlendOp
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT |         // colorWriteMask
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,     // sType
        nullptr,                                                      // *pNext
        0,                                                            // flags
        VK_FALSE,                                                     // logicOpEnable
        VK_LOGIC_OP_COPY,                                             // logicOp
        1,                                                            // attachmentCount
        &colorBlendAttachmentState,                                   // *pAttachments
        { 0.0f, 0.0f, 0.0f, 0.0f }                                    // blendConstants[4]
    };
    
    //layout creation
    
    VkPipelineLayoutCreateInfo layoutInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,  // sType
        nullptr,                                        // *pNext
        0,                                              // flags
        0,                                              // setLayoutCount
        nullptr,                                        // *pSetLayouts
        0,                                              // pushConstantRangeCount
        nullptr                                         // *pPushConstantRanges
    };
    
    VkResult result = vkCreatePipelineLayout(SME::Render::getLogicalDevice(), &layoutInfo, nullptr, &pipelineLayout);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed creating pipeline layout: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    //bringing all the shit togeth... err creating the actual pipeline
    
    VkGraphicsPipelineCreateInfo pipelineInfo = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,              // sType
        nullptr,                                                      // *pNext
        0,                                                            // flags
        static_cast<uint32_t>(shaderStageInfos.size()),               // stageCount
        &shaderStageInfos[0],                                         // *pStages
        &vertexInputStateInfo,                                        // *pVertexInputState;
        &inputAssemblyStateInfo,                                      // *pInputAssemblyState
        nullptr,                                                      // *pTessellationState
        &viewportStateInfo,                                           // *pViewportState
        &rasterizationStateInfo,                                      // *pRasterizationState
        &multisampleInfo,                                             // *pMultisampleState
        nullptr,                                                      // *pDepthStencilState
        &colorBlendStateInfo,                                         // *pColorBlendState
        nullptr,                                                      // *pDynamicState
        pipelineLayout,                                               // layout
        renderPass,                                                   // renderPass
        0,                                                            // subpass
        VK_NULL_HANDLE,                                               // basePipelineHandle
        -1                                                            // basePipelineIndex
    };
    
    result = vkCreateGraphicsPipelines(SME::Render::getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed creating pipeline: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    return true;
}

void SME::TestPipeline::onPipelineAdded(){
    //required extensions blah blah
}