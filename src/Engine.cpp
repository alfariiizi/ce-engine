#include "Engine.hpp"

#include "DebugUtilsMessenger.hpp"

#include <vector>
#include <optional>

Engine::Engine()
{
    this->InitializeVulkanBase();
    this->PrepareCommandBuffer();
}

Engine::~Engine()
{
    m_delQueue.flush();
}

int Engine::Compute()
{
    auto si = vk::SubmitInfo{};
    si.setCommandBuffers( m_cmdBuffer );
    m_computeQueue.submit( si, VK_NULL_HANDLE );

    return 0;
}

void Engine::InitializeVulkanBase()
{
    //// Instance and Debug Utils Messenger
    {
        /// Application Info
        auto appInfo = vk::ApplicationInfo {
            "CE", VK_MAKE_VERSION( 0, 1, 0 ),
            "CE-Engine", VK_MAKE_VERSION( 1, 0, 0 ),
            VK_API_VERSION_1_3
        };

        /// Debug Utils create info
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsInfo {};
        debugUtilsInfo.setMessageSeverity( 
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
        );
        debugUtilsInfo.setMessageType(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
        );
        debugUtilsInfo.setPfnUserCallback( debugutils::debugUtilsMessengerCallback );

        /// The Validation Layers
        auto instanceLayers = vk::enumerateInstanceLayerProperties();
        auto validationLayers = this->InstanceValidations();
        std::vector<const char*> enableValidationLayers;
        enableValidationLayers.reserve( validationLayers.size() );
        for( const auto& layer : validationLayers )
        {
            auto found = std::find_if( instanceLayers.begin(), instanceLayers.end(),
                                [&layer]( const vk::LayerProperties& l ){ return strcmp( l.layerName, layer ); }
            );
            assert( found != instanceLayers.end() );
            enableValidationLayers.push_back( layer );
        }

        auto instanceExtensions = vk::enumerateInstanceExtensionProperties();
        auto enabledExtensions = this->InstanceExtensions();    // Assume that the extensions is available
        for ( const auto& extension : enabledExtensions )
        {
            auto found = std::find_if( instanceExtensions.begin(), instanceExtensions.end(),
                            [&extension]( const vk::ExtensionProperties& ext ){ return strcmp( ext.extensionName, extension ) == 0; }
            );
            assert( found != instanceExtensions.end() );
        }

        /// Instance create info
        vk::InstanceCreateInfo instanceInfo {};
        instanceInfo.setPNext( reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>( &debugUtilsInfo ) );
        instanceInfo.setPApplicationInfo( &appInfo );
        instanceInfo.setPEnabledLayerNames( enableValidationLayers );
        instanceInfo.setPEnabledExtensionNames( enabledExtensions );

        /// Creating instance and debugutils
        m_pInstance = vk::createInstanceUnique( instanceInfo );
        VkDebugUtilsMessengerEXT dbgUtils;
        debugutils::CreateDebugUtilsMessengerEXT( 
            static_cast<VkInstance>( m_pInstance.get() ), 
            reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugUtilsInfo), 
            nullptr, &dbgUtils
        );

        m_debugUtils = static_cast<vk::DebugUtilsMessengerEXT>( dbgUtils );
        m_delQueue.pushFunction( [i=m_pInstance.get(), d=m_debugUtils, f=debugutils::DestroyDebugUtilsMessengerEXT](){
            f( static_cast<VkInstance>( i ), static_cast<VkDebugUtilsMessengerEXT>( d ), nullptr );
        });
    }

    //// Pick Physical Device and Create Device
    {
        m_physicalDevice = this->PickPhysicalDevice( m_queueFlags );
        m_device = this->CreateDevice();

        auto queueFam = FindQueueFamilyIndices( m_physicalDevice, m_queueFlags );
        vk::DeviceQueueInfo2 qi = {};

        // "1" because we want queueFamily of 0 (why? because queuefaily of 0 have more queue count)
        // In the future, maybe it coulbe be source of bug.
        qi.setQueueFamilyIndex( queueFam[1].value() );
        qi.setQueueIndex(0);
        m_computeQueue = m_device->getQueue2( qi );
    }
}

void Engine::PrepareCommandBuffer()
{
    auto allocInfo = vk::CommandBufferAllocateInfo{};
    allocInfo.setCommandPool( m_cmdPool );
    allocInfo.setLevel( vk::CommandBufferLevel::ePrimary );
    allocInfo.setCommandBufferCount( 1 );
    m_cmdBuffer = m_pDevice->allocateCommandBuffers( allocInfo ).front();

    auto beginInfo = vk::CommandBufferBeginInfo{};
    beginInfo.setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit );

    m_cmdBuffer.begin( beginInfo );
    
    m_cmdBuffer.dispatch( 1, 1, 1 );

    m_cmdBuffer.end();
}

vk::PhysicalDevice Engine::PickPhysicalDevice(const std::vector<vk::QueueFlagBits>& flags)
{
    auto physicalDevices = m_pInstance->enumeratePhysicalDevices();
    vk::PhysicalDevice choose;
    for( auto& physicalDevice : physicalDevices )
    {
        bool found = true;
        auto indices = FindQueueFamilyIndices( physicalDevice, flags );
        for( const auto& i : indices )
        {
            if( !i.has_value() )
                found = false;
        }

        if( found )
        {
            choose = physicalDevice;
            break;
        }
    }
    return choose;
}

std::vector<std::optional<size_t>> Engine::FindQueueFamilyIndices( const vk::PhysicalDevice& physicalDevice, const std::vector<vk::QueueFlagBits>& flags )
{
    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    auto& queueFamilies = flags;

    // Index 0: Compute; Index 1: Graphics (If any)
    std::vector<std::optional<size_t>> queueFamilyIndices;
    queueFamilyIndices.reserve( queueFamilies.size() );

    std::vector<uint32_t> qfis;

    size_t i = 0;
    for( const auto& prop : queueFamilyProperties )
    {
        size_t j = 0;
        for( const auto& queueFam : queueFamilies )
        {
            if( prop.queueCount > 0 &&
                (prop.queueFlags & queueFam))
            {
                queueFamilyIndices[j] = i;
                qfis.push_back(static_cast<uint32_t>(i));
            }
            ++j;
        }
        ++i;
    }

    return queueFamilyIndices;
}

vk::UniqueDevice Engine::CreateDevice()
{
    auto queueFamily = FindQueueFamilyIndices( m_physicalDevice, m_queueFlags );

    // "1" because we want queueFamily of 0 (why? because queuefaily of 0 have more queue count)
    // In the future, maybe it coulbe be source of bug.
    uint32_t queueIndex = static_cast<uint32_t>( queueFamily[1].value() );

    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueInfo {
        vk::DeviceQueueCreateFlags(),
        queueIndex,     // NOTE: This also could be source of bug, in the future.
        1,              // queue count
        &queuePriority  // queue priority
    };

    // auto deviceExtensions = m_physicalDevice.enumerateDeviceExtensionProperties();
    // std::vector<const char*> enabledExtension = { VK_NV_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME };
    // std::vector<const char*> extensions;
    // extensions.reserve( enabledExtension.size() );
    // for ( const auto& extension : enabledExtension )
    // {
    //     auto found = std::find_if( deviceExtensions.begin(), deviceExtensions.end(),
    //                     [&extension]( const vk::ExtensionProperties& ext ){ return strcmp( ext.extensionName, extension ) == 0; }
    //     );
    //     assert( found != deviceExtensions.end() );
    //     extensions.push_back( extension );
    // }
    std::vector<const char*> extensions = {}; // For now, i do not use any extension
    

    auto deviceFeatures = m_physicalDevice.getFeatures();
    auto validateLayers = this->InstanceValidations();
    vk::DeviceCreateInfo deviceInfo {
        vk::DeviceCreateFlags(),
        queueInfo,
        validateLayers,   // device validation layers
        extensions,                     // device extensions
        &deviceFeatures                 // device features
    };

    return m_physicalDevice.createDeviceUnique( deviceInfo );
}

std::vector<const char*> Engine::InstanceExtensions()
{
    /// Extensions for GLFW
    // uint32_t glfwExtensionCount = 0;
    // const char** glfwExtensions;
    // glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    std::vector<const char*> extensions;

    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    // extensions.push_back(VK_NV_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME);

    return extensions;
}

std::vector<const char*> Engine::InstanceValidations()
{
    return { "VK_LAYER_KHRONOS_validation" };
}