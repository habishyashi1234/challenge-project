#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

using plugin_init_fn = int (*)();
using plugin_get_name_fn = const char* (*)();
using plugin_get_log_path_fn = const char* (*)();

class SegfaultPluginLoader {
public:
    explicit SegfaultPluginLoader(const std::string& path) {
#ifdef _WIN32
        handle_ = LoadLibraryA(path.c_str());
#else
        handle_ = dlopen(path.c_str(), RTLD_LAZY);
#endif
    }

    ~SegfaultPluginLoader() {
        if (handle_ == nullptr) {
            return;
        }
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle_));
#else
        dlclose(handle_);
#endif
    }

    SegfaultPluginLoader(const SegfaultPluginLoader&) = delete;
    SegfaultPluginLoader& operator=(const SegfaultPluginLoader&) = delete;

    template <typename T>
    T get_symbol(const char* name) const {
        if (handle_ == nullptr) {
            return nullptr;
        }
#ifdef _WIN32
        return reinterpret_cast<T>(GetProcAddress(static_cast<HMODULE>(handle_), name));
#else
        return reinterpret_cast<T>(dlsym(handle_, name));
#endif
    }

    [[nodiscard]] bool is_loaded() const {
        return handle_ != nullptr;
    }

private:
    void* handle_ = nullptr;
};

std::string get_plugin_path() {
    const char* env = std::getenv("PLUGIN_SEGFAULT_PATH");
    if (env != nullptr) {
        return env;
    }
#ifdef _WIN32
    return "./plugin_segfault.dll";
#else
    return "./libplugin_segfault.so";
#endif
}

class SegfaultPluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        loader_ = std::make_unique<SegfaultPluginLoader>(get_plugin_path());
        ASSERT_TRUE(loader_->is_loaded()) << "plugin_segfault not found. Set PLUGIN_SEGFAULT_PATH.";
    }

    void TearDown() override { loader_.reset(); }

    std::unique_ptr<SegfaultPluginLoader> loader_;
};

TEST_F(SegfaultPluginTest, LoadsSuccessfully) {
    EXPECT_TRUE(loader_->is_loaded());
}

TEST_F(SegfaultPluginTest, InitReturnsZero) {
    auto fn = loader_->get_symbol<plugin_init_fn>("plugin_init");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn(), 0);
}

TEST_F(SegfaultPluginTest, NameIsCorrect) {
    auto fn = loader_->get_symbol<plugin_get_name_fn>("plugin_get_name");
    ASSERT_NE(fn, nullptr);
    EXPECT_STREQ(fn(), "plugin_segfault");
}

TEST_F(SegfaultPluginTest, AllSymbolsPresent) {
    EXPECT_NE(loader_->get_symbol<plugin_init_fn>("plugin_init"), nullptr);
    EXPECT_NE(loader_->get_symbol<plugin_get_name_fn>("plugin_get_name"), nullptr);
    EXPECT_NE(loader_->get_symbol<plugin_get_log_path_fn>("plugin_get_crash_log_path"), nullptr);
}

TEST_F(SegfaultPluginTest, CrashLogPathRespectsEnvVar) {
    const char* expected = std::getenv("SEGFAULT_LOG_PATH");
    if (expected == nullptr) {
        GTEST_SKIP() << "SEGFAULT_LOG_PATH not set";
    }

    auto init_fn = loader_->get_symbol<plugin_init_fn>("plugin_init");
    auto path_fn = loader_->get_symbol<plugin_get_log_path_fn>("plugin_get_crash_log_path");

    ASSERT_NE(init_fn, nullptr);
    ASSERT_NE(path_fn, nullptr);
    EXPECT_EQ(init_fn(), 0);
    EXPECT_STREQ(path_fn(), expected);
}

}  // namespace