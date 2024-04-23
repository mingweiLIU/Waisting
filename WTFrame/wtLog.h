#ifndef WTLOG_H
#define WTLOG_H
#include "wtFrameDefines.h"
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/details/thread_pool-inl.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/async.h> //support for async logging

WTNAMESPACESTART
FRAMENAMESPACESTART

//使用方法
//int param = 1;
//XLOG_TRACE("this is trace log record, param: {}", ++param); // int type param is ok
//XLOG_DEBUG("this is debug log record, param: {}", ++param);
//XLOG_INFO("this is info log record, param: {}", ++param);
//XLOG_WARN("this is warn log record, param: {}", double(++param)); // double type param is ok
//XLOG_ERROR("this is error log record, param: {}", std::to_string(++param)); // string type param is ok

class WTFRAMEAPI wtLog
{
public:
    static wtLog* getInstance();
    std::shared_ptr<spdlog::logger> getLogger();
private:
    wtLog();
    ~wtLog();
    wtLog(const wtLog&) = delete;
    wtLog& operator=(const wtLog&) = delete;

    std::shared_ptr<spdlog::logger> m_logger;
};

#define wtLOG_TRACE(...) SPDLOG_LOGGER_CALL(WT::Frame::wtLog::getInstance()->getLogger().get(), spdlog::level::trace, __VA_ARGS__)
#define wtLOG_DEBUG(...) SPDLOG_LOGGER_CALL(WT::Frame::wtLog::getInstance()->getLogger().get(), spdlog::level::debug, __VA_ARGS__)
#define wtLOG_INFO(...) SPDLOG_LOGGER_CALL(WT::Frame::wtLog::getInstance()->getLogger().get(), spdlog::level::info, __VA_ARGS__)
#define wtLOG_WARN(...) SPDLOG_LOGGER_CALL(WT::Frame::wtLog::getInstance()->getLogger().get(), spdlog::level::warn, __VA_ARGS__)
#define wtLOG_ERROR(...) SPDLOG_LOGGER_CALL(WT::Frame::wtLog::getInstance()->getLogger().get(), spdlog::level::err, __VA_ARGS__)

FRAMENAMESPACEEND
WTNAMESPACEEND
#endif // WTLOG_H
