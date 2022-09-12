#include "Buffer.hpp"

Buffer::Buffer()
    :
    m_hasBeenInitialized( false )
{
}

Buffer::Buffer( vma::Allocator allocator, size_t allocSize, vk::BufferUsageFlags bufferUsageFlag, vma::MemoryUsage memoryUsage )
    :
    m_allocator( allocator ),
    m_hasBeenInitialized( false )
{
    auto bufferInfo = vk::BufferCreateInfo{};
    bufferInfo.setSize( allocSize );
    bufferInfo.setUsage( bufferUsageFlag );
    bufferInfo.setSharingMode( vk::SharingMode::eExclusive );

    auto allocInfo = vma::AllocationCreateInfo{};
    allocInfo.setUsage( memoryUsage );

    auto tmp = allocator.createBuffer( bufferInfo, allocInfo );
    m_buffer = tmp.first;
    m_allocation = tmp.second;

    m_hasBeenInitialized = true;
}

void Buffer::DelQueueRegistered( DeletionQueue& delQueue )
{
    delQueue.pushFunction([altor=m_allocator, b=m_buffer, a=m_allocation](){
        altor.destroyBuffer( b, a );
    });
}

vk::Buffer Buffer::GetBuffer() const
{
    return m_buffer;
}

vma::Allocation Buffer::GetAllocation() const
{
    return m_allocation;
}