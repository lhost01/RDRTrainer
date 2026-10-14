#pragma once

namespace Notifications
{
	// Toast in the corner of our overlay and (optionally) a native game HUD message.
	void Info(const std::string& text);
	void Toggle(const std::string& featureName, const std::string& stateZh, const std::string& stateEn);
	void Warn(const std::string& text);

	void SetLanguage(int language);   // 0 = Chinese, 1 = English
	int  Language();

	void SetGameHudMessages(bool enabled);
	bool GameHudMessages();

	void Update();
	void Draw();
	void Shutdown();
}
