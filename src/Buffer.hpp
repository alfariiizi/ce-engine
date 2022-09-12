#pragma once

#include "vk_mem_alloc.h"
#include "vk_mem_alloc.hpp"
#include "vulkan/vulkan.hpp"
#include "DeletionQueue.hpp"

/// REMEMBER TO ALWAYS PUT THE BUFFER IN DELETION_QUEUE OBJECT

class Buffer
{
public:
    Buffer();
    Buffer( vma::Allocator allocator, size_t allocSize, vk::BufferUsageFlags bufferUsageFlag, vma::MemoryUsage memoryUsage );
    void DelQueueRegistered( DeletionQueue& delQueue );
    vk::Buffer GetBuffer() const;
    vma::Allocation GetAllocation() const;
private:
    vk::Buffer m_buffer;
    vma::Allocation m_allocation;
    bool m_hasBeenInitialized;
    vma::Allocator m_allocator;
};