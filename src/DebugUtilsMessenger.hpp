#pragma once

#include <vulkan/vulkan.hpp>
#include <iostream>

namespace debugutils
{

VKAPI_ATTR VkBool32 VKAPI_CALL
    debugUtilsMessengerCallback( VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                                VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
                                VkDebugUtilsMessengerCallbackDataEXT const * pCallbackData,
                                void * /*pUserData*/ )
{
    std::cerr << "\n\n";

    std::cerr << vk::to_string( static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>( messageSeverity ) ) << ": "
            << vk::to_string( static_cast<vk::DebugUtilsMessageTypeFlagsEXT>( messageTypes ) ) << ":\n";
    std::cerr << "\t"
            << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
    std::cerr << "\t"
            << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
    std::cerr << "\t"
            << "message         = <" << pCallbackData->pMessage << ">\n";
    if ( 0 < pCallbackData->queueLabelCount )
    {
    std::cerr << "\t"
                << "Queue Labels:\n";
    for ( uint8_t i = 0; i < pCallbackData->queueLabelCount; i++ )
    {
        std::cerr << "\t\t"
                << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
    }
    }
    if ( 0 < pCallbackData->cmdBufLabelCount )
    {
    std::cerr << "\t"
                << "CommandBuffer Labels:\n";
    for ( uint8_t i = 0; i < pCallbackData->cmdBufLabelCount; i++ )
    {
        std::cerr << "\t\t"
                << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
    }
    }
    if ( 0 < pCallbackData->objectCount )
    {
    std::cerr << "\t"
                << "Objects:\n";
    for ( uint8_t i = 0; i < pCallbackData->objectCount; i++ )
    {
        std::cerr << "\t\t"
                << "Object " << i << "\n";
        std::cerr << "\t\t\t"
                << "objectType   = "
                << vk::to_string( static_cast<vk::ObjectType>( pCallbackData->pObjects[i].objectType ) ) << "\n";
        std::cerr << "\t\t\t"
                << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
        if ( pCallbackData->pObjects[i].pObjectName )
        {
        std::cerr << "\t\t\t"
                    << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">";
        }
    }

    }
    return VK_TRUE;
}

VkResult 
    CreateDebugUtilsMessengerEXT(
                                 VkInstance instance,
                                 VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                 const VkAllocationCallbacks* pAllocator,
                                 VkDebugUtilsMessengerEXT* pDebugMessenger )
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if( func != nullptr )
    {
        return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator
)
{
    auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
    if( func != nullptr )
        func( instance, debugMessenger, pAllocator );
}

} // namespace debugutils
