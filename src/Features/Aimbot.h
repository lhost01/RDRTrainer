#pragma once

namespace Aimbot
{
	enum Mode
	{
		Mode_Assist = 0,   // native lock-on helpers + perfect accuracy + auto fire (safest)
		Mode_Mouse = 1,    // moves the real mouse cursor towards the target
		Mode_Native = 2    // experimental AI_GOAL_AIM_AT_OBJECT every frame
	};

	enum Priority
	{
		Priority_Crosshair = 0,
		Priority_Distance = 1,
		Priority_Health = 2
	};

	struct Settings
	{
		bool  Enabled = false;
		int   Hotkey = KEY_F3;

		int   Trigger = 0;            // 0 = only while AimKey is held, 1 = always
		int   AimKey = Utils::MouseKeyBase + 1;   // right mouse button
		int   Mode = Mode_Mouse;
		int   TargetPriority = Priority_Crosshair;

		float FovPixels = 220.0f;     // radius around the crosshair, in pixels
		float MaxDistance = 120.0f;   // meters
		bool  HostileOnly = false;
		bool  RequireLineOfSight = false;
		bool  IgnoreAnimals = true;

		bool  AutoFire = false;
		bool  PerfectAccuracy = true;
		bool  AimAssistFlag = true;
		bool  HardlockBias = false;   // experimental
		bool  TurnBody = false;       // also rotate the player towards the target

		float MouseSmooth = 0.25f;    // mouse mode: fraction of the error applied per frame (lower = smoother)
		float MouseMaxStep = 40.0f;   // mouse mode: pixel clamp per frame
		bool  DrawFov = true;         // draw the aimbot FOV circle
	};

	Settings& Get();
	void LoadSettings();
	void SaveSettings();
	void Update();

	Actor CurrentTarget();
	void  Reset();
}
