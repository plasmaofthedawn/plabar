#include <stdio.h>

// DEBUG_DEBUG
// DEBUG_INFO
// DEBUG_WARN
// DEBUG_ERROR

#if defined(DEBUG_DEBUG)
#define LOG_DEBUG(FORMAT, ...)\
    fprintf(stdout, "DEBUG: [%s %s %d] " FORMAT, __FILE__, __func__, __LINE__ __VA_OPT__(,) __VA_ARGS__)
#else 
#define LOG_DEBUG(FORMAT, ...)\
    do {} while(0)
#endif

#if defined(DEBUG_DEBUG) || defined(DEBUG_INFO)
#define LOG_INFO(FORMAT, ...)\
    fprintf(stdout, "INFO: [%s %s %d] " FORMAT, __FILE__, __func__, __LINE__ __VA_OPT__(,) __VA_ARGS__)
#else 
#define LOG_INFO(FORMAT, ...)\
    do {} while(0)
#endif

#if defined(DEBUG_DEBUG) || defined(DEBUG_INFO) || defined(DEBUG_WARN)
#define LOG_WARN(FORMAT, ...)\
    fprintf(stdout, "WARN: [%s %s %d] " FORMAT, __FILE__, __func__, __LINE__ __VA_OPT__(,) __VA_ARGS__)
#else 
#define LOG_WARN(FORMAT, ...)\
    do {} while(0)
#endif

#if defined(DEBUG_DEBUG) || defined(DEBUG_INFO) || defined(DEBUG_WARN) || defined(DEBUG_ERROR)
#define LOG_ERROR(FORMAT, ...)\
    fprintf(stdout, "ERROR: [%s %s %d] " FORMAT, __FILE__, __func__, __LINE__ __VA_OPT__(,) __VA_ARGS__)
#else 
#define LOG_ERROR(FORMAT, ...)\
    do {} while(0)
#endif
