#ifndef SME_PIPELINE_H
#define SME_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>

#include "SME_model.h"

namespace SME {
    class Pipeline {
    public:
        ~Pipeline();
        
        /**
         * Creates the Vulkan render pass and its associated subpasses, as well
         * as shader descriptions.
         * @return true if successfully created the render pass, false otherwise
         */
        virtual bool createRenderPass() = 0;
        
        /**
         * Creates the vulkan pipeline, usually called after the necessary
         * framebuffers are attached
         * @return true if pipeline creation was successful, false otherwise
         */
        virtual bool createPipeline() = 0;
        
        /**
         * Records the draw operations onto the passed command buffer. Called
         * multiple times, once per presentation image (double buffering, etc)
         * @param commandBuffer the command buffer to send the commands to
         * @param framebufferIndex the framebuffer index to be used
         */
        virtual void recordCommandBuffers(VkCommandBuffer commandBuffer, int framebufferIndex);
        
        /**
         * Event function called when the pipeline is added to the Render system.
         * Used for declaring the necessary extensions or other requirements to
         * the render system so it can check if the system actually supports it.
         */
        virtual void onPipelineAdded() = 0;
        
        /**
         * Adds a framebuffer to the list of attached framebuffers. The pipeline
         * will render to these.
         * @param framebuffer the framebuffer to attach to the pipeline's final
         * output
         */
        void attachFramebuffer(VkFramebuffer framebuffer);
        
        VkRenderPass getRenderPass();
    protected:
        VkRenderPass renderPass;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        std::vector<VkFramebuffer> attachedFramebuffers;
        virtual void recordDrawCommands(VkCommandBuffer commandBuffer, int framebufferIndex) = 0;
    };
    
    class TestPipeline : public Pipeline {
    public:
        bool createRenderPass();
        
        bool createPipeline();
        
        bool recordCommandBuffers();
        
        void onPipelineAdded();
    protected:
        void recordDrawCommands(VkCommandBuffer commandBuffer, int framebufferIndex);
    private:
        VkShaderModule vertexShader;
        VkShaderModule fragmentShader;
        SME::Model model;
    };
}

#endif /* SME_PIPELINE_H */

