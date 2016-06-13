#include "SME_VkUtil.h"

#include <stdio.h>
#include <fstream>
#include <string.h>

const char* SME::VkUtil::translateVkResult(VkResult result){
    #define ENUMCASE(e) case(e): return #e
    switch(result){
        ENUMCASE(VK_SUCCESS);
        ENUMCASE(VK_NOT_READY);
        ENUMCASE(VK_TIMEOUT);
        ENUMCASE(VK_EVENT_SET);
        ENUMCASE(VK_EVENT_RESET);
        ENUMCASE(VK_INCOMPLETE);
        ENUMCASE(VK_ERROR_OUT_OF_HOST_MEMORY);
        ENUMCASE(VK_ERROR_OUT_OF_DEVICE_MEMORY);
        ENUMCASE(VK_ERROR_INITIALIZATION_FAILED);
        ENUMCASE(VK_ERROR_DEVICE_LOST);
        ENUMCASE(VK_ERROR_MEMORY_MAP_FAILED);
        ENUMCASE(VK_ERROR_LAYER_NOT_PRESENT);
        ENUMCASE(VK_ERROR_EXTENSION_NOT_PRESENT);
        ENUMCASE(VK_ERROR_FEATURE_NOT_PRESENT);
        ENUMCASE(VK_ERROR_INCOMPATIBLE_DRIVER);
        ENUMCASE(VK_ERROR_TOO_MANY_OBJECTS);
        ENUMCASE(VK_ERROR_FORMAT_NOT_SUPPORTED);
        ENUMCASE(VK_ERROR_SURFACE_LOST_KHR);
        ENUMCASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
        ENUMCASE(VK_SUBOPTIMAL_KHR);
        ENUMCASE(VK_ERROR_OUT_OF_DATE_KHR);
        ENUMCASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
        ENUMCASE(VK_ERROR_VALIDATION_FAILED_EXT);
        ENUMCASE(VK_ERROR_INVALID_SHADER_NV);
        ENUMCASE(VK_RESULT_MAX_ENUM);
        #undef ENUMCASE
    }
}

bool SME::VkUtil::checkExtensionAvailabile(std::vector<VkExtensionProperties> availableExtensions, const char* extensionName){
    for(size_t i = 0; i < availableExtensions.size(); ++i){
        if(strcmp(availableExtensions[i].extensionName, extensionName) == 0 ){
          return true;
        }
    }
    return false;
}

bool SME::VkUtil::createShaderModule(VkShaderModule* shaderModule, VkDevice device, const char* filename){
    std::ifstream file(filename, std::ios::in|std::ios::binary|std::ios::ate);
    if(file.is_open()){
        std::streampos size = file.tellg();
        char* code = new char[size];
        file.seekg(0, std::ios::beg);
        file.read(code, size);
        file.close();
        
        VkShaderModuleCreateInfo shaderModuleInfo = {
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,    //sType
            nullptr,                                        //pNext
            0,                                              //flags
            static_cast<size_t>(size),                      //codeSize
            reinterpret_cast<const uint32_t*>(&code[0])     //codePointer
        };
        
        VkResult result = vkCreateShaderModule(device, &shaderModuleInfo, nullptr, shaderModule);
        delete[] code;
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Failed to create shader module from file %s: %d (%s)\n", filename, result, translateVkResult(result));
            return false;
        } else {
            return true;
        }
    } else {
        fprintf(stderr, "Couldn't open file!\n");
        return false;
    }
}