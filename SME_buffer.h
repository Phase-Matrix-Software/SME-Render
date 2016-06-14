#ifndef SME_BUFFER_H
#define SME_BUFFER_H

#include <vulkan/vulkan.h>

namespace SME {
    class Buffer {
    public:
        ~Buffer();
        
        /**
         * Creates a vulkan buffer with the passed information, device and
         * physical device. The logical device is stored for future use when
         * uploading or reuploading data to the device itself.
         * @param bufferInfo the information required for creating the VkBuffer.
         * @param device the device on which the buffer will reside.
         * @param physicalDevice the physical representation of the logical device
         * @return true if the buffer was successfully created, false otherwise
         */
        bool createBuffer(VkBufferCreateInfo* bufferInfo, VkDevice device, VkPhysicalDevice physicalDevice);
        
        /**
         * Uploads the specified data to the buffer and the device on which resides.
         * @param data the data to be sent, of the same size as the one stated
         * in the bufferInfo passed onto the createBuffer function.
         * @return true if the data was uploaded successfully, false otherwise
         */
        bool uploadDataToDevice(void* data);
        
        VkBuffer* getHandle();
    private:
        VkDevice device;
        VkBuffer handle;
        VkDeviceMemory memory;
        uint32_t size;
    };
}

#endif /* SME_BUFFER_H */

