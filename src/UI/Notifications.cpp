#include "pch.h"

namespace
{
	struct Toast
	{
		std::string text;
		uint64_t    expires;
		Color       color;
	};

	std::deque<Toast> g_toasts;
	int  g_language = 0;
	bool g_gameHudMessages = true;

	constexpr size_t kMaxToasts = 6;
	constexpr uint64_t kDurationMs = 2600;
}

namespace Notifications
{
	void SetLanguage(int language) { g_language = Utils::ClampInt(language, 0, 1); }
	int  Language() { return g_language; }
	void SetGameHudMessages(bool enabled) { g_gameHudMessages = enabled; }
	bool GameHudMessages() { return g_gameHudMessages; }

	void Info(const std::string& text)
	{
		g_toasts.push_back({ text, GetTickCount64() + kDurationMs, Theme::ItemText });

		while (g_toasts.size() > kMaxToasts)
			g_toasts.pop_front();
	}

	void Warn(const std::string& text)
	{
		g_toasts.push_back({ text, GetTickCount64() + kDurationMs + 900, Theme::ValueText });

		while (g_toasts.size() > kMaxToasts)
			g_toasts.pop_front();
	}

	void Toggle(const std::string& featureName, const std::string& stateZh, const std::string& stateEn)
	{
		const std::string state = (g_language == 0) ? stateZh : stateEn;
		const std::string message = Utils::Format("%s %s", featureName.c_str(), state.c_str());

		Info(message);

		if (g_gameHudMessages)
		{
			// The game may keep the pointer, so hand it a buffer that stays alive.
			static char buffer[256] = {};
			_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%s <green>%s", featureName.c_str(), state.c_str());

			HUD::PRINT_SMALL_B(buffer, 1.5f, true, 0, 0, 0, 0);
		}
	}

	void Update()
	{
		const uint64_t now = GetTickCount64();

		while (!g_toasts.empty() && g_toasts.front().expires < now)
			g_toasts.pop_front();
	}

	void Draw()
	{
		if (g_toasts.empty() || !Screen::IsValid())
			return;

		const float rowHeight = 22.0f;
		const float margin = 18.0f;
		float y = Screen::Height() - margin - rowHeight * (float)g_toasts.size();
		const uint64_t now = GetTickCount64();

		for (const Toast& toast : g_toasts)
		{
			// Fade out during the last 600 ms.
			const float remaining = (float)(toast.expires - now);
			const float alpha = Utils::Clamp(remaining / 600.0f, 0.0f, 1.0f);

			Render::Text(toast.text, margin, y, 1.0f, toast.color.WithAlpha((int)(255.0f * alpha)), 0, true);

			y += rowHeight;
		}
	}

	void Shutdown()
	{
		g_toasts.clear();
	}
}
