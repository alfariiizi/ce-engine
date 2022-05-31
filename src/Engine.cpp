#include "Engine.hpp"

Engine::Engine()
{
    PrepareCommandBuffer();
}

Engine::~Engine()
{
    
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
