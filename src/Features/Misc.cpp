#include "pch.h"

namespace
{
	Misc::Settings g_settings;

	bool  g_teleportActive = false;
	int   g_teleportAttempts = 0;
	float g_teleportHeight = 0.0f;
	Vector3 g_teleportTarget{};
	uint64_t g_lastTeleportStep = 0;

	constexpr int   kMaxTeleportAttempts = 40;
	constexpr float kTeleportStartHeight = 360.0f;
	constexpr float kTeleportStepDown = 5.0f;
}

namespace Misc
{
	Settings& Get() { return g_settings; }

	void LoadSettings()
	{
		g_settings.TeleportHotkey = Config::GetInt("Misc.TeleportHotkey", g_settings.TeleportHotkey);
		g_settings.TimeScaleEnabled = Config::GetBool("Misc.TimeScaleEnabled", g_settings.TimeScaleEnabled);
		g_settings.TimeScale = Config::GetFloat("Misc.TimeScale", g_settings.TimeScale);
		g_settings.Honor = Config::GetFloat("Misc.Honor", g_settings.Honor);
		g_settings.Notoriety = Config::GetFloat("Misc.Notoriety", g_settings.Notoriety);
		g_settings.NoWanted = Config::GetBool("Misc.NoWanted", g_settings.NoWanted);
		g_settings.MaxHonor = Config::GetBool("Misc.MaxHonor", g_settings.MaxHonor);
	}

	void SaveSettings()
	{
		Config::SetInt("Misc.TeleportHotkey", g_settings.TeleportHotkey);
		Config::SetBool("Misc.TimeScaleEnabled", g_settings.TimeScaleEnabled);
		Config::SetFloat("Misc.TimeScale", g_settings.TimeScale);
		Config::SetFloat("Misc.Honor", g_settings.Honor);
		Config::SetFloat("Misc.Notoriety", g_settings.Notoriety);
		Config::SetBool("Misc.NoWanted", g_settings.NoWanted);
		Config::SetBool("Misc.MaxHonor", g_settings.MaxHonor);
	}

	bool TeleportToWaypoint()
	{
		if (!Game::IsLocalPlayerValid())
			return false;

		Vector3 waypoint = {};
		HUD::GET_USER_DEFINED_WAYPOINT(&waypoint);

		if (waypoint.x == 0.0f && waypoint.z == 0.0f)
		{
			Notifications::Warn("地图上没有标记点 / no map waypoint set");
			return false;
		}

		g_teleportTarget = waypoint;
		g_teleportHeight = kTeleportStartHeight;
		g_teleportTarget.y = g_teleportHeight;
		g_teleportAttempts = 0;
		g_teleportActive = true;
		g_lastTeleportStep = 0;

		ACTOR::TELEPORT_ACTOR(Game::LocalPlayer(), &g_teleportTarget, true, true, true);

		Notifications::Info("传送中 / teleporting...");
		return true;
	}

	void GiveAllWeapons()
	{
		const Actor player = Game::LocalPlayer();

		if (!ENTITY::IS_ACTOR_VALID(player))
			return;

		// Only top up the weapon in hand. Blindly looping a guessed 0..N weapon id
		// range crashes on this build (invalid ids cause ACCESS_VIOLATION in the
		// inventory native), same root cause as the F2 crash.
		const WeaponModel weapon = INVENTORY::GET_WEAPON_IN_HAND(player);

		if ((int)weapon > 0)
		{
			INVENTORY::ACTOR_SET_INV_AMMO_INFINITE(player, weapon, true);

			const float maxAmmo = WEAPON::GET_WEAPON_MAX_AMMO(weapon);

			if (maxAmmo > 0.0f)
				INVENTORY::ACTOR_SET_WEAPON_AMMO(player, weapon, maxAmmo);
		}

		Notifications::Info("已补满当前武器 / current weapon topped up");
	}

	void RefillHealth()
	{
		const Actor player = Game::LocalPlayer();

		if (!ENTITY::IS_ACTOR_VALID(player))
			return;

		const float maxHealth = HEALTH::GET_ACTOR_MAX_HEALTH(player);

		if (maxHealth > 1.0f)
			HEALTH::SET_ACTOR_HEALTH(player, maxHealth);

		Notifications::Info("血量已补满 / health refilled");
	}

	void RefillDeadeye()
	{
		const Actor player = Game::LocalPlayer();

		if (!ENTITY::IS_ACTOR_VALID(player))
			return;

		ACTOR::SET_PLAYER_DEADEYE_POINTS(player, 100.0f, true);
		Notifications::Info("死神之眼已补满 / deadeye refilled");
	}

	void ApplyHonor(float honor)
	{
		ACTOR::SET_PLAYER_CURRENT_HONOR(Utils::Clamp(honor, -1.0f, 1.0f));
		Notifications::Info(Utils::Format("荣誉值 -> %.2f", honor));
	}

	void ApplyNotoriety(float notoriety)
	{
		ACTOR::SET_PLAYER_CURRENT_NOTORIETY(Utils::Clamp(notoriety, 0.0f, 1.0f));
		Notifications::Info(Utils::Format("恶名值 -> %.2f", notoriety));
	}

	void Update()
	{
		if (g_settings.TimeScaleEnabled)
			REDHOOK::SET_TIME_SCALE(Utils::Clamp(g_settings.TimeScale, 0.0f, 5.0f));

		static bool     s_wantedEdge = false;
		static uint64_t s_wantedLast = 0;

		if (g_settings.NoWanted && Game::IsLocalPlayerValid())
		{
			const uint64_t now = GetTickCount64();

			// Fire immediately when the switch is turned ON (clears an ACTIVE wanted
			// level right away), then re-apply every 500 ms.
			if (!s_wantedEdge || now - s_wantedLast >= 500)
			{
				s_wantedEdge = true;
				s_wantedLast = now;

				const Actor player = Game::LocalPlayer();

				// Make every AI (lawmen included) forget the player: posses lose
				// their target and stop chasing. Classic clear-wanted combo.
				AI_MEMORY::MEMORY_EVERYBODY_FORGET_ABOUT(player);
				AI_MEMORY::AI_GLOBAL_CLEAR_ALL_DANGER();

				// No active crimes, no notoriety -> nothing new to spawn law for.
				ENTITY::SET_ACTOR_TIME_OF_LAST_CRIME(player, 0);
				ACTOR::SET_PLAYER_CURRENT_NOTORIETY(0.0f);

				// Hide the on-screen wanted stars (visual layer).
				HUD::WANTEDFX_SET_LEVEL(0.0f);
				HUD::WANTEDFX_ENABLED(false);
			}
		}
		else
		{
			// Re-arm the edge trigger so turning it on again clears immediately.
			s_wantedEdge = false;
		}

		if (g_settings.MaxHonor && Game::IsLocalPlayerValid())
		{
			// Honor in RDR1 is a 0..100 scale internally (not -1..1); 100 is maximum
			// honor. Applying every frame keeps it pinned no matter what you do.
			ACTOR::SET_PLAYER_CURRENT_HONOR(100.0f);
		}

		if (!g_teleportActive)
			return;

		if (!Game::IsLocalPlayerValid())
		{
			g_teleportActive = false;
			return;
		}

		const uint64_t now = GetTickCount64();

		if (now - g_lastTeleportStep < 60)
			return;

		g_lastTeleportStep = now;

		const Actor player = Game::LocalPlayer();

		if (ENTITY::IS_ACTOR_ON_GROUND(player) || g_teleportAttempts >= kMaxTeleportAttempts)
		{
			g_teleportActive = false;
			Notifications::Info("传送完成 / teleport finished");
			return;
		}

		++g_teleportAttempts;
		g_teleportHeight -= kTeleportStepDown;
		g_teleportTarget.y = g_teleportHeight;

		ACTOR::TELEPORT_ACTOR(player, &g_teleportTarget, true, true, true);
	}
}
