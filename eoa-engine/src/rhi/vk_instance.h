#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace eoa {

// Создаёт VkInstance. В Debug-сборке включает validation layers —
// это единственная реальная страховка от "тихой" порчи GPU-состояния,
// которую иначе крайне тяжело диагностировать без чтения кода вручную.
class VulkanInstance {
public:
    explicit VulkanInstance(bool enableValidation);
    ~VulkanInstance();

    VulkanInstance(const VulkanInstance&) = delete;
    VulkanInstance& operator=(const VulkanInstance&) = delete;

    VkInstance Handle() const { return instance_; }

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    bool validationEnabled_ = false;

    void SetupDebugMessenger();
    void DestroyDebugMessenger();
    std::vector<const char*> RequiredExtensions() const;
};

} // namespace eoa
