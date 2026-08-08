#include "rhi/vk_instance.h"
#include "log.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstring>

namespace eoa {

namespace {

const char* kValidationLayer = "VK_LAYER_KHRONOS_validation";

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* /*userData*/) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        EOA_WARN("[Vulkan] %s", data->pMessage);
    } else {
        EOA_LOG("[Vulkan] %s", data->pMessage);
    }
    return VK_FALSE; // не прерывать вызов, который вызвал сообщение
}

bool LayerAvailable(const char* name) {
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    for (const auto& l : layers) {
        if (std::strcmp(l.layerName, name) == 0) return true;
    }
    return false;
}

} // namespace

std::vector<const char*> VulkanInstance::RequiredExtensions() const {
    uint32_t glfwCount = 0;
    const char** glfwExt = glfwGetRequiredInstanceExtensions(&glfwCount);
    std::vector<const char*> ext(glfwExt, glfwExt + glfwCount);
    if (validationEnabled_) {
        ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return ext;
}

VulkanInstance::VulkanInstance(bool enableValidation) {
    validationEnabled_ = enableValidation && LayerAvailable(kValidationLayer);
    if (enableValidation && !validationEnabled_) {
        EOA_WARN("Запрошены validation layers, но %s не найден — продолжаю без них",
                 kValidationLayer);
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Echoes Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "EOA Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    auto extensions = RequiredExtensions();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (validationEnabled_) {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &kValidationLayer;
    }

    EOA_CHECK_VK(vkCreateInstance(&createInfo, nullptr, &instance_));
    EOA_LOG("Vulkan instance created (validation=%s)", validationEnabled_ ? "on" : "off");

    if (validationEnabled_) {
        SetupDebugMessenger();
    }
}

void VulkanInstance::SetupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = DebugCallback;

    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));
    if (!func) {
        EOA_WARN("vkCreateDebugUtilsMessengerEXT недоступен — debug messenger не создан");
        return;
    }
    EOA_CHECK_VK(func(instance_, &info, nullptr, &debugMessenger_));
}

void VulkanInstance::DestroyDebugMessenger() {
    if (debugMessenger_ == VK_NULL_HANDLE) return;
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
    if (func) func(instance_, debugMessenger_, nullptr);
}

VulkanInstance::~VulkanInstance() {
    DestroyDebugMessenger();
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
    }
}

} // namespace eoa
