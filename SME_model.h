#ifndef SME_MODEL_H
#define SME_MODEL_H

#include <vulkan/vulkan.h>

#include "SME_buffer.h"

namespace SME {
    class Model {
    public:
        /**
         * Loads the model and creates the necessary vertex buffers.
         * TODO: load the model from a collada (.dae) file.
         * @return true if the model was successfully loaded, false otherwise
         */
        bool loadModel();
        
        /**
         * Records the rendering commands to the passed commandBuffer.
         * @param commandBuffer the command buffer to send the draw commands.
         */
        void draw(VkCommandBuffer commandBuffer);
    private:
        SME::Buffer buffer;
    };
    
}

#endif /* SME_MODEL_H */

