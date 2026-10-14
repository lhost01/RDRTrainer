#include "pch.h"

namespace
{
	Aimbot::Settings g_settings;
	Actor g_target = 0;
	bool g_autoFireApplied = false;

	// Per-target / per-toggle caches so we don't fire the same native every frame.
	// Repeated redundant native calls correlate with the ACCESS_VIOLATION crash.
	Actor g_lastAimAssistTarget = 0;
	Actor g_lastHardlockTarget = 0;
	bool g_perfectAccuracyApplied = false;

	float ScreenDistancePixels(const ActorEntry& entry)
	{
		const float centerX = Screen::Width() * 0.5f;
		const float centerY = Screen::Height() * 0.5f;
		const float x = entry.screenX * Screen::Width();
		const float y = entry.screenY * Screen::Height();
		const float dx = x - centerX;
		const float dy = y - centerY;

		return sqrtf(dx * dx + dy * dy);
	}

	bool IsValidTarget(const ActorEntry& entry)
	{
		if (!entry.valid || entry.dead || entry.isLocalPlayer)
			return false;

		if (entry.category == ActorCategory::Vehicle)
			return false;

		if (entry.category == ActorCategory::Animal && g_settings.IgnoreAnimals)
			return false;

		if (g_settings.HostileOnly && entry.category != ActorCategory::Hostile)
			return false;

		if (entry.distance > g_settings.MaxDistance)
			return false;

		// Aim at whatever is visibly aimingable: the head point must be a real
		// projection. An enemy poking their head over cover has headValid=true even
		// though the body is hidden - exactly the shot we want to take.
		if (!entry.headValid)
			return false;

		return ScreenDistancePixels(entry) <= g_settings.FovPixels;
	}

	bool FindTarget(Actor& outTarget)
	{
		// Target stickiness: while the current target is still valid and stays within
		// 1.8x the FOV radius we keep aiming at it. With several enemies inside the
		// circle this prevents the view from flickering between them every frame.
		const ActorEntry* current = Game::FindActor(g_target);

		if (current != nullptr && IsValidTarget(*current)
			&& ScreenDistancePixels(*current) <= g_settings.FovPixels * 1.8f)
		{
			outTarget = g_target;
			return true;
		}

		const ActorEntry* best = nullptr;
		float bestScore = 3.4e38f;

		for (const ActorEntry& entry : Game::Actors())
		{
			if (!IsValidTarget(entry))
				continue;

			float score = 0.0f;

			switch (g_settings.TargetPriority)
			{
				case Aimbot::Priority_Distance:
					score = entry.distance;
					break;

				case Aimbot::Priority_Health:
					score = entry.health;
					break;

				case Aimbot::Priority_Crosshair:
				default:
					score = ScreenDistancePixels(entry);
					break;
			}

			if (score >= bestScore)
				continue;

			if (g_settings.RequireLineOfSight)
			{
				const Vector3 eye = Game::LocalPlayerPosition() + Vector3(0.0f, 1.6f, 0.0f);

				if (!Game::HasLineOfSight(eye, entry.head, Game::LocalPlayer()))
					continue;
			}

			best = &entry;
			bestScore = score;
		}

		if (best == nullptr)
			return false;

		outTarget = best->actor;
		return true;
	}

	void ApplyAimAssist(Actor target)
	{
		const Player player = (Player)Game::LocalSlot();

		// Cache per-target / per-toggle state so we don't fire the same native every
		// frame. Repeated redundant native calls correlate with the ACCESS_VIOLATION crash.
		if (g_settings.PerfectAccuracy && !g_perfectAccuracyApplied)
		{
			TARGETING::SET_PLAYER_PERFECT_ACCURACY(player, true);
			g_perfectAccuracyApplied = true;
		}

		if (g_settings.AimAssistFlag && target != g_lastAimAssistTarget)
		{
			TARGETING::SET_ACTOR_CAN_BE_AIMASSISTED(target, 1);
			g_lastAimAssistTarget = target;
		}

		if (g_settings.HardlockBias && target != g_lastHardlockTarget)
		{
			TARGETING::SET_ACTOR_CAN_BE_HARDLOCKED(target, 1);
			g_lastHardlockTarget = target;
		}

		if (g_settings.TurnBody)
		{
			const Vector3 playerPosition = Game::LocalPlayerPosition();
			const Vector3 targetPosition = ACTOR::GET_POSITION(target);
			const float dx = targetPosition.x - playerPosition.x;
			const float dz = targetPosition.z - playerPosition.z;
			const float heading = atan2f(dx, dz) * Math<float>::RadToDeg;

			ACTOR::SET_ACTOR_HEADING(Game::LocalPlayer(), Utils::NormalizeHeading(heading), false);
		}
	}

	void ApplyMouseAim()
	{
		const ActorEntry* entry = Game::FindActor(g_target);

		if (entry == nullptr || !entry->onScreen)
			return;

		if (GetForegroundWindow() != Screen::Window())
			return;

		// The crosshair is ALWAYS at the center of the screen in this game - the
		// Windows cursor position has nothing to do with where you are aiming.
		// The old code compared the target against the cursor, which made the
		// aim drift off target and jitter.
		const float crosshairX = Screen::Width() * 0.5f;
		const float crosshairY = Screen::Height() * 0.5f;

		const float aimX = entry->headScreenX * Screen::Width();
		const float aimY = entry->headScreenY * Screen::Height();
		float dx = aimX - crosshairX;
		float dy = aimY - crosshairY;

		// Dead zone: stop moving once we are close enough. Without this the mouse
		// keeps micro-correcting around the head every frame and the view shakes.
		if (fabsf(dx) < 4.0f && fabsf(dy) < 4.0f)
			return;

		// Frame-rate independent smoothing: at 30 FPS one tick must move as far
		// as two ticks at 60 FPS, otherwise the aim speed varies with FPS.
		static uint64_t lastTick = 0;
		const uint64_t now = GetTickCount64();
		const float elapsed = (lastTick == 0) ? 16.0f : Utils::Clamp((float)(now - lastTick), 1.0f, 100.0f);
		lastTick = now;

		const float smooth = Utils::Clamp(g_settings.MouseSmooth, 0.02f, 1.0f) * (elapsed / 16.6f);
		const float maxStep = Utils::Clamp(g_settings.MouseMaxStep, 1.0f, 400.0f) * (elapsed / 16.6f);

		dx = Utils::Clamp(dx * smooth, -maxStep, maxStep);
		dy = Utils::Clamp(dy * smooth, -maxStep, maxStep);

		Input::MoveMouseRelative(dx, dy);
	}

	void ApplyAutoFire(bool enabled)
	{
		if (g_autoFireApplied == enabled)
			return;

		REDHOOK::SET_PLAYER_AUTO_FIRING(enabled);
		g_autoFireApplied = enabled;
	}
}

namespace Aimbot
{
	Settings& Get() { return g_settings; }

	void LoadSettings()
	{
		g_settings.Enabled = Config::GetBool("Aimbot.Enabled", g_settings.Enabled);
		g_settings.Hotkey = Config::GetInt("Aimbot.Hotkey", g_settings.Hotkey);
		g_settings.Trigger = Config::GetInt("Aimbot.Trigger", g_settings.Trigger);
		g_settings.AimKey = Config::GetInt("Aimbot.AimKey", g_settings.AimKey);
		g_settings.Mode = Config::GetInt("Aimbot.Mode", g_settings.Mode);
		g_settings.TargetPriority = Config::GetInt("Aimbot.Priority", g_settings.TargetPriority);
		g_settings.FovPixels = Config::GetFloat("Aimbot.FovPixels", g_settings.FovPixels);
		g_settings.MaxDistance = Config::GetFloat("Aimbot.MaxDistance", g_settings.MaxDistance);
		g_settings.HostileOnly = Config::GetBool("Aimbot.HostileOnly", g_settings.HostileOnly);
		g_settings.RequireLineOfSight = Config::GetBool("Aimbot.RequireLineOfSight", g_settings.RequireLineOfSight);
		g_settings.IgnoreAnimals = Config::GetBool("Aimbot.IgnoreAnimals", g_settings.IgnoreAnimals);
		g_settings.AutoFire = Config::GetBool("Aimbot.AutoFire", g_settings.AutoFire);
		g_settings.PerfectAccuracy = Config::GetBool("Aimbot.PerfectAccuracy", g_settings.PerfectAccuracy);
		g_settings.AimAssistFlag = Config::GetBool("Aimbot.AimAssistFlag", g_settings.AimAssistFlag);
		g_settings.HardlockBias = Config::GetBool("Aimbot.HardlockBias", g_settings.HardlockBias);
		g_settings.TurnBody = Config::GetBool("Aimbot.TurnBody", g_settings.TurnBody);
		g_settings.MouseSmooth = Config::GetFloat("Aimbot.MouseSmooth", g_settings.MouseSmooth);
		g_settings.MouseMaxStep = Config::GetFloat("Aimbot.MouseMaxStep", g_settings.MouseMaxStep);
		g_settings.DrawFov = Config::GetBool("Aimbot.DrawFov", g_settings.DrawFov);
	}

	void SaveSettings()
	{
		Config::SetBool("Aimbot.Enabled", g_settings.Enabled);
		Config::SetInt("Aimbot.Hotkey", g_settings.Hotkey);
		Config::SetInt("Aimbot.Trigger", g_settings.Trigger);
		Config::SetInt("Aimbot.AimKey", g_settings.AimKey);
		Config::SetInt("Aimbot.Mode", g_settings.Mode);
		Config::SetInt("Aimbot.Priority", g_settings.TargetPriority);
		Config::SetFloat("Aimbot.FovPixels", g_settings.FovPixels);
		Config::SetFloat("Aimbot.MaxDistance", g_settings.MaxDistance);
		Config::SetBool("Aimbot.HostileOnly", g_settings.HostileOnly);
		Config::SetBool("Aimbot.RequireLineOfSight", g_settings.RequireLineOfSight);
		Config::SetBool("Aimbot.IgnoreAnimals", g_settings.IgnoreAnimals);
		Config::SetBool("Aimbot.AutoFire", g_settings.AutoFire);
		Config::SetBool("Aimbot.PerfectAccuracy", g_settings.PerfectAccuracy);
		Config::SetBool("Aimbot.AimAssistFlag", g_settings.AimAssistFlag);
		Config::SetBool("Aimbot.HardlockBias", g_settings.HardlockBias);
		Config::SetBool("Aimbot.TurnBody", g_settings.TurnBody);
		Config::SetFloat("Aimbot.MouseSmooth", g_settings.MouseSmooth);
		Config::SetFloat("Aimbot.MouseMaxStep", g_settings.MouseMaxStep);
		Config::SetBool("Aimbot.DrawFov", g_settings.DrawFov);
	}

	void Reset()
	{
		g_target = 0;
		Game::SetAimbotTarget(0);
		ApplyAutoFire(false);

		g_lastAimAssistTarget = 0;
		g_lastHardlockTarget = 0;
		g_perfectAccuracyApplied = false;
	}

	void Update()
	{
		if (!g_settings.Enabled)
		{
			if (g_target != 0)
				Reset();

			return;
		}

		// Don't fight the user for the mouse while the trainer menu is open.
		if (Menu::IsOpen())
		{
			Reset();
			return;
		}

		if (!Game::IsLocalPlayerValid())
		{
			Reset();
			return;
		}

		const bool active = (g_settings.Trigger == 1) || Input::Down(g_settings.AimKey);

		if (!active)
		{
			Reset();
			return;
		}

		Actor target = 0;

		if (!FindTarget(target))
		{
			Reset();
			return;
		}

		g_target = target;
		Game::SetAimbotTarget(target);

		ApplyAimAssist(target);

		if (g_settings.Mode == Mode_Mouse)
			ApplyMouseAim();
		else if (g_settings.Mode == Mode_Native)
			AI_MISC::AI_GOAL_AIM_AT_OBJECT(Game::LocalPlayer(), target, 0);

		ApplyAutoFire(g_settings.AutoFire);
	}

	Actor CurrentTarget()
	{
		return g_target;
	}
}
