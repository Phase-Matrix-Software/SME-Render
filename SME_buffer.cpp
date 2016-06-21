#include "SME_buffer.h"
#include "SME_VkUtil.h"
#include <iostream>
#include <cstring>
#include <SME_core.h>

VkQueue transferQueue;
VkCommandPool transferQueueCommandPool;
VkCommandBuffer transferCommandBuffer;
SME::Buffer transferBuffer;

void cleanup(){
    transferBuffer.~Buffer();
}

bool SME::Buffer::initTransferBuffer(uint32_t familyIndex, VkDevice device, VkPhysicalDevice physicalDevice){
    vkGetDeviceQueue(device, familyIndex, 0, &transferQueue);
    
    VkCommandPoolCreateInfo cmdPoolInfo = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        0,
        familyIndex
    };
    
    VkResult result = vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &transferQueueCommandPool);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed creating transfer command pool: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    VkCommandBufferAllocateInfo cmdBufferAllocateInfo;    
    cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocateInfo.pNext = nullptr;
    cmdBufferAllocateInfo.commandPool = transferQueueCommandPool;
    cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocateInfo.commandBufferCount = 1;
    
    result = vkAllocateCommandBuffers(device, &cmdBufferAllocateInfo, &transferCommandBuffer);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed allocating transfer command buffer: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    VkBufferCreateInfo bufferInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,   // sType
        nullptr,                                // *pNext
        0,                                      // flags
        SME_TRANSFER_BUFFER_SIZE,               // size
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,       // usage
        VK_SHARING_MODE_EXCLUSIVE,              // sharingMode
        0,                                      // queueFamilyIndexCount
        nullptr                                 // *pQueueFamilyIndices
    };
    
    if(!transferBuffer.createBuffer(&bufferInfo, device, physicalDevice)){
        fprintf(stderr, "Failed creating transfer buffer!\n");
        return false;
    }
    
    SME::Core::addCleanupHook(cleanup);
    
    return true;
}

bool SME::Buffer::createBuffer(VkBufferCreateInfo* bufferInfo, VkDevice device, VkPhysicalDevice physicalDevice){
    this->device = device;
    this->transfer = bufferInfo->usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    VkResult result = vkCreateBuffer(device, bufferInfo, nullptr, &handle);
    if(result != VK_SUCCESS){
        fprintf(stderr, "Failed creating buffer: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    VkMemoryRequirements bufferMemoryRequirements;
    vkGetBufferMemoryRequirements(device, handle, &bufferMemoryRequirements );

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    
    for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++){
        if( (bufferMemoryRequirements.memoryTypeBits & (1 << i))
                && (memoryProperties.memoryTypes[i].propertyFlags & (transfer ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))){
            
            VkMemoryAllocateInfo memoryAllocateInfo = {
                VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,     // sType
                nullptr,                                    // *pNext
                bufferMemoryRequirements.size,              // allocationSize
                i                                           // memoryTypeIndex
            };

            result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &memory);
            if(result != VK_SUCCESS){
                fprintf(stderr, "Failed allocating memory for buffer: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
                return false;
            } else {
                return true;
            }
        }
    }
    
    fprintf(stderr, "Couldn't find suitable memory to allocate the buffer.\n");
    return false;
}

bool SME::Buffer::uploadDataToDevice(void* data, VkDeviceSize offset, VkDeviceSize size){
    if(transfer){
        if(!transferBuffer.uploadDataToDevice(data, 0, size)){
            fprintf(stderr, "Couldn't upload data to transfer buffer!\n");
            return false;
        }
        
        VkResult result = vkBindBufferMemory(device, handle, memory, 0);
        if(result != VK_SUCCESS){
            fprintf(stderr, "Could not bind memory for buffer: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
            return false;
        }
        
        //transfer data from transfer buffer to final buffer
        
        VkCommandBufferBeginInfo cmdBufferBegininfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,        //sType
            nullptr,                                            //*pNext
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,        //flags
            nullptr                                             //*pInheritanceInfo
        };

        vkBeginCommandBuffer(transferCommandBuffer, &cmdBufferBegininfo);

        VkBufferCopy bufferCopyInfo = {
            0,                                                  //srcOffset
            offset,                                             //dstOffset
            size                                                //size
        };
        
        vkCmdCopyBuffer(transferCommandBuffer, transferBuffer.handle, handle, 1, &bufferCopyInfo);

        VkBufferMemoryBarrier bufferMemoryBarrier = {
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,            //sType;
            nullptr,                                            //*pNext
            VK_ACCESS_MEMORY_WRITE_BIT,                         //srcAccessMask
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,                //dstAccessMask         //TODO: make this thing variable
            VK_QUEUE_FAMILY_IGNORED,                            //srcQueueFamilyIndex
            VK_QUEUE_FAMILY_IGNORED,                            //dstQueueFamilyIndex
            handle,                                             //buffer
            0,                                                  //offset
            VK_WHOLE_SIZE                                       //size
        };
        vkCmdPipelineBarrier(transferCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr);
        //TODO: make the 2nd and 3rd arguments variable as well
        
        vkEndCommandBuffer(transferCommandBuffer);

        // Submit command buffer and copy data from staging buffer to a vertex buffer
        VkSubmitInfo submitInfo = {
            VK_STRUCTURE_TYPE_SUBMIT_INFO,                      //sType
            nullptr,                                            //*pNext
            0,                                                  //waitSemaphoreCount
            nullptr,                                            //*pWaitSemaphores
            nullptr,                                            //*pWaitDstStageMask;
            1,                                                  //commandBufferCount
            &transferCommandBuffer,                                    //*pCommandBuffers
            0,                                                  //signalSemaphoreCount
            nullptr                                             //*pSignalSemaphores
        };

        result = vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if(result != VK_SUCCESS){
            fprintf(stderr, "Could not submit transfer commands: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
            return false;
        }

        vkDeviceWaitIdle(device);
    } else {
        VkResult result = vkBindBufferMemory(device, handle, memory, 0);
        if(result != VK_SUCCESS){
            fprintf(stderr, "Could not bind memory for buffer: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
            return false;
        }

        void* memoryPointer;
        result = vkMapMemory(device, memory, offset, size, 0, &memoryPointer);
        if(result != VK_SUCCESS){
            fprintf(stderr, "Could not map memory for buffer: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
            return false;
        }

        memcpy(memoryPointer, data, size);

        VkMappedMemoryRange flushRange = {
          VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,            // sType
          nullptr,                                          // *pNext
          memory,                                           // memory
          offset,                                           // offset
          size                                              // size
        };

        vkFlushMappedMemoryRanges(device, 1, &flushRange);

        vkUnmapMemory(device, memory);
    }
    return true;
}

VkBuffer* SME::Buffer::getHandle(){
    return &handle;
}

SME::Buffer::~Buffer(){
    if(handle != VK_NULL_HANDLE){
        vkDestroyBuffer(device, handle, nullptr);
        handle = VK_NULL_HANDLE;
    }

    if(memory != VK_NULL_HANDLE){
        vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
}