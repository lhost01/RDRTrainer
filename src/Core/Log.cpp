#include "pch.h"

namespace
{
	std::mutex g_mutex;
	FILE* g_file = nullptr;

	void Write(const char* level, const char* format, va_list args)
	{
		char message[1024] = {};
		_vsnprintf_s(message, sizeof(message), _TRUNCATE, format, args);

		std::lock_guard<std::mutex> lock(g_mutex);

		char line[1200] = {};
		_snprintf_s(line, sizeof(line), _TRUNCATE, "[%8llu] [%-5s] %s\n",
			(unsigned long long)GetTickCount64(), level, message);

		if (g_file != nullptr)
		{
			fputs(line, g_file);
			fflush(g_file);
		}

		OutputDebugStringA(line);
	}
}

namespace Log
{
	void Init()
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		if (g_file != nullptr)
			return;

		const std::filesystem::path file = Utils::ModuleFile(L"RDRTrainer.log");

		if (_wfopen_s(&g_file, file.c_str(), L"wb") != 0)
			g_file = nullptr;
	}

	void Shutdown()
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		if (g_file != nullptr)
		{
			fclose(g_file);
			g_file = nullptr;
		}
	}

	void Info(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		Write("INFO", format, args);
		va_end(args);
	}

	void Warn(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		Write("WARN", format, args);
		va_end(args);
	}

	void Error(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		Write("ERROR", format, args);
		va_end(args);
	}
}
