#ifndef SME_RENDER_H
#define SME_RENDER_H

#if defined(__unix) || defined(__unix__) || defined(__linux__)
    #define LINUX
    #define VK_USE_PLATFORM_XCB_KHR
#elif defined(_WIN32)
    #define WINDOWS
#endif
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <vector>

#include "SME_pipeline.h"

namespace SME { namespace Render {
    
    struct SwapChain {
        VkSwapchainKHR handle;
        VkSurfaceFormatKHR surfaceFormat;
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
        uint32_t imageCount;
    };
    
    /**
     * This function will initialise the Vulkan renderer
     * @param applicationName name of the application
     * @param applicationVersion the version of the application, for convinience
     * use VK_MAKE_VERSION
     * @return true if initialised correctly, false if otherwise
     */
    bool init(const char* applicationName, uint32_t applicationVersion);
    
    /**
     * Adds a pipeline to the renderer system
     * @param pipeline
     */
    void addPipeline(Pipeline* pipeline);
    
    /*
     * Destroys the vulkan context and all pipelines associated with it
     * Called automatically, however, can be called manually
     */
    void cleanup();
    
    /**
     * Returns the swap chain currently used for displaying images
     * @return a structure containing the information of the swapchain
     */
    SwapChain getSwapChain();
    
    /**
     * Returns the logical device used for all operations with Vulkan.
     * TODO: multiple GPU support 
     * @return the logical device currently in use
     */
    VkDevice getLogicalDevice();
}}

#endif /* SME_RENDER_H */

