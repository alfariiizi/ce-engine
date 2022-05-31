#include <vulkan/vulkan.hpp>

class Engine
{
public:
    Engine();
    ~Engine();
    int Compute();

private:
    void PrepareCommandBuffer();

private:
    vk::UniqueDevice    m_pDevice;
    vk::CommandPool     m_cmdPool;
    vk::CommandBuffer   m_cmdBuffer;
    vk::Queue           m_computeQueue;
};