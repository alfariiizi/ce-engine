#pragma once

#include "DeletionQueue.hpp"

#include <vulkan/vulkan.hpp>
#include <optional>

#include "vk_mem_alloc.h"
#include "vk_mem_alloc.hpp"

class Engine
{
public:
    Engine();
    ~Engine();
    int Compute();

private:
    void InitializeVulkanBase();
    void CreatePipelineLayout();
    void CreatePipeline();
    void CreateDescriptorPool();
    void AllocateDescriptorSet();
    void PrepareCommandPool();
    void PrepareCommandBuffer();

private: // For supporting the initialize vulkan base
    vk::PhysicalDevice PickPhysicalDevice(const std::vector<vk::QueueFlagBits>& flags) const;
    vk::UniqueDevice CreateDevice() const;
    std::vector<char> readFile( const std::string& fileName, bool isSPIRV = true ) const;
    vk::UniqueShaderModule CreateShaderModule( const std::string& fileName ) const;

private: // Utility
    std::vector<const char*> InstanceExtensions() const;
    std::vector<const char*> InstanceValidations() const;
    std::vector<std::optional<size_t>> FindQueueFamilyIndices( const vk::PhysicalDevice& physicalDevice, const std::vector<vk::QueueFlagBits>& flags ) const;

private:
    DeletionQueue                           m_delQueue; // For non-smart-pointer (raw heap's allocation) variable
    uint32_t                                m_queueFamilyIndex;
    const std::vector<vk::QueueFlagBits>    m_queueFlags = { vk::QueueFlagBits::eCompute };
    vma::Allocator                          m_allocator;

private:
    vk::UniqueInstance                          m_pInstance;
    vk::DebugUtilsMessengerEXT                  m_debugUtils;
    vk::PhysicalDevice                          m_physicalDevice;
    vk::UniqueDevice                            m_pDevice;
    vk::UniqueCommandPool                       m_pCmdPool;
    vk::UniqueCommandBuffer                     m_pCmdBuffer;
    vk::Queue                                   m_computeQueue;
    vk::UniquePipelineLayout                    m_pPipelineLayout;
    vk::UniquePipeline                          m_pPipeline;
    vk::UniqueDescriptorSetLayout               m_pSetLayout;
    vk::UniqueDescriptorPool                    m_pDescPool;
    // std::vector<vk::UniqueDescriptorSet>        m_pSets;
    vk::UniqueDescriptorSet                     m_pSet;
};