#pragma once

namespace Misc
{
	struct Settings
	{
		int   TeleportHotkey = KEY_F9;
		bool  TimeScaleEnabled = false;
		float TimeScale = 1.0f;
		float Honor = 1.0f;
		float Notoriety = 0.0f;
		bool  NoWanted = false;    // clear the wanted level every frame
		bool  MaxHonor = false;    // pin honor to maximum every frame
	};

	Settings& Get();
	void LoadSettings();
	void SaveSettings();
	void Update();

	bool TeleportToWaypoint();
	void GiveAllWeapons();
	void RefillHealth();
	void RefillDeadeye();
	void ApplyHonor(float honor);
	void ApplyNotoriety(float notoriety);
}
