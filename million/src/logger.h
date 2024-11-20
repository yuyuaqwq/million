#pragma once

#include <million/service_handle.h>
#include <million/logger/logger.h>

namespace million {
class Million;

class Logger {
public:
	Logger(Million* million);
	~Logger();

	bool Init();
	void Log(const ServiceHandle& sender, logger::LogLevel level, const char* file, int line, const char* function, const std::string& str);
	void SetLevel(const ServiceHandle& sender, const std::string& level);

private:
	Million* million_;
	ServiceHandle logger_handle_;
};

} // namespace million
