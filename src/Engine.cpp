#include "Engine.hpp"
#include "DebugUtilsMessenger.hpp"
#include <vector>
// #include "vk_init.hpp"

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

        /// The Extensensions
        // vk::enumerateInstanceExtensionProperties();
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        auto instanceExtensions = vk::enumerateInstanceExtensionProperties();
        auto enabledExtensions = this->InstanceExtensions();
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
        {
            auto queueFamilyIndices = this->FindQueueFamilyIndices( m_physicalDevice, m_queueFlags );
            m_computeQueue = m_pDevice->getQueue( queueFamilyIndices[0].value(), 0 );
        }
    }
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
    // bool isFound = true;

    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    auto& queueFamilies = flags;

    // Index 0: Compute; Index 1: Graphics (If any)
    std::vector<std::optional<size_t>> queueFamilyIndices;
    queueFamilyIndices.reserve( queueFamilies.size() );

    size_t i = 0;
    for( const auto& prop : queueFamilyProperties )
    {
        size_t j = 0;
        for( const auto& queueFam : queueFamilies )
        {
            if( prop.queueCount > 0 &&
                (prop.queueFlags & queueFam) )
            {
                queueFamilyIndices[j] = i;
            }
            ++j;
        }
    }

    // i = 0;
    // for( const auto& q : queueFamilyIndices )
    // {
    //     if( !q.has_value() )
    //         // throw std::runtime_error( std::string("FAILED: Queue family at index " + i) );
    //         isFound = false;
    //     ++i;
    // }

    return queueFamilyIndices;
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

    return extensions;
}

std::vector<const char*> Engine::InstanceValidations()
{
    return { "VK_LAYER_KHRONOS_validation" };
}

vk::UniqueDevice Engine::CreateDevice()
{
    auto queueFamily = FindQueueFamilyIndices( m_physicalDevice, m_queueFlags );

    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueInfo {
        vk::DeviceQueueCreateFlags(),
        queueFamily[0].value(), // NOTE: assume that index 0 is for compute family queue
        1,              // queue count
        &queuePriority  // queue priority
    };

    // auto deviceExtensions = m_physicalDevice.enumerateDeviceExtensionProperties();
    // std::vector<const char*> enabledExtension = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
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
