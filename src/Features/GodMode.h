#pragma once

namespace GodMode
{
	struct Settings
	{
		bool Enabled = false;
		int  Hotkey = KEY_F1;

		bool FullHealth = true;        // keep the player at full health every frame
		bool Unkillable = true;        // SET_ACTOR_UNKILLABLE / SET_TOUGH_ACTOR
		bool Horse = true;             // also protect the current mount
		bool InfiniteDeadeye = false;  // never run out of deadeye
		bool DeadeyePower = false;     // deadeye damage / regeneration boosts
	};

	Settings& Get();
	void LoadSettings();
	void SaveSettings();
	void Update();
	void Apply(bool enabled);
}
