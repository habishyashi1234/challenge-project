#pragma once

#ifdef _WIN32
#ifdef PLUGIN_SEGFAULT_BUILDING
#define PLUGIN_SEGFAULT_API __declspec(dllexport)
#else
#define PLUGIN_SEGFAULT_API __declspec(dllimport)
#endif
#else
#define PLUGIN_SEGFAULT_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

PLUGIN_SEGFAULT_API int plugin_init(void);

PLUGIN_SEGFAULT_API const char* plugin_get_name(void);

PLUGIN_SEGFAULT_API void plugin_trigger_segfault(void);

PLUGIN_SEGFAULT_API const char* plugin_get_crash_log_path(void);

#ifdef __cplusplus
}
#endif