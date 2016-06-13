#ifndef SME_VKUTIL_H
#define SME_VKUTIL_H

#include <vector>
#include <vulkan/vulkan.h>

namespace SME { namespace VkUtil {
    
    /**
     * Translates a VkResult variable into a human-readable version of it
     * (the enum name of said variable). Useful for error logging.
     * @param result the VkResult to be translated
     * @return a string containing the name of said enum
     */
    const char* translateVkResult(VkResult result);
    
    /**
     * Checks if a given extension is available
     * @param availableExtensions list of available versions to check against
     * @param extensionName the extension to be checked
     * @return true if the extension is available, false if it isn't
     */
    bool checkExtensionAvailabile(std::vector<VkExtensionProperties> availableExtensions, const char* extensionName);
    
    /**
     * Creates (loads) a shader module from the given file
     * @param shaderModule the VkShaderModule pointer in which to store the shader module
     * @param device device on which to create the shader module
     * @param filename the name of the file to be loaded and fed into Vulkan.
     * Must be a binary SPIR file.
     * @return true if it was successfully loaded, false if it wasn't
     */
    bool createShaderModule(VkShaderModule* shaderModule, VkDevice device, const char* filename);
}}

#endif /* SME_VKUTIL_H */

