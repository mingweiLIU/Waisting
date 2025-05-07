#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace WT{
    class Logger {
    public:
        // ��־����ö��
        enum class Level {
            TRACE = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            CRITICAL = 5,
            OFF = 6
        };

        // ��ʼ����־ϵͳ
        static void Init(Level consoleLevel = Level::INFO,
            Level fileLevel = Level::TRACE,
            const std::string& logFile = "");

        // ���ÿ���̨��־����
        static void SetConsoleLevel(Level level);

        // �����ļ���־����
        static void SetFileLevel(Level level);

        // ��ȡ��ǰ����̨��־����
        static Level GetConsoleLevel();

        // ��ȡ��ǰ�ļ���־����
        static Level GetFileLevel();

        // ��־�������
        template<typename... Args>
        static void Trace(const std::string& file, int line, const std::string& func, Args&&... args) {
            Log(Level::TRACE, file, line, func, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Debug(const std::string& file, int line, const std::string& func, Args&&... args) {
            Log(Level::DEBUG, file, line, func, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Info(const std::string& file, int line, const std::string& func, Args&&... args) {
            Log(Level::INFO, file, line, func, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Warn(const std::string& file, int line, const std::string& func, Args&&... args) {
            Log(Level::WARN, file, line, func, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Error(const std::string& file, int line, const std::string& func, Args&&... args) {
            Log(Level::ERROR, file, line, func, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Critical(const std::string& file, int line, const std::string& func, Args&&... args) {
            Log(Level::CRITICAL, file, line, func, std::forward<Args>(args)...);
        }

    private:
        static std::shared_ptr<spdlog::logger> s_Logger;
        static Level s_ConsoleLevel;
        static Level s_FileLevel;

        template<typename... Args>
        static void Log(Level level, const std::string& file, int line, const std::string& func, Args&&... args) {
            if (!s_Logger) return;

            // ���ݼ���ѡ����־����
            switch (level) {
            case Level::TRACE:
                if (static_cast<int>(level) >= static_cast<int>(s_ConsoleLevel) {
                    s_Logger->trace("[{}:{}:{}] {}", file, line, func, fmt::format(std::forward<Args>(args)...));
                }
                if (static_cast<int>(level) >= static_cast<int>(s_FileLevel)) {
                    s_Logger->trace("[{}:{}:{}] {}", file, line, func, fmt::format(std::forward<Args>(args)...));
                }
                break;
            case Level::DEBUG:
                if (static_cast<int>(level) >= static_cast<int>(s_ConsoleLevel)) {
                    s_Logger->debug("[{}:{}:{}] {}", file, line, func, fmt::format(std::forward<Args>(args)...));
                }
                if (static_cast<int>(level) >= static_cast<int>(s_FileLevel)) {
                    s_Logger->debug("[{}:{}:{}] {}", file, line, func, fmt::format(std::forward<Args>(args)...));
                }
                break;
            case Level::INFO:
                if (static_cast<int>(level) >= static_cast<int>(s_ConsoleLevel)) {
                    s_Logger->info("[{}:{}:{}] {}", file, line, func, fmt::format(std::forward<Args>(args)...));
                }
                if (static_cast<int>(level) >= static_cast<int>(s_FileLevel)) {
                    s_Logger->info("[{}:{}:{}] {}", file, line, func, fmt::format(std::forward<Args>(args)...));
                }
                break;
            case Level::WARN:
                if (static_cast<int>(level) >= static_cast<int>(s_ConsoleLevel)) {
                    s_Logger->warn("[{}:{}:{}] {}", file, line, func, fmt::format(std::forward<Args>(args)...));
                }
                if (static_cast<int>(level) >= static_cast<int>(s_FileLevel)) {
                    s_Logger->warn("[{}:{}:{}] {}", file, line, func, fmt::format(std::forward<Args>(args)...));
                }
                break;
            case Level::ERROR:
                if (static_cast<int>(level) >= static_cast<int>(s_ConsoleLevel)) {
                    s_Logger->error("[{}:{}:{}] {}", file, line, func, fmt::format(std::forward<Args>(args)...));
                }
                if (static_cast<int>(level) >= static_cast<int>(s_FileLevel)) {
                    s_Logger->error("[{}:{}:{}] {}", file, line, func, fmt::format(std::forward<Args>(args)...));
                }
                break;
            case Level::CRITICAL:
                if (static_cast<int>(level) >= static_cast<int>(s_ConsoleLevel)) {
                    s_Logger->critical("[{}:{}:{}] {}", file, line, func, fmt::format(std::forward<Args>(args)...));
                }
                if (static_cast<int>(level) >= static_cast<int>(s_FileLevel)) {
                    s_Logger->critical("[{}:{}:{}] {}", file, line, func, fmt::format(std::forward<Args>(args)...));
                }
                break;
            default:
                break;
            }
        }
    };

    // �궨�����־����
    #define LOG_TRACE(...)    Logger::Trace(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
    #define LOG_DEBUG(...)    Logger::Debug(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
    #define LOG_INFO(...)     Logger::Info(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
    #define LOG_WARN(...)     Logger::Warn(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
    #define LOG_ERROR(...)    Logger::Error(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
    #define LOG_CRITICAL(...) Logger::Critical(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
}