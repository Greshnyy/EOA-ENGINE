#pragma once

// Платформенные макросы
#ifdef _WIN32
    #define EOA_PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define EOA_PLATFORM_LINUX 1
#elif defined(__APPLE__)
    #define EOA_PLATFORM_MACOS 1
#endif

// Экспорт/Импорт DLL
#ifdef EOA_PLATFORM_WINDOWS
    #ifdef EOA_BUILD_DLL
        #define EOA_API __declspec(dllexport)
    #else
        #define EOA_API __declspec(dllimport)
    #endif
#else
    #define EOA_API
#endif

// Макросы для отладки
#ifdef EOA_DEBUG
    #define EOA_DEBUG_BREAK() __debugbreak()
    #define EOA_LOG_INFO(msg) /* Логирование */
    #define EOA_LOG_ERROR(msg) /* Логирование ошибки */
#else
    #define EOA_DEBUG_BREAK()
    #define EOA_LOG_INFO(msg)
    #define EOA_LOG_ERROR(msg)
#endif

// Версия движка
#define EOA_VERSION_MAJOR 0
#define EOA_VERSION_MINOR 1
#define EOA_VERSION_PATCH 0
#define EOA_VERSION_STRING "0.1.0"
