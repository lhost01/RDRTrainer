#include "pch.h"

namespace
{
	GodMode::Settings g_settings;
}

namespace GodMode
{
	Settings& Get() { return g_settings; }

	void LoadSettings()
	{
		g_settings.Enabled = Config::GetBool("GodMode.Enabled", g_settings.Enabled);
		g_settings.Hotkey = Config::GetInt("GodMode.Hotkey", g_settings.Hotkey);
		g_settings.FullHealth = Config::GetBool("GodMode.FullHealth", g_settings.FullHealth);
		g_settings.Unkillable = Config::GetBool("GodMode.Unkillable", g_settings.Unkillable);
		g_settings.Horse = Config::GetBool("GodMode.Horse", g_settings.Horse);
		g_settings.InfiniteDeadeye = Config::GetBool("GodMode.InfiniteDeadeye", g_settings.InfiniteDeadeye);
		g_settings.DeadeyePower = Config::GetBool("GodMode.DeadeyePower", g_settings.DeadeyePower);
	}

	void SaveSettings()
	{
		Config::SetBool("GodMode.Enabled", g_settings.Enabled);
		Config::SetInt("GodMode.Hotkey", g_settings.Hotkey);
		Config::SetBool("GodMode.FullHealth", g_settings.FullHealth);
		Config::SetBool("GodMode.Unkillable", g_settings.Unkillable);
		Config::SetBool("GodMode.Horse", g_settings.Horse);
		Config::SetBool("GodMode.InfiniteDeadeye", g_settings.InfiniteDeadeye);
		Config::SetBool("GodMode.DeadeyePower", g_settings.DeadeyePower);
	}

	void Apply(bool enabled)
	{
		const Actor player = Game::LocalPlayer();

		if (!ENTITY::IS_ACTOR_VALID(player))
			return;

		ACTOR::SET_ACTOR_INVULNERABILITY(player, enabled);

		if (g_settings.Unkillable)
			ACTOR::SET_ACTOR_UNKILLABLE(player, enabled);

		if (g_settings.InfiniteDeadeye)
			ACTOR::SET_INFINITE_DEADEYE(player, enabled);

		if (g_settings.DeadeyePower)
		{
			ACTOR::SET_DEADEYE_INVULNERABILITY(player, enabled);
			ACTOR::SET_DEADEYE_DAMAGE_SCALING(player, enabled ? 50.0f : 1.0f);
			ACTOR::SET_DEADEYE_REGENERATION_RATE(player, enabled ? 5.0f : 1.0f, true);
		}

		if (g_settings.Horse && RIDING::IS_ACTOR_RIDING(player))
		{
			const Actor mount = RIDING::GET_MOUNT(player);

			if (ENTITY::IS_ACTOR_VALID(mount))
			{
				ACTOR::SET_ACTOR_INVULNERABILITY(mount, enabled);
				ACTOR::SET_ACTOR_UNKILLABLE(mount, enabled);
			}
		}
	}

	void Update()
	{
		if (!g_settings.Enabled)
			return;

		if (!Game::IsLocalPlayerValid())
			return;

		const Actor player = Game::LocalPlayer();

		ACTOR::SET_ACTOR_INVULNERABILITY(player, true);

		if (g_settings.Unkillable)
			ACTOR::SET_ACTOR_UNKILLABLE(player, true);

		if (g_settings.FullHealth)
		{
			const float maxHealth = HEALTH::GET_ACTOR_MAX_HEALTH(player);

			if (maxHealth > 1.0f)
				HEALTH::SET_ACTOR_HEALTH(player, maxHealth);
		}

		if (g_settings.InfiniteDeadeye)
			ACTOR::SET_INFINITE_DEADEYE(player, true);

		if (g_settings.DeadeyePower)
		{
			ACTOR::SET_DEADEYE_INVULNERABILITY(player, true);
			ACTOR::SET_DEADEYE_DAMAGE_SCALING(player, 50.0f);
			ACTOR::SET_DEADEYE_REGENERATION_RATE(player, 5.0f, true);
		}

		if (g_settings.Horse && RIDING::IS_ACTOR_RIDING(player))
		{
			const Actor mount = RIDING::GET_MOUNT(player);

			if (ENTITY::IS_ACTOR_VALID(mount))
			{
				const float maxHealth = HEALTH::GET_ACTOR_MAX_HEALTH(mount);

				ACTOR::SET_ACTOR_INVULNERABILITY(mount, true);
				ACTOR::SET_ACTOR_UNKILLABLE(mount, true);

				if (maxHealth > 1.0f)
					HEALTH::SET_ACTOR_HEALTH(mount, maxHealth);
			}
		}
	}
}
