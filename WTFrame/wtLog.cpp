#include "wtLog.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
using namespace WT::Frame;
wtLog* wtLog::getInstance()
{
	static wtLog wtLogger;
	return &wtLogger;
}

std::shared_ptr<spdlog::logger> wtLog::getLogger()
{
	return m_logger;
}

wtLog::wtLog()
{
	const std::string logDir = "./log";
	const std::string loggerNamePrefix = "wt_";
	bool console = false;
	std::string level = "debug";
	try
	{
		auto now = std::chrono::system_clock::now();
		std::time_t now_time = std::chrono::system_clock::to_time_t(now- std::chrono::hours(24));
		std::stringstream ss;
		ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %X");
		std::string logger_name = ss.str();
		if (console)
		{
			m_logger = spdlog::stdout_color_st(logger_name);
		}
		else
		{
			m_logger= spdlog::create_async<spdlog::sinks::rotating_file_sink_mt>(logger_name, logDir + "/" + logger_name + ".log", 500 * 1024 * 1024, 1000);
		}

		m_logger->set_pattern("%Y-%m-%d %H:%M:%S.%f <thread %t> [%l] [%@] %v"); // with timestamp, thread_id, filename and line number

		if (level == "trace")
		{
			m_logger->set_level(spdlog::level::trace);
			m_logger->flush_on(spdlog::level::trace);
		}
		else if (level == "debug")
		{
			m_logger->set_level(spdlog::level::debug);
			m_logger->flush_on(spdlog::level::debug);
		}
		else if (level == "info")
		{
			m_logger->set_level(spdlog::level::info);
			m_logger->flush_on(spdlog::level::info);
		}
		else if (level == "warn")
		{
			m_logger->set_level(spdlog::level::warn);
			m_logger->flush_on(spdlog::level::warn);
		}
		else if (level == "error")
		{
			m_logger->set_level(spdlog::level::err);
			m_logger->flush_on(spdlog::level::err);
		}
	}
	catch (const spdlog::spdlog_ex& ex)
	{
		std::cout << "Log initialization failed: " << ex.what() << std::endl;
	}
}

wtLog::~wtLog()
{
	spdlog::drop_all(); // must do this
}
