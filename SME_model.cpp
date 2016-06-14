#include "SME_model.h"
#include "SME_render.h"
#include "SME_VkUtil.h"
#include <iostream>

bool SME::Model::loadModel(){
    float vertexData[] = {
        -0.7f, -0.7f, 0.0f, 1.0f,    //xyzw vertex 1
        1.0f, 0.0f, 0.0f, 1.0f,      //rgba vertex 1
        -0.7f, 0.7f, 0.0f, 1.0f,     //xyzw vertex 2
        0.0f, 1.0f, 0.0f, 1.0f,      //rgba vertex 2
        0.7f, -0.7f, 0.0f, 1.0f,     //xyzw vertex 3
        0.0f, 0.0f, 1.0f, 1.0f,      //rgba vertex 3
        0.7f, 0.7f, 0.0f, 1.0f,      //xyzw vertex 4
        0.3f, 0.3f, 0.3f, 1.0f       //rgba vertex 4
    };
    
    VkBufferCreateInfo bufferInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,   // sType
        nullptr,                                // *pNext
        0,                                      // flags
        sizeof(vertexData),                            // size
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,      // usage
        VK_SHARING_MODE_EXCLUSIVE,              // sharingMode
        0,                                      // queueFamilyIndexCount
        nullptr                                 // *pQueueFamilyIndices
    };
    
    if(!buffer.createBuffer(&bufferInfo, SME::Render::getLogicalDevice(), SME::Render::getPhysicalDevice())){
        fprintf(stderr, "Couldn't create vertex buffer for model!\n");
        return false;
    }
    
    if(!buffer.uploadDataToDevice(vertexData)){
        fprintf(stderr, "Couldn't upload vertex data to GPU!\n");
        return false;
    }
    
    return true;    
}

void SME::Model::draw(VkCommandBuffer commandBuffer){
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffer.getHandle(), &offset );
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}