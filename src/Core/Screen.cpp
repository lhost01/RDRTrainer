#include "pch.h"

namespace
{
	HWND  g_window = nullptr;
	float g_width = 1920.0f;
	float g_height = 1080.0f;
	uint64_t g_lastUpdate = 0;

	struct WindowSearch
	{
		DWORD processId;
		HWND  best;
		long  bestArea;
	};

	BOOL CALLBACK EnumWindowsProc(HWND window, LPARAM parameter)
	{
		WindowSearch* search = (WindowSearch*)parameter;

		DWORD processId = 0;
		GetWindowThreadProcessId(window, &processId);

		if (processId != search->processId || !IsWindowVisible(window))
			return TRUE;

		RECT rect = {};

		if (!GetClientRect(window, &rect))
			return TRUE;

		const long area = (rect.right - rect.left) * (rect.bottom - rect.top);

		if (area > search->bestArea)
		{
			search->bestArea = area;
			search->best = window;
		}

		return TRUE;
	}
}

namespace Screen
{
	void Update()
	{
		const uint64_t now = GetTickCount64();

		if (now - g_lastUpdate < 2000 && g_window != nullptr && IsWindow(g_window))
			return;

		g_lastUpdate = now;

		if (g_window == nullptr || !IsWindow(g_window))
		{
			WindowSearch search = {};
			search.processId = GetCurrentProcessId();
			EnumWindows(EnumWindowsProc, (LPARAM)&search);
			g_window = search.best;
		}

		if (g_window == nullptr)
			return;

		RECT rect = {};

		if (!GetClientRect(g_window, &rect))
			return;

		const float width = (float)(rect.right - rect.left);
		const float height = (float)(rect.bottom - rect.top);

		if (width >= 100.0f && height >= 100.0f)
		{
			g_width = width;
			g_height = height;
		}
	}

	HWND  Window() { return g_window; }
	float Width() { return g_width; }
	float Height() { return g_height; }
	float Aspect() { return g_height > 0.0f ? g_width / g_height : 1.0f; }
	bool  IsValid() { return g_window != nullptr && IsWindow(g_window); }

	float ToPixelX(float normalizedX) { return normalizedX * g_width; }
	float ToPixelY(float normalizedY) { return normalizedY * g_height; }
	float ToNormalizedX(float pixelX) { return g_width > 0.0f ? pixelX / g_width : 0.0f; }
	float ToNormalizedY(float pixelY) { return g_height > 0.0f ? pixelY / g_height : 0.0f; }
}
