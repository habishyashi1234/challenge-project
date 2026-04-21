#include "plugin_segfault/plugin_segfault.h"

#include <boost/stacktrace.hpp>
#include <csignal>
#include <cstdlib>
#include <cstring>

namespace {

char g_log_path[512] = "crash_report.txt";

void signal_handler(int sig) {
    boost::stacktrace::safe_dump_to(g_log_path);
    ::signal(sig, SIG_DFL);
    ::raise(sig);
}

}  // namespace

extern "C" {

PLUGIN_SEGFAULT_API int plugin_init(void) {
    const char* env = std::getenv("SEGFAULT_LOG_PATH");
    if (env)
        std::strncpy(g_log_path, env, sizeof(g_log_path) - 1);
    ::signal(SIGSEGV, signal_handler);
    return 0;
}

PLUGIN_SEGFAULT_API const char* plugin_get_name(void) {
    return "plugin_segfault";
}

PLUGIN_SEGFAULT_API void plugin_trigger_segfault(void) {
    volatile int* p = nullptr;
    (void)*p;
}

PLUGIN_SEGFAULT_API const char* plugin_get_crash_log_path(void) {
    return g_log_path;
}

} 