#pragma once

namespace Esp
{
	struct Settings
	{
		bool  Enabled = false;
		int   Hotkey = KEY_F4;

		bool  Box = true;
		bool  Corners = false;
		bool  Name = true;
		bool  Distance = true;
		bool  Health = true;
		bool  Snapline = false;
		bool  HighlightTarget = true;
		bool  ShowDead = false;
		bool  ShowLocalPlayer = false;
		bool  ShowAnimals = false;
		bool  ShowFriendly = false;

		float MaxDistance = 250.0f;
		float LineThickness = 1.6f;
		float LabelScale = 1.0f;
		float HeadOffset = 1.78f;

		int   ColorHostile = 0;    // Theme::Presets index
		int   ColorAnimal = 3;
		int   ColorFriendly = 5;
		int   ColorTarget = 2;

		int   HostilityMode = 2;   // see Game::SetHostilityMode
		bool  ShowWatermark = true;
	};

	Settings& Get();
	void LoadSettings();
	void SaveSettings();
	void Draw();
}
