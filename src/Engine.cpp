#include "Engine.hpp"

#include "DebugUtilsMessenger.hpp"

#include <vector>
#include <optional>
#include <fstream>
#include <shaderc/shaderc.hpp>

#ifndef SHADER_PATH
    #define SHADER_PATH
#endif

// In *one* source file:
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "vk_mem_alloc.hpp"

Engine::Engine()
{
    this->InitializeVulkanBase();

    this->CreatePipelineLayout();
    this->CreatePipeline();

    this->CreateDescriptorPool();
    this->AllocateDescriptorSet();

    this->PrepareCommandPool();
    this->PrepareCommandBuffer();
}

Engine::~Engine()
{
    m_delQueue.flush();
}

int Engine::Compute()
{
    auto pFence = m_pDevice->createFenceUnique( vk::FenceCreateInfo{} );

    auto si = vk::SubmitInfo{};
    si.setCommandBuffers( m_pCmdBuffer.get() );
    m_computeQueue.submit( si, pFence.get() );

    auto result = m_pDevice->waitForFences( pFence.get(), true, UINT64_MAX );
    if( result != vk::Result::eSuccess )
    {
        throw std::runtime_error("Failed to wait fence");
    }

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
        m_pDevice = this->CreateDevice();

        auto queueFam = FindQueueFamilyIndices( m_physicalDevice, m_queueFlags );
        vk::DeviceQueueInfo2 qi = {};

        // "1" because we want queueFamily of 0 (why? because queuefaily of 0 have more queue count)
        // In the future, maybe it coulbe be source of bug.
        m_queueFamilyIndex = static_cast<uint32_t>(queueFam[1].value());
        qi.setQueueFamilyIndex( m_queueFamilyIndex );
        qi.setQueueIndex(0);
        m_computeQueue = m_pDevice->getQueue2( qi );
    }

    auto allocatorInfo = vma::AllocatorCreateInfo{};
    allocatorInfo.setInstance( m_pInstance.get() );
    allocatorInfo.setDevice( m_pDevice.get() );
    allocatorInfo.setPhysicalDevice( m_physicalDevice );
    allocatorInfo.setVulkanApiVersion( VK_API_VERSION_1_3 );
    m_allocator = vma::createAllocator( allocatorInfo );
    m_delQueue.pushFunction([a = m_allocator](){
        a.destroy();
    });
}

void Engine::PrepareCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo {};
    poolInfo.setQueueFamilyIndex( m_queueFamilyIndex );
    poolInfo.setFlags( vk::CommandPoolCreateFlagBits::eResetCommandBuffer );

    m_pCmdPool = m_pDevice->createCommandPoolUnique( poolInfo );
}

void Engine::PrepareCommandBuffer()
{
    auto allocInfo = vk::CommandBufferAllocateInfo{};
    allocInfo.setCommandPool( m_pCmdPool.get() );
    allocInfo.setLevel( vk::CommandBufferLevel::ePrimary );
    allocInfo.setCommandBufferCount( 1 );
    m_pCmdBuffer = std::move( m_pDevice->allocateCommandBuffersUnique( allocInfo ).front() );

    auto beginInfo = vk::CommandBufferBeginInfo{};
    beginInfo.setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit );

    /// Begin to Recording
    /// ==================
    m_pCmdBuffer->begin( beginInfo );
    /// ------------------

    {
        m_pCmdBuffer->bindPipeline( vk::PipelineBindPoint::eCompute, m_pPipeline.get() );
        m_pCmdBuffer->bindDescriptorSets( vk::PipelineBindPoint::eCompute, m_pPipelineLayout.get(), 0, m_pSet.get(), nullptr );
        m_pCmdBuffer->dispatch( 1, 1, 1 );
    }

    /// End Recording
    /// =============
    m_pCmdBuffer->end();
    /// -------------
}

void Engine::CreatePipelineLayout()
{
    /// Descriptor Set Layout
    {
        std::array<vk::DescriptorSetLayoutBinding, 2> setLayoutBinding;
        setLayoutBinding[0].setBinding( 0 );
        setLayoutBinding[0].setDescriptorCount( 1 );
        setLayoutBinding[0].setDescriptorType( vk::DescriptorType::eStorageBuffer );
        setLayoutBinding[0].setStageFlags( vk::ShaderStageFlagBits::eCompute );
        setLayoutBinding[1].setBinding( 1 );
        setLayoutBinding[1].setDescriptorCount( 1 );
        setLayoutBinding[1].setDescriptorType( vk::DescriptorType::eStorageBuffer );
        setLayoutBinding[1].setStageFlags( vk::ShaderStageFlagBits::eCompute );

        auto setLayoutInfo = vk::DescriptorSetLayoutCreateInfo{};
        setLayoutInfo.setBindings( setLayoutBinding );

        m_pSetLayout = m_pDevice->createDescriptorSetLayoutUnique( setLayoutInfo );
    }

    auto layoutInfo = vk::PipelineLayoutCreateInfo{};
    layoutInfo.setSetLayouts( m_pSetLayout.get() );
    m_pPipelineLayout = m_pDevice->createPipelineLayoutUnique( layoutInfo );
}

void Engine::CreatePipeline()
{
    /// Creating Module
    /// ===============
    auto shaderPath = std::string(SHADER_PATH) + std::string("/shader.comp.spv");
    auto pShaderModule = this->CreateShaderModule( shaderPath );

    /// Filling Shader Stage Info
    /// ==========================
    auto shaderStageInfo = vk::PipelineShaderStageCreateInfo{};
    shaderStageInfo.setStage( vk::ShaderStageFlagBits::eCompute );
    shaderStageInfo.setPName("main");
    shaderStageInfo.setModule( pShaderModule.get() );

    /// Creating Pipeline
    /// =================
    auto pipelineInfo = vk::ComputePipelineCreateInfo{};
    pipelineInfo.setStage( shaderStageInfo );
    pipelineInfo.setBasePipelineIndex( -1 );
    pipelineInfo.setLayout( m_pPipelineLayout.get() );

    auto checker =  m_pDevice->createComputePipelinesUnique( VK_NULL_HANDLE, pipelineInfo );
    assert( checker.result == vk::Result::eSuccess );
    m_pPipeline = std::move( checker.value[0] );    // Because we just create single pipeline
}

void Engine::CreateDescriptorPool()
{
    std::vector<vk::DescriptorPoolSize> poolSizes {
        { vk::DescriptorType::eStorageBuffer, 10 }
    };

    auto descPoolInfo = vk::DescriptorPoolCreateInfo{};
    descPoolInfo.setPoolSizes( poolSizes );
    descPoolInfo.setMaxSets( 10 );

    m_pDescPool = m_pDevice->createDescriptorPoolUnique( descPoolInfo );
}

void Engine::AllocateDescriptorSet()
{
    auto setAllocateInfo = vk::DescriptorSetAllocateInfo{};
    setAllocateInfo.setDescriptorPool( m_pDescPool.get() );
    setAllocateInfo.setSetLayouts( m_pSetLayout.get() );
    setAllocateInfo.setDescriptorSetCount( 1 );

    m_pSet = std::move( m_pDevice->allocateDescriptorSetsUnique( setAllocateInfo )[0] );
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

std::vector<char> Engine::readFile( const std::string& fileName, bool isSPIRV )
{
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error(std::string("Failed to open: ") + fileName);
    }

    // if( !isSPIRV )
    // {
    //     auto compiler = shaderc::Compiler{};
    //     auto options = shaderc::CompileOptions{};
    //     shaderc::SpvCompilationResult moduleResult = compiler.CompileGlslToSpv()
    // }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

vk::UniqueShaderModule Engine::CreateShaderModule( const std::string& fileName )
{
    auto code = this->readFile( fileName );
    auto shaderModuleInfo = vk::ShaderModuleCreateInfo{};
    shaderModuleInfo.setCodeSize( code.size() );
    shaderModuleInfo.setPCode( reinterpret_cast<uint32_t*>(code.data()) );

    return m_pDevice->createShaderModuleUnique( shaderModuleInfo );
}