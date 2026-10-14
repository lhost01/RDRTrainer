#pragma once

namespace Log
{
	void Init();
	void Shutdown();

	void Info(const char* format, ...);
	void Warn(const char* format, ...);
	void Error(const char* format, ...);
}
