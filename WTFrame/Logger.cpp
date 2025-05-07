#include "Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
namespace WT{
    void Logger::Init(Level consoleLevel, Level fileLevel, const std::string& logFile) {
        std::vector<spdlog::sink_ptr> sinks;

        // 控制台输出
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(static_cast<spdlog::level::level_enum>(consoleLevel));
        sinks.push_back(consoleSink);

        // 文件输出
        if (!logFile.empty()) {
            auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true);
            fileSink->set_level(static_cast<spdlog::level::level_enum>(fileLevel));
            sinks.push_back(fileSink);
        }

        // 创建logger
        s_Logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
        s_Logger->set_level(spdlog::level::trace); // 设置logger为最低级别，由sink控制实际输出

        // 设置日志格式
        s_Logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        // 注册logger
        spdlog::register_logger(s_Logger);

        // 保存级别设置
        s_ConsoleLevel = consoleLevel;
        s_FileLevel = fileLevel;
    }

    void Logger::SetConsoleLevel(Level level) {
        s_ConsoleLevel = level;
        if (s_Logger) {
            for (auto& sink : s_Logger->sinks()) {
                if (dynamic_cast<spdlog::sinks::stdout_color_sink_mt*>(sink.get())) {
                    sink->set_level(static_cast<spdlog::level::level_enum>(level));
                }
            }
        }
    }

    void Logger::SetFileLevel(Level level) {
        s_FileLevel = level;
        if (s_Logger) {
            for (auto& sink : s_Logger->sinks()) {
                if (dynamic_cast<spdlog::sinks::basic_file_sink_mt*>(sink.get())) {
                    sink->set_level(static_cast<spdlog::level::level_enum>(level));
                }
            }
        }
    }

    Logger::Level Logger::GetConsoleLevel() {
        return s_ConsoleLevel;
    }

    Logger::Level Logger::GetFileLevel() {
        return s_FileLevel;
    }
}