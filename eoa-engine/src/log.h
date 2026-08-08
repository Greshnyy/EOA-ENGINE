#pragma once
#include <cstdio>
#include <cstdlib>

// Все ошибки движка идут через эти макросы — единая точка, куда смотреть.
// EOA_FATAL печатает сообщение и завершает процесс НЕМЕДЛЕННО (не даёт
// продолжить работу с невалидным состоянием и потом упасть где-то дальше
// с непонятным крашем без контекста).

#define EOA_LOG(fmt, ...)   std::fprintf(stdout, "[EOA] " fmt "\n", ##__VA_ARGS__)
#define EOA_WARN(fmt, ...)  std::fprintf(stderr, "[EOA][WARN] " fmt "\n", ##__VA_ARGS__)
#define EOA_ERROR(fmt, ...) std::fprintf(stderr, "[EOA][ERROR] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define EOA_FATAL(fmt, ...)                                                   \
    do {                                                                      \
        std::fflush(stdout); /* иначе буферизованные EOA_LOG теряются при abort() под pipe/redirect */ \
        std::fprintf(stderr, "[EOA][FATAL] %s:%d: " fmt "\n", __FILE__,       \
                      __LINE__, ##__VA_ARGS__);                               \
        std::fflush(stderr);                                                  \
        std::abort();                                                         \
    } while (0)

#define EOA_CHECK_VK(expr)                                                    \
    do {                                                                      \
        VkResult _res = (expr);                                              \
        if (_res != VK_SUCCESS) {                                            \
            EOA_FATAL("Vulkan call failed: %s -> VkResult=%d", #expr, _res); \
        }                                                                     \
    } while (0)
