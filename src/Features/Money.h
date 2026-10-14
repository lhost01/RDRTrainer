#pragma once

// Money lives in a script global (same layout as the PS3/360 builds: global index 3391).
// We resolve the global base once, verify it looks sane, and then read/write it directly.

namespace Money
{
	struct Settings
	{
		int  Hotkey = KEY_F6;
		int  QuickAmount = 10000;     // amount added by the hotkey
		int  SetAmount = 100000;      // amount used by the menu "set" entry
		int  GlobalIndex = 3391;      // script global index of the player money
		bool UseNative = false;       // also call UI_SET_MONEY (experimental)
		bool ShowOnHud = true;        // print the current money in the overlay
	};

	Settings& Get();
	void LoadSettings();
	void SaveSettings();

	void Update();
	void Add(int amount);
	void Set(int amount);
	int  Current();
	bool Available();
	const char* Status();
}
