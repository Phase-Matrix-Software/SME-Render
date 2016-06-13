#ifndef SME_PIPELINE_H
#define SME_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>

namespace SME {
    class Pipeline {
    public:
        virtual bool createRenderPass() = 0;
        
        virtual bool createPipeline() = 0;
        
        virtual void recordCommandBuffers(VkCommandBuffer commandBuffer, int framebufferIndex);
        
        virtual void onPipelineAdded() = 0;
        
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
    };
}

#endif /* SME_PIPELINE_H */

