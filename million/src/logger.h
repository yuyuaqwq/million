#pragma once

#include <million/service_handle.h>
#include <million/logger/logger.h>

namespace million {
class Million;

class Logger {
public:
	Logger(Million* million);
	~Logger();

	void Init();
	void Log(ServiceHandle sender, logger::LogLevel level, const char* file, int line, const char* function, std::string_view str);
	void SetLevel(ServiceHandle sender, std::string_view level);

private:
	Million* million_;
	ServiceHandle logger_handle_;
};

} // namespace million
