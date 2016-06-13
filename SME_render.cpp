#include "SME_render.h"

#include <iostream>
#include "SME_VkUtil.h"
#include <SME_window.h>
#include <SME_core.h>

#ifdef DEBUG
#include <cstring>
#endif

VkInstance instance; //Vulkan instance of the engine

VkSurfaceKHR surface; //Vulkan surface that will output the graphics

VkPhysicalDevice physicalDevice = 0; //Physical device being used

VkDevice device; //Logical device used for referencing on vulkan

//Queue family indices
uint32_t presentQueueFamilyIndex = UINT32_MAX;
uint32_t graphicsQueueFamilyIndex = UINT32_MAX;

//Queues
VkQueue graphicsQueue;
VkQueue presentQueue;

//Semaphores
VkSemaphore imageAvailableSemaphore;
VkSemaphore renderingFinishedSemaphore;

//Swapchains
SME::Render::SwapChain swapChain;

//Command Pools
VkCommandPool graphicsQueueCmdPool;

//Command Buffers
std::vector<VkCommandBuffer> graphicsCommandBuffers;

//Pipelines
std::vector<SME::Pipeline*> pipelines;

void SME::Render::addPipeline(SME::Pipeline* pipeline){
    pipelines.push_back(pipeline);
    pipeline->onPipelineAdded();
}

SME::Render::SwapChain SME::Render::getSwapChain(){
    return swapChain;
}

VkDevice SME::Render::getLogicalDevice(){
    return device;
}

void render(){
    vkDeviceWaitIdle(device);
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain.handle, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    switch(result){
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            //window size changed
            break;
        default:
            fprintf(stderr, "Problem occurred during swap chain image acquisition: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
            abort();
    }

    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsCommandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;

    result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed submitting drawing queue: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        abort();
    }

    VkPresentInfoKHR presentInfo;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain.handle;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    switch(result){
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
            //window size changed
            break;
        default:
            fprintf(stderr, "Problem occurred during swap chain image present: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
            abort();
    }
}

bool createSwapchain(){
    if(swapChain.handle != VK_NULL_HANDLE){
        vkDestroySwapchainKHR(device, swapChain.handle, nullptr);
    }
    
    VkResult result;
    
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed acquiring surface capabilities: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    uint32_t formatCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (result != VK_SUCCESS || formatCount == 0) {
        fprintf(stderr, "Failed enumerating available surface formats: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, &surfaceFormats[0]);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed acquiring surface formats: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    uint32_t presentModesCount;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, nullptr);
    if (result != VK_SUCCESS || presentModesCount == 0) {
        fprintf(stderr, "Failed enumerating present modes: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    std::vector<VkPresentModeKHR> presentModes(presentModesCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, &presentModes[0]);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed acquiring present modes: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    uint32_t swapchainImageCount = surfaceCapabilities.minImageCount + 1;
    if(surfaceCapabilities.maxImageCount > 0 && swapchainImageCount > surfaceCapabilities.maxImageCount){
        swapchainImageCount = surfaceCapabilities.maxImageCount;
        #ifdef DEBUG
        fprintf(stdout, "More images requested than the device is capable of using, capped to %u\n", swapchainImageCount);
        #endif
    }    
    
    if(surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED){
        swapChain.surfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
        swapChain.surfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    } else {
        bool formatFound = false;
        for(VkSurfaceFormatKHR &format : surfaceFormats){
            if(format.format == VK_FORMAT_R8G8B8A8_UNORM){
                swapChain.surfaceFormat = format;
                formatFound = true;
                break;
            }
        }
        if(!formatFound){
            swapChain.surfaceFormat = surfaceFormats[0];
        }
    }
    
    VkExtent2D swapChainExtent = { static_cast<uint32_t>(SME::Window::getWidth()), static_cast<uint32_t>(SME::Window::getHeight()) };
    
    if(surfaceCapabilities.currentExtent.width == -1){
        if( swapChainExtent.width < surfaceCapabilities.minImageExtent.width){
            swapChainExtent.width = surfaceCapabilities.minImageExtent.width;
        }
        if(swapChainExtent.height < surfaceCapabilities.minImageExtent.height){
            swapChainExtent.height = surfaceCapabilities.minImageExtent.height;
        }
        if(swapChainExtent.width > surfaceCapabilities.maxImageExtent.width){
            swapChainExtent.width = surfaceCapabilities.maxImageExtent.width;
        }
        if(swapChainExtent.height > surfaceCapabilities.maxImageExtent.height){
            swapChainExtent.height = surfaceCapabilities.maxImageExtent.height;
        }
    }
    
    #ifdef DEBUG
    fprintf(stdout, "Supported swap chain image usage flags:\n");
    if(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) fprintf(stdout, "\tVK_IMAGE_USAGE_TRANSFER_DST\n");
    if(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) fprintf(stdout, "\tVK_IMAGE_USAGE_TRANSFER_SRC\n");
    if(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT) fprintf(stdout, "\tVK_IMAGE_USAGE_SAMPLED\n");
    if(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) fprintf(stdout, "\tVK_IMAGE_USAGE_STORAGE\n");
    if(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) fprintf(stdout, "\tVK_IMAGE_USAGE_COLOR_ATTACHMENT\n");
    if(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) fprintf(stdout, "\tVK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n");
    if(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) fprintf(stdout, "\tVK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n");
    if(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) fprintf(stdout, "\tVK_IMAGE_USAGE_INPUT_ATTACHMENT\n");
    #endif
    
    VkImageUsageFlags imageUsageFlags;
    
    if(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT){
        imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    } else {
        fprintf(stderr, "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT not supported by the swap chain!");
        return false;
    }
    
    VkSurfaceTransformFlagBitsKHR transformFlags;
    
    //don't apply any transformation
    if(surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR){
        transformFlags = VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR;
    } else {
        transformFlags = surfaceCapabilities.currentTransform;
    }
    
    VkPresentModeKHR presentMode;
    bool presentModeSet = false;
    
    for(VkPresentModeKHR &presentModeAvailable : presentModes){
        if(presentModeAvailable == VK_PRESENT_MODE_MAILBOX_KHR){
            presentMode = presentModeAvailable;
            presentModeSet = true;
            break;
        }
    }
    if(!presentModeSet){
        for(VkPresentModeKHR &presentModeAvailable : presentModes){
            if(presentModeAvailable == VK_PRESENT_MODE_FIFO_KHR){
                presentMode = presentModeAvailable;
                presentModeSet = true;
                break;
            }
        }
        if(!presentModeSet){
            fprintf(stderr, "Couldn't set suitable present mode!");
            return false;
        }
    }
    
    VkSwapchainCreateInfoKHR swapChainInfo;
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.pNext = nullptr;
    swapChainInfo.flags = 0;
    swapChainInfo.surface = surface;
    swapChainInfo.minImageCount = swapchainImageCount;
    swapChainInfo.imageFormat = swapChain.surfaceFormat.format;
    swapChainInfo.imageColorSpace = swapChain.surfaceFormat.colorSpace;
    swapChainInfo.imageExtent = swapChainExtent;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = imageUsageFlags;
    swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainInfo.queueFamilyIndexCount = 0;
    swapChainInfo.pQueueFamilyIndices = nullptr;
    swapChainInfo.preTransform = transformFlags;
    swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainInfo.presentMode = presentMode;
    swapChainInfo.clipped = VK_TRUE;
    swapChainInfo.oldSwapchain = swapChain.handle; //previous swapchain, in case of resize
    
    result = vkCreateSwapchainKHR(device, &swapChainInfo, nullptr, &swapChain.handle);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed creating swapchain: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    result = vkGetSwapchainImagesKHR(device, swapChain.handle, &swapChain.imageCount, nullptr);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed getting swapchain image count: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    swapChain.images.resize(swapChain.imageCount);
    result = vkGetSwapchainImagesKHR(device, swapChain.handle, &swapChain.imageCount, &swapChain.images[0]);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed getting swapchain images: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    //create image views
    swapChain.imageViews.resize(swapChain.imageCount);
    
    VkImageViewCreateInfo imageViewInfo;
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.pNext = nullptr;
    imageViewInfo.flags = 0;    
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = swapChain.surfaceFormat.format;
    imageViewInfo.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
    };
    imageViewInfo.subresourceRange = {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0,
        1,
        0,
        1
    };
    
    for(size_t i = 0; i < swapChain.imageCount; i++){
        imageViewInfo.image = swapChain.images[i];
        
        result = vkCreateImageView(device, &imageViewInfo, nullptr, &swapChain.imageViews[i]);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Failed creating image view: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
            return false;
        }
    }
    
    return true;
}

bool SME::Render::init(const char* applicationName, uint32_t applicationVersion){
    //==========================Create Instance===============================//
    VkApplicationInfo appInfo;
    VkInstanceCreateInfo instanceInfo;
    
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = NULL;
    appInfo.pApplicationName = applicationName;
    appInfo.pEngineName = "SME";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
    appInfo.applicationVersion = applicationVersion;
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = NULL;
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledLayerCount = 0;
    instanceInfo.ppEnabledLayerNames = NULL;
    
    std::vector<const char *> enabledExtensions {
        VK_KHR_SURFACE_EXTENSION_NAME,
        #if defined(WINDOWS)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        #elif defined(LINUX)
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
        #endif
    };
    
    instanceInfo.enabledExtensionCount = enabledExtensions.size();
    instanceInfo.ppEnabledExtensionNames = &enabledExtensions[0];
    
    VkResult result = vkCreateInstance(&instanceInfo, NULL, &instance);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create instance: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }   
    
    //========================Acquire drawable surface========================//
    
    #if defined(WINDOWS)
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.hinstance = SME::Window::hInstance; 
        surfaceCreateInfo.hwnd = SME::Window::hwnd;           
        VkResult result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
    #elif defined(LINUX)
        VkXcbSurfaceCreateInfoKHR surfaceCreateInfo;
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.connection = SME::Window::connection;
        surfaceCreateInfo.window = SME::Window::window;
        result = vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
    #endif
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan surface: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
        
    //========================Get Physical Devices============================//
    
    std::vector<const char *> requiredExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    
    uint32_t deviceCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to query the number of physical devices present: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    #ifdef DEBUG
    printf("Number of devices found is %u\n", deviceCount);
    #endif

    // There has to be at least one device present
    if (deviceCount == 0) {
        fprintf(stderr, "Couldn't detect any device present with Vulkan support: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }

    // Get the physical devices
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    result = vkEnumeratePhysicalDevices(instance, &deviceCount, &physicalDevices[0]);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to enumerate physical devices present: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    //Enumerate all physical devices and check for their properties
    //Automatically chooses the device that fulfills the requirements
    for (uint32_t physicalDeviceIndex = 0; physicalDeviceIndex < deviceCount; physicalDeviceIndex++) {
        VkPhysicalDevice currentPhysicalDevice = physicalDevices[physicalDeviceIndex];
        
        #ifdef DEBUG
        fprintf(stdout, "=============[Debug info of device %2d]============\n", physicalDeviceIndex);
        VkPhysicalDeviceProperties deviceProperties;
        memset(&deviceProperties, 0, sizeof deviceProperties);
        vkGetPhysicalDeviceProperties(currentPhysicalDevice, &deviceProperties);        
        printf("Driver Version: %u.%u.%u\n",
                VK_VERSION_MAJOR(deviceProperties.driverVersion),
                VK_VERSION_MINOR(deviceProperties.driverVersion),
                VK_VERSION_PATCH(deviceProperties.driverVersion));
        printf("Device Name:    %s\n", deviceProperties.deviceName);
        printf("Device Type:    %d\n", deviceProperties.deviceType);
        printf("API Version:    %d.%d.%d\n",
                VK_VERSION_MAJOR(deviceProperties.apiVersion),
                VK_VERSION_MINOR(deviceProperties.apiVersion),
                VK_VERSION_PATCH(deviceProperties.apiVersion));
        fprintf(stdout, "--------------------------------------------------\n");
        #endif
        
        uint32_t extensionCount;
        result = vkEnumerateDeviceExtensionProperties(currentPhysicalDevice, nullptr, &extensionCount, nullptr);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Failed to count physical device #%u extensions: %d (%s)\n", physicalDeviceIndex, result, SME::VkUtil::translateVkResult(result));
            return false;
        }
        
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        result = vkEnumerateDeviceExtensionProperties(currentPhysicalDevice, nullptr, &extensionCount, &availableExtensions[0]);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Failed to enumerate physical device #%u extensions: %d (%s)\n", physicalDeviceIndex, result, SME::VkUtil::translateVkResult(result));
            return false;
        }
        
        #ifdef DEBUG
        fprintf(stdout, "Supported extensions on this device:\n");
        for(VkExtensionProperties properties : availableExtensions){
            fprintf(stdout, "\t%s\n", properties.extensionName);
        }
        fprintf(stdout, "--------------------------------------------------\n");
        #endif
        
        bool shouldSkip = false;
        
        for(const char* extension : requiredExtensions){
            if(!SME::VkUtil::checkExtensionAvailabile(availableExtensions, extension)){
                #ifdef DEBUG
                fprintf(stdout, "Device %u is missing required %s extension, skipping", physicalDeviceIndex, extension);
                #endif
                shouldSkip = true;
            }
        }
        
        if(shouldSkip){
            continue; 
       }
        
        //Check available queue types
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(currentPhysicalDevice, &queueFamilyCount, NULL);
        std::vector<VkQueueFamilyProperties> familyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(currentPhysicalDevice, &queueFamilyCount, &familyProperties[0]);

        for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++) {
            #ifdef DEBUG
            printf("Count of Queues in this queue family: %u\n", familyProperties[queueFamilyIndex].queueCount);
            printf("Supported operations in this queue family:\n");
            if (familyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                printf("\tGraphics\n");
            if (familyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_COMPUTE_BIT)
                printf("\tCompute\n");
            if (familyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_TRANSFER_BIT)
                printf("\tTransfer\n");
            if (familyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                printf("\tSparse Binding\n");
            fprintf(stdout, "--------------------------------------------------\n");
            #endif
            VkBool32 supportsPresentation = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(currentPhysicalDevice, queueFamilyIndex, surface, &supportsPresentation);

            if(presentQueueFamilyIndex == UINT32_MAX && supportsPresentation){
                presentQueueFamilyIndex = queueFamilyIndex;
            }
            
            if(familyProperties[queueFamilyIndex].queueCount > 0 && familyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT){
                if(graphicsQueueFamilyIndex == UINT32_MAX){
                    graphicsQueueFamilyIndex = queueFamilyIndex;
                }
            }
        }
        
        if(graphicsQueueFamilyIndex == UINT32_MAX){
            #ifdef DEBUG
            fprintf(stdout, "Device %u is missing a graphics capable queue, skipping\n", physicalDeviceIndex);
            #endif
        } else if(presentQueueFamilyIndex == UINT32_MAX){
            #ifdef DEBUG
            fprintf(stdout, "Device %u is missing a present capable queue, skipping\n", physicalDeviceIndex);
            #endif
        } else {
            #ifdef DEBUG
            fprintf(stdout, "Using device %u, with the following queues\n"
                    "\tGraphics queue family: %u\n"
                    "\tPresentation queue family: %u\n",
                    physicalDeviceIndex, graphicsQueueFamilyIndex, presentQueueFamilyIndex);
            fprintf(stdout, "==================================================\n");
            #endif
            physicalDevice = currentPhysicalDevice;
            break;
        }        
    }
    
    if(physicalDevice == 0){
        fprintf(stderr, "No compatible physical device found!");
        return false;
    }
    
    //=========================Create logical device==========================//
      
    VkDeviceCreateInfo deviceInfo;
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = NULL;
    deviceInfo.flags = 0;

    //Set enabled extensions or layers
    deviceInfo.enabledLayerCount = 0;
    deviceInfo.ppEnabledLayerNames = NULL; //No enabled layers
    deviceInfo.enabledExtensionCount = requiredExtensions.size();
    deviceInfo.ppEnabledExtensionNames = &requiredExtensions[0];
    deviceInfo.pEnabledFeatures = NULL; //No enabled features
    
    std::vector<VkDeviceQueueCreateInfo> queueCreationInfos;
    std::vector<float> queuePriorities = { 1.0f };
    
    queueCreationInfos.push_back({
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,     //sType
        nullptr,                                        //pNext
        0,                                              //flags
        graphicsQueueFamilyIndex,                       //queueFamilyIndex
        static_cast<uint32_t>(queuePriorities.size()),  //queueCount
        &queuePriorities[0]                             //pQueuePriorities
    });
    
    if(graphicsQueueFamilyIndex != presentQueueFamilyIndex){
        queueCreationInfos.push_back({
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,     //sType
            nullptr,                                        //pNext
            0,                                              //flags
            presentQueueFamilyIndex,                   //queueFamilyIndex
            static_cast<uint32_t>(queuePriorities.size()),  //queueCount
            &queuePriorities[0]                             //pQueuePriorities
        });
    }
    
    // Submit queue(s) into device info
    deviceInfo.queueCreateInfoCount = queueCreationInfos.size();
    deviceInfo.pQueueCreateInfos = &queueCreationInfos[0];
    
    result = vkCreateDevice(physicalDevice, &deviceInfo, NULL, &device);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed creating logical device: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
        
    //=============================Create Queues==============================//
    
    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);    
    vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
    
    //============================Create Semaphore============================//
        
    VkSemaphoreCreateInfo semaphoreInfo;
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.flags = 0;
    
    result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed creating image available semaphore: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderingFinishedSemaphore);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed creating rendering finished semaphore: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    //==========================Create Swapchain==============================//
    
    if(!createSwapchain()){
        fprintf(stderr, "Couldn't create swap chain!\n");
        return false;
    }
    
    //======================Start creating pipeline===========================//
    
    VkFramebufferCreateInfo framebufferInfo;
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = SME::Window::getWidth();
    framebufferInfo.height = SME::Window::getHeight();
    framebufferInfo.layers = 1;
    
    for(Pipeline* pipeline : pipelines){
        if(!pipeline->createRenderPass()){
            fprintf(stderr, "Failed creating pipeline render pass!\n");
            return false;
        }
        
        for(size_t i = 0; i < swapChain.imageCount; i++){
            framebufferInfo.renderPass = pipeline->getRenderPass();
            framebufferInfo.pAttachments = &swapChain.imageViews[i];

            VkFramebuffer framebuffer;
            result = vkCreateFramebuffer(SME::Render::getLogicalDevice(), &framebufferInfo, nullptr, &framebuffer);
            if (result != VK_SUCCESS) {
                fprintf(stderr, "Failed creating framebuffer: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
                return false;
            } else {
                pipeline->attachFramebuffer(framebuffer);
            }
        }
        
        if(!pipeline->createPipeline()){
            fprintf(stderr, "Failed creating pipeline!\n");
            return false;
        }
    }
    
    //=========================Create command buffers=========================//
    
    VkCommandPoolCreateInfo gfxCmdPoolInfo = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        0,
        graphicsQueueFamilyIndex
    };
    
    result = vkCreateCommandPool(device, &gfxCmdPoolInfo, nullptr, &graphicsQueueCmdPool);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed creating graphics command pool: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
    
    graphicsCommandBuffers.resize(swapChain.imageCount, VK_NULL_HANDLE);
    
    VkCommandBufferAllocateInfo cmdBufferAllocateInfo;    
    cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocateInfo.pNext = nullptr;
    cmdBufferAllocateInfo.commandPool = graphicsQueueCmdPool;
    cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocateInfo.commandBufferCount = swapChain.imageCount;
    
    result = vkAllocateCommandBuffers(device, &cmdBufferAllocateInfo, &graphicsCommandBuffers[0]);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed allocating graphics command buffers: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
        return false;
    }
        
    //===============Record command buffers description=======================//
    
    VkCommandBufferBeginInfo graphicsCmdBufferBeginInfo;
    graphicsCmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    graphicsCmdBufferBeginInfo.pNext = nullptr;
    graphicsCmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    graphicsCmdBufferBeginInfo.pInheritanceInfo = nullptr;
    
    VkImageSubresourceRange imageSubresourceRange;
    imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresourceRange.baseMipLevel = 0;
    imageSubresourceRange.levelCount = 1;
    imageSubresourceRange.baseArrayLayer = 0;
    imageSubresourceRange.layerCount = 1;
        
    for(size_t i = 0; i < swapChain.imageCount; i++){
        vkBeginCommandBuffer(graphicsCommandBuffers[i], &graphicsCmdBufferBeginInfo);

        if(presentQueue != graphicsQueue){
            VkImageMemoryBarrier barrierFromPresentToDraw = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,     // sType
                nullptr,                                    // *pNext
                VK_ACCESS_MEMORY_READ_BIT,                  // srcAccessMask
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,       // dstAccessMask
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,            // oldLayout
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,            // newLayout
                presentQueueFamilyIndex,                    // srcQueueFamilyIndex
                graphicsQueueFamilyIndex,                   // dstQueueFamilyIndex
                swapChain.images[i],                        // image
                imageSubresourceRange                       // subresourceRange
            };

            vkCmdPipelineBarrier( graphicsCommandBuffers[i],
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0,
                nullptr, 1, &barrierFromPresentToDraw );
        }
        
        for(Pipeline* pipeline : pipelines){
            pipeline->recordCommandBuffers(graphicsCommandBuffers[i], i);
        }
        
        if(presentQueue != graphicsQueue) {
            VkImageMemoryBarrier barrierFromDrawToPresent = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,         // sType
                nullptr,                                        // *pNext
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // srcAccessMask
                VK_ACCESS_MEMORY_READ_BIT,                      // dstAccessMask
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                // oldLayout
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                // newLayout
                graphicsQueueFamilyIndex,                       // srcQueueFamilyIndex
                presentQueueFamilyIndex,                        // dstQueueFamilyIndex
                swapChain.images[i],                            // image
                imageSubresourceRange                           // subresourceRange
            };
            vkCmdPipelineBarrier( graphicsCommandBuffers[i],
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr,
                1, &barrierFromDrawToPresent);
        }

        result = vkEndCommandBuffer(graphicsCommandBuffers[i]);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Could not record graphics command buffers: %d (%s)\n", result, SME::VkUtil::translateVkResult(result));
            return false;
        }
    }
    
    //=============================Add Hooks==================================//
    
    SME::Core::addLoopRenderHook(render);
    SME::Core::addCleanupHook(cleanup);
    
    return true;
}

void SME::Render::cleanup(){
    if(device != VK_NULL_HANDLE){
        vkDeviceWaitIdle(device);

        if(graphicsCommandBuffers.size() > 0 && graphicsCommandBuffers[0] != VK_NULL_HANDLE){
            vkFreeCommandBuffers(device, graphicsQueueCmdPool, static_cast<uint32_t>(graphicsCommandBuffers.size()), &graphicsCommandBuffers[0]);
            graphicsCommandBuffers.clear();
        }

        if(graphicsQueueCmdPool != VK_NULL_HANDLE){
            vkDestroyCommandPool(device, graphicsQueueCmdPool, nullptr);
            graphicsQueueCmdPool = VK_NULL_HANDLE;
        }

        if(imageAvailableSemaphore != VK_NULL_HANDLE){
            vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        }

        if(renderingFinishedSemaphore != VK_NULL_HANDLE){
            vkDestroySemaphore(device, renderingFinishedSemaphore, nullptr);
        }
        vkDestroyDevice(device, nullptr);
    }
    
    if(surface != VK_NULL_HANDLE){
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
    
    if(instance != VK_NULL_HANDLE){
        vkDestroyInstance(instance, NULL);
    }
}