#ifndef SME_BUFFER_H
#define SME_BUFFER_H

#ifndef SME_TRANSFER_BUFFER_SIZE
#define SME_TRANSFER_BUFFER_SIZE 4096 //the transfer buffer size defaults to 4kb
#endif

#include <vulkan/vulkan.h>

namespace SME {
    class Buffer {
    public:
        ~Buffer();
        
        /**
         * Initialises the transfer queue, command pool and buffer for transfer
         * operations in the gpu.
         * <b>Do not call directly! This gets automatically called when appropriate
         * by the render!</b>
         * @param familyIndex the family index on which the transfer queue should
         * be created
         * @param device logical device on which the queue, command pool and
         * buffer will reside
         * @param physicalDevice the physical representation of the previous
         * logical device
         * @return true if the buffer was successfully created or false otherwise
         */
        static bool initTransferBuffer(uint32_t familyIndex, VkDevice device, VkPhysicalDevice physicalDevice);
        
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
         * @param offset offset to be used when uploading the data to the device
         * @param size size of the data to be uploaded
         * @return true if the data was uploaded successfully, false otherwise
         */
        bool uploadDataToDevice(void* data, VkDeviceSize offset, VkDeviceSize size);
        
        VkBuffer* getHandle();
    private:
        VkDevice device;
        VkBuffer handle;
        VkDeviceMemory memory;
        bool transfer;
    };
}

#endif /* SME_BUFFER_H */

