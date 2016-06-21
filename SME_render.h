#ifndef SME_RENDER_H
#define SME_RENDER_H

#if defined(__linux__)
    #define VK_USE_PLATFORM_XCB_KHR
#elif defined(_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
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
     * @param pipeline it has to be an object created with new, otherwise the
     * code will cause a segfault. Object deletion is handled by the engine, no
     * need for further action by the user.
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
    
    /**
     * Returns the physical device used for the creation of the logical device
     * @return the physical device that represents the logical device in use
     */
    VkPhysicalDevice getPhysicalDevice();
}}

#endif /* SME_RENDER_H */

