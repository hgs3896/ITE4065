#pragma once
#include <thread>

#define LOG_LEVEL 0

#define TO_STR2(arg) #arg
#define TO_STR(arg) TO_STR2(arg)

static thread_local auto ttid = std::this_thread::get_id();

// Empty placeholders for compatibility
#if LOG_LEVEL >= 5
#define INDEX_LOG_TRACE(fmt, ...) (printf("[TRACE][" TO_STR(__LINE__) "][%x] " fmt "\n", ttid __VA_OPT__(, ) __VA_ARGS__))
#else
#define INDEX_LOG_TRACE(...)      ((void)0)
#endif

#if LOG_LEVEL >= 4
#define INDEX_LOG_DEBUG(fmt, ...) (printf("[DEBUG][" TO_STR(__LINE__) "][%x] " fmt "\n", ttid __VA_OPT__(, ) __VA_ARGS__))
#else
#define INDEX_LOG_DEBUG(...)      ((void)0)
#endif

#if LOG_LEVEL >= 3
#define INDEX_LOG_INFO(fmt, ...)  (printf("[ INFO][" TO_STR(__LINE__) "][%x] " fmt "\n", ttid __VA_OPT__(, ) __VA_ARGS__))
#else
#define INDEX_LOG_INFO(...)       ((void)0)
#endif

#if LOG_LEVEL >= 2
#define INDEX_LOG_WARN(fmt, ...)  (printf("[ WARN][" TO_STR(__LINE__) "][%x] " fmt "\n", ttid __VA_OPT__(, ) __VA_ARGS__))
#else
#define INDEX_LOG_WARN(...)       ((void)0)
#endif

#if LOG_LEVEL >= 1
#define INDEX_LOG_ERROR(fmt, ...) (printf("[ERROR][" TO_STR(__LINE__) "][%x] " fmt "\n", ttid __VA_OPT__(, ) __VA_ARGS__))
#else
#define INDEX_LOG_ERROR(...)      ((void)0)
#endif
