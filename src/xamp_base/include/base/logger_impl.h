//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/singleton.h>
#include <base/stl.h>
#include <base/logger.h>

#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

#include <memory>
#include <string>

namespace spdlog {
    class logger;

    namespace sinks {
        class sink;
    }

    using sink_ptr = std::shared_ptr<sinks::sink>;
}

namespace xamp::base {

enum LogLevel {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL,
    LOG_LEVEL_OFF,
};

using LoggerMutex = std::recursive_mutex;

#define DECLARE_LOG_ARG_API(Name, Level) \
    template <typename Args>\
    void Log##Name(Args &&args) {\
		Log(Level, nullptr, 0, nullptr, "{}", std::forward<Args>(args));\
	}\
    template <typename... Args>\
	void Log##Name(std::string_view s, Args &&...args) {\
		Log(Level, nullptr, 0, nullptr, s, std::forward<Args>(args)...);\
	}\
	template <typename... Args>\
	void Log##Name(const char* filename, int32_t line, const char* func, std::string_view s, Args &&...args) {\
		Log(Level, filename, line, func, s, std::forward<Args>(args)...);\
	}

class XAMP_BASE_API Logger {
public:
    explicit Logger(const std::shared_ptr<spdlog::logger>& logger);

    DECLARE_LOG_ARG_API(Trace, LOG_LEVEL_TRACE)
	DECLARE_LOG_ARG_API(Debug, LOG_LEVEL_DEBUG)
	DECLARE_LOG_ARG_API(Info, LOG_LEVEL_INFO)
	DECLARE_LOG_ARG_API(Warn, LOG_LEVEL_WARN)
    DECLARE_LOG_ARG_API(Error, LOG_LEVEL_ERROR)
    DECLARE_LOG_ARG_API(Critical, LOG_LEVEL_CRITICAL)

    void SetLevel(LogLevel level);

    LogLevel GetLevel() const;

    const std::string & GetName() const;

    template <typename... Args>
    void Log(LogLevel level, const char* filename, int32_t line, const char* func, std::string_view s, const Args&... args) {
        auto message = fmt::format(s, args...);
        LogMsg(level, filename, line, func, message);
    }
private:
    void LogMsg(LogLevel level, const char* filename, int32_t line, const char* func, const std::string &msg) const;

    std::shared_ptr<spdlog::logger> logger_;
};

class XAMP_BASE_API LoggerManager final {
public:
    static constexpr int kMaxLogFileSize = 1024 * 1024;

    friend class Singleton<LoggerManager>;

    static LoggerManager& GetInstance() noexcept;

    XAMP_DISABLE_COPY(LoggerManager)

	LoggerManager& Startup();

    LoggerManager& AddDebugOutput();

    LoggerManager& AddLogFile(const std::string& file_name);

    LoggerManager& AddSink(spdlog::sink_ptr sink);

    Logger* GetDefaultLogger() noexcept {
        return default_logger_.get();
    }

    LoggerPtr GetLogger(const std::string_view& name);

    LoggerPtr GetLogger(const std::string& name);

    Vector<LoggerPtr> GetAllLogger();

    void SetLevel(LogLevel level);

    void Shutdown();
private:
    LoggerManager() noexcept;

    ~LoggerManager();

    Vector<spdlog::sink_ptr> sinks_;
    LoggerPtr default_logger_;
};

}

