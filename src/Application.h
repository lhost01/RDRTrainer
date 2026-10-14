#pragma once

class Application
{
public:
	static void Initialize(HMODULE module);
	static void Shutdown();

	static void Frame();

	// config plumbing shared by the menu and the features
	static void RequestSave();
	static void SaveConfig();
	static void ReloadConfig();
	static void ApplyMenuSettings();

	static int  Fps();
};
