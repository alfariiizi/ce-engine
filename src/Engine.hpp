#include <vulkan/vulkan.hpp>
#include "DeletionQueue.hpp"
#include <optional>

class Engine
{
public:
    Engine();
    ~Engine();
    int Compute();

private:
    void PrepareCommandBuffer();
    void InitializeVulkanBase();

private: // For supporting the initialize vulkan base
    vk::PhysicalDevice PickPhysicalDevice(const std::vector<vk::QueueFlagBits>& flags);
    vk::UniqueDevice CreateDevice();

private: // Utility
    std::vector<const char*> InstanceExtensions();
    std::vector<const char*> InstanceValidations();
    std::vector<std::optional<size_t>> FindQueueFamilyIndices( const vk::PhysicalDevice& physicalDevice, const std::vector<vk::QueueFlagBits>& flags );

private:
    DeletionQueue               m_delQueue; // For non-smart-pointer (raw heap's allocation) variable
    const std::vector<vk::QueueFlagBits> m_queueFlags = { vk::QueueFlagBits::eCompute };

private:
    vk::UniqueInstance          m_pInstance;
    vk::PhysicalDevice          m_physicalDevice;
    vk::UniqueDevice            m_device;
    vk::DebugUtilsMessengerEXT  m_debugUtils;
    vk::UniqueDevice            m_pDevice;
    vk::CommandPool             m_cmdPool;
    vk::CommandBuffer           m_cmdBuffer;
    vk::Queue                   m_computeQueue;
};