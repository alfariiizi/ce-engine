#pragma once

#include "DeletionQueue.hpp"
#include "Buffer.hpp"

#include <vulkan/vulkan.hpp>
#include <optional>

#include "vk_mem_alloc.h"
#include "vk_mem_alloc.hpp"

// struct Buffer
// {
//     vk::Buffer buffer;
//     // vk::DeviceMemory memory;
//     // vk::DeviceSize size;
//     vma::Allocation allocation;
// };

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
    void AllocateBuffers( size_t inputSize, size_t outputSize );    // Allocating buffer for input and output
    void PrepareCommandPool();
    void PrepareCommandBuffer();

private: // Utility
    vk::PhysicalDevice PickPhysicalDevice(const std::vector<vk::QueueFlagBits>& flags) const;
    vk::UniqueDevice CreateDevice() const;
    std::vector<char> readFile( const std::string& fileName, bool isSPIRV = true ) const;
    vk::UniqueShaderModule CreateShaderModule( const std::string& fileName ) const;
    std::vector<const char*> InstanceExtensions() const;
    std::vector<const char*> InstanceValidations() const;
    std::vector<std::optional<size_t>> FindQueueFamilyIndices( const vk::PhysicalDevice& physicalDevice, const std::vector<vk::QueueFlagBits>& flags ) const;
    // Buffer CreateBuffer( size_t allocSize, vk::BufferUsageFlags bufferUsageFlag, vma::MemoryUsage memoryUsage ) const;

private:
    template<typename T>
    void CopyToBuffer( T* dataToCopy, size_t dataSize, Buffer dstBuffer )
    {
        void* data;
        if( m_allocator.mapMemory( dstBuffer.GetAllocation(), &data ) != vk::Result::eSuccess )
            throw std::runtime_error("Failed to mapping memory\n");
        memcpy( data, dataToCopy, dataSize );
        m_allocator.unmapMemory( dstBuffer.GetAllocation() );
    }
    template<typename T>
    void CopyFromBuffer( T* variable, size_t dataSize, Buffer srcBuffer )
    {
        void* data;
        if( m_allocator.mapMemory( srcBuffer.GetAllocation(), &data ) != vk::Result::eSuccess )
            throw std::runtime_error("Failed to mapping memory\n");
        memcpy( variable, data, dataSize );
        m_allocator.unmapMemory( srcBuffer.GetAllocation() );
    }

private:
    DeletionQueue                           m_delQueue; // For non-smart-pointer (raw heap's allocation) variable
    uint32_t                                m_queueFamilyIndex;
    const std::vector<vk::QueueFlagBits>    m_queueFlags = { vk::QueueFlagBits::eCompute };
    vma::Allocator                          m_allocator;

private: // Buffer
    int inputData[1000];
    Buffer m_inputBuffer;
    float outputData[1000];
    Buffer m_outputBuffer;

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