#include "Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
namespace WT{
    void Logger::Init(Level consoleLevel, Level fileLevel, const std::string& logFile) {
        std::vector<spdlog::sink_ptr> sinks;

        // ����̨���
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(static_cast<spdlog::level::level_enum>(consoleLevel));
        sinks.push_back(consoleSink);

        // �ļ����
        if (!logFile.empty()) {
            auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true);
            fileSink->set_level(static_cast<spdlog::level::level_enum>(fileLevel));
            sinks.push_back(fileSink);
        }

        // ����logger
        s_Logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
        s_Logger->set_level(spdlog::level::trace); // ����loggerΪ��ͼ�����sink����ʵ�����

        // ������־��ʽ
        s_Logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        // ע��logger
        spdlog::register_logger(s_Logger);

        // ���漶������
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