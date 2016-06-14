#include "SME_buffer.h"
#include "SME_VkUtil.h"
#include <iostream>
#include <cstring>

bool SME::Buffer::createBuffer(VkBufferCreateInfo* bufferInfo, VkDevice device, VkPhysicalDevice physicalDevice){
    this->device = device;
    this->size = bufferInfo->size;
    
    VkResult result = vkCreateBuffer(device, bufferInfo, nullptr, &handle);
    if(result != VK_SUCCESS){
        fprintf(stderr, "Failed creating vertex buffer for model: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    VkMemoryRequirements bufferMemoryRequirements;
    vkGetBufferMemoryRequirements(device, handle, &bufferMemoryRequirements );

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    
    for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++){
        if( (bufferMemoryRequirements.memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ) {
            VkMemoryAllocateInfo memoryAllocateInfo = {
                VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,     // sType
                nullptr,                                    // *pNext
                bufferMemoryRequirements.size,              // allocationSize
                i                                           // memoryTypeIndex
            };

            result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &memory);
            if(result != VK_SUCCESS){
                fprintf(stderr, "Failed allocating memory for vertex buffer: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
                return false;
            } else {
                return true;
            }
        }
    }
    
    return false;
}

bool SME::Buffer::uploadDataToDevice(void* data){
    VkResult result = vkBindBufferMemory(device, handle, memory, 0);
    if(result != VK_SUCCESS){
        fprintf(stderr, "Could not bind memory for buffer: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }

    void* memoryPointer;
    result = vkMapMemory(device, memory, 0, size, 0, &memoryPointer);
    if(result != VK_SUCCESS){
        fprintf(stderr, "Could not map memory for buffer: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    memcpy(memoryPointer, data, size);

    VkMappedMemoryRange flushRange = {
      VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,            // sType
      nullptr,                                          // *pNext
      memory,                                    // memory
      0,                                                // offset
      VK_WHOLE_SIZE                                     // size
    };
    
    vkFlushMappedMemoryRanges(device, 1, &flushRange);

    vkUnmapMemory(device, memory);

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