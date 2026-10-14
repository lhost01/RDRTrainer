#pragma once

namespace InfiniteAmmo
{
	struct Settings
	{
		bool Enabled = false;
		int  Hotkey = KEY_F2;

		bool RefillClip = true;    // keep the equipped weapon topped up
		bool NoReload = false;     // near instant reloads
		int  MaxWeaponModel = 50;  // how many weapon ids get the infinite flag
	};

	Settings& Get();
	void LoadSettings();
	void SaveSettings();
	void Update();
	void Apply(bool enabled);
	void GiveAllWeapons();
}
