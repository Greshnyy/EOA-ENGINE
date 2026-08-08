#include "rhi/vk_device.h"
#include "log.h"
#include <vector>
#include <string>

namespace eoa {

namespace {

const char* kSwapchainExt = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

bool HasGraphicsAndPresent(VkPhysicalDevice dev, VkSurfaceKHR surface, uint32_t& outFamily) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, families.data());

    for (uint32_t i = 0; i < count; ++i) {
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport);
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
            outFamily = i;
            return true;
        }
    }
    return false;
}

bool SupportsSwapchain(VkPhysicalDevice dev) {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> ext(count);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &count, ext.data());
    for (const auto& e : ext) {
        if (std::string(e.extensionName) == kSwapchainExt) return true;
    }
    return false;
}

} // namespace

VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface) {
    PickPhysicalDevice(instance, surface);
    CreateLogicalDevice();
}

void VulkanDevice::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0) {
        EOA_FATAL("Не найдено ни одного GPU с поддержкой Vulkan. "
                  "Если это headless-окружение без GPU — ожидаемо на этом этапе.");
    }
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    for (auto dev : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);

        uint32_t family = 0;
        bool queueOk = HasGraphicsAndPresent(dev, surface, family);
        bool swapchainOk = SupportsSwapchain(dev);

        EOA_LOG("GPU found: %s (queue ok=%d, swapchain ok=%d)",
                props.deviceName, queueOk, swapchainOk);

        if (queueOk && swapchainOk) {
            physical_ = dev;
            graphicsFamily_ = family;
            EOA_LOG("Selected GPU: %s", props.deviceName);
            return;
        }
    }
    EOA_FATAL("Ни один GPU не подходит (нужны graphics+present очередь и VK_KHR_swapchain)");
}

void VulkanDevice::CreateLogicalDevice() {
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = graphicsFamily_;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures features{}; // пока без доп. фич — добавим по мере надобности

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &features;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = &kSwapchainExt;

    EOA_CHECK_VK(vkCreateDevice(physical_, &createInfo, nullptr, &logical_));
    vkGetDeviceQueue(logical_, graphicsFamily_, 0, &graphicsQueue_);
    EOA_LOG("Logical device created, graphics queue family=%u", graphicsFamily_);
}

VulkanDevice::~VulkanDevice() {
    if (logical_ != VK_NULL_HANDLE) {
        vkDestroyDevice(logical_, nullptr);
    }
}

} // namespace eoa
