#pragma once

#include "DeletionQueue.hpp"

#include <vulkan/vulkan.hpp>
#include <optional>

class Engine
{
public:
    Engine();
    ~Engine();
    int Compute();

private:
    void InitializeVulkanBase();
    void PrepareCommandPool();
    void PrepareCommandBuffer();
    void CreatePipelineLayout();
    void CreatePipeline();

private: // For supporting the initialize vulkan base
    vk::PhysicalDevice PickPhysicalDevice(const std::vector<vk::QueueFlagBits>& flags);
    vk::UniqueDevice CreateDevice();
    std::vector<char> readFile( const std::string& fileName, bool isSPIRV = true );
    vk::UniqueShaderModule CreateShaderModule( const std::string& fileName );

private: // Utility
    std::vector<const char*> InstanceExtensions();
    std::vector<const char*> InstanceValidations();
    std::vector<std::optional<size_t>> FindQueueFamilyIndices( const vk::PhysicalDevice& physicalDevice, const std::vector<vk::QueueFlagBits>& flags );

private:
    DeletionQueue                           m_delQueue; // For non-smart-pointer (raw heap's allocation) variable
    uint32_t                                m_queueFamilyIndex;
    const std::vector<vk::QueueFlagBits>    m_queueFlags = { vk::QueueFlagBits::eCompute };

private:
    vk::UniqueInstance          m_pInstance;
    vk::DebugUtilsMessengerEXT  m_debugUtils;
    vk::PhysicalDevice          m_physicalDevice;
    vk::UniqueDevice            m_pDevice;
    vk::UniqueCommandPool       m_pCmdPool;
    vk::UniqueCommandBuffer     m_pCmdBuffer;
    vk::Queue                   m_computeQueue;
    vk::UniquePipelineLayout    m_pPipelineLayout;
    vk::UniquePipeline          m_pPipeline;
};