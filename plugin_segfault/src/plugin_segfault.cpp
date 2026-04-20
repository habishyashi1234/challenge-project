#include "plugin_segfault/plugin_segfault.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>

#ifdef _WIN32
    #include <dbghelp.h>
    #include <windows.h>
#else
    #include <execinfo.h>
    #include <signal.h>
    #include <unistd.h>
#endif

namespace {

char g_log_path[512] = "crash_report.txt";

constexpr int kMaxFrames = 32;

#ifdef _WIN32

LONG WINAPI exception_filter(EXCEPTION_POINTERS* info) {
    FILE* f = std::fopen(g_log_path, "w");
    if (!f) return EXCEPTION_CONTINUE_SEARCH;

    std::time_t now = std::time(nullptr);
    std::array<void*, kMaxFrames> frames{};
    USHORT count = CaptureStackBackTrace(0, kMaxFrames, frames.data(), nullptr);

    std::fprintf(f, "=== Crash report ===\n");
    std::fprintf(f, "Exception : 0x%08lX\n",
                 info ? info->ExceptionRecord->ExceptionCode : 0UL);
    std::fprintf(f, "Time      : %s", std::ctime(&now));
    std::fprintf(f, "Frames    : %u\n\n", count);
    for (USHORT i = 0; i < count; ++i)
        std::fprintf(f, "  [%2u] %p\n", i, frames[i]);

    std::fclose(f);
    return EXCEPTION_CONTINUE_SEARCH;
}

void install_handler() {
    SetUnhandledExceptionFilter(exception_filter);
}

#else

void signal_handler(int sig) {
    std::array<void*, kMaxFrames> frames{};
    int count = backtrace(frames.data(), kMaxFrames);

    FILE* f = std::fopen(g_log_path, "w");
    if (!f) {
        backtrace_symbols_fd(frames.data(), count, STDERR_FILENO);
        raise(sig);
        return;
    }

    std::time_t now = std::time(nullptr);
    std::fprintf(f, "=== Crash report ===\n");
    std::fprintf(f, "Signal : %d (%s)\n", sig, strsignal(sig));
    std::fprintf(f, "Time   : %s", std::ctime(&now));
    std::fprintf(f, "Frames : %d\n\n", count);

    char** syms = backtrace_symbols(frames.data(), count);
    for (int i = 0; i < count; ++i)
        std::fprintf(f, "  [%2d] %s\n", i, syms ? syms[i] : "??");

    std::fclose(f);

    // SA_RESETHAND has already reset the signal to SIG_DFL.
    // Re-raise so the OS records the correct exit status.
    raise(sig);
}

void install_handler() {
    struct sigaction sa{};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGBUS,  &sa, nullptr);
}

#endif

}  // namespace

extern "C" {

PLUGIN_SEGFAULT_API int plugin_init(void) {
    const char* env = std::getenv("SEGFAULT_LOG_PATH");
    if (env) {
        std::strncpy(g_log_path, env, sizeof(g_log_path) - 1);
        g_log_path[sizeof(g_log_path) - 1] = '\0';
    }
    install_handler();
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