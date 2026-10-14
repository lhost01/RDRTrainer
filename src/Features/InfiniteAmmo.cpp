#include "pch.h"

namespace
{
	InfiniteAmmo::Settings g_settings;
}

namespace InfiniteAmmo
{
	Settings& Get() { return g_settings; }

	void LoadSettings()
	{
		g_settings.Enabled = Config::GetBool("Ammo.Enabled", g_settings.Enabled);
		g_settings.Hotkey = Config::GetInt("Ammo.Hotkey", g_settings.Hotkey);
		g_settings.RefillClip = Config::GetBool("Ammo.RefillClip", g_settings.RefillClip);
		g_settings.NoReload = Config::GetBool("Ammo.NoReload", g_settings.NoReload);
		g_settings.MaxWeaponModel = Config::GetInt("Ammo.MaxWeaponModel", g_settings.MaxWeaponModel);
	}

	void SaveSettings()
	{
		Config::SetBool("Ammo.Enabled", g_settings.Enabled);
		Config::SetInt("Ammo.Hotkey", g_settings.Hotkey);
		Config::SetBool("Ammo.RefillClip", g_settings.RefillClip);
		Config::SetBool("Ammo.NoReload", g_settings.NoReload);
		Config::SetInt("Ammo.MaxWeaponModel", g_settings.MaxWeaponModel);
	}

	void Apply(bool enabled)
	{
		const Actor player = Game::LocalPlayer();

		if (!ENTITY::IS_ACTOR_VALID(player))
			return;

		// Only touch the weapon in hand. Looping a guessed 0..N weapon id range and
		// calling ACTOR_SET_INV_AMMO_INFINITE on ids that don't exist on this build
		// crashes the game (ACCESS_VIOLATION). Update() re-applies to the equipped
		// weapon, so switching weapons still keeps infinite ammo.
		const WeaponModel weapon = INVENTORY::GET_WEAPON_IN_HAND(player);

		if ((int)weapon > 0)
			INVENTORY::ACTOR_SET_INV_AMMO_INFINITE(player, weapon, enabled);

		REDHOOK::SET_PLAYER_RELOAD_TIME_SCALE(enabled && g_settings.NoReload, enabled && g_settings.NoReload ? 0.02f : 1.0f);
	}

	void GiveAllWeapons()
	{
		const Actor player = Game::LocalPlayer();

		if (!ENTITY::IS_ACTOR_VALID(player))
			return;

		for (int weapon = 0; weapon <= g_settings.MaxWeaponModel; ++weapon)
		{
			INVENTORY::GIVE_WEAPON_TO_ACTOR(player, (WeaponModel)weapon, 1000, false, false);
			INVENTORY::ACTOR_SET_INV_AMMO_INFINITE(player, (WeaponModel)weapon, true);
		}
	}

	void Update()
	{
		if (!g_settings.Enabled)
			return;

		if (!Game::IsLocalPlayerValid())
			return;

		const Actor player = Game::LocalPlayer();

		// Re-apply the infinite flag regularly: picking up or swapping weapons resets it.
		if (Game::Frame() % 30 == 0)
		{
			const WeaponModel weapon = INVENTORY::GET_WEAPON_IN_HAND(player);

			if ((int)weapon > 0)
			{
				INVENTORY::ACTOR_SET_INV_AMMO_INFINITE(player, weapon, true);

				// SET_INV_AMMO_INFINITE alone does not stop the reserve (inventory
				// ammo) from dropping on this build, so top it back up to max too.
				const AmmoType ammo = INVENTORY::GET_AMMOENUM_FOR_WEAPONENUM(weapon);
				const float maxReserve = INVENTORY::ACTOR_GET_INV_AMMO_MAX_AMOUNT(player, ammo);

				if (maxReserve > 0.0f)
				{
					const float reserve = INVENTORY::ACTOR_GET_INV_AMMO(player, ammo, false);

					if (reserve < maxReserve)
						INVENTORY::ACTOR_SET_INV_AMMO(player, ammo, maxReserve, false);
				}
			}
		}

		if (g_settings.NoReload)
			REDHOOK::SET_PLAYER_RELOAD_TIME_SCALE(true, 0.02f);

		if (g_settings.RefillClip)
		{
			const WeaponModel weapon = INVENTORY::GET_WEAPON_IN_HAND(player);

			if ((int)weapon >= 0 && (int)weapon <= g_settings.MaxWeaponModel)
			{
				const float maxAmmo = WEAPON::GET_WEAPON_MAX_AMMO(weapon);

				if (maxAmmo > 0.0f && INVENTORY::ACTOR_GET_WEAPON_AMMO(player, weapon) < maxAmmo)
					INVENTORY::ACTOR_SET_WEAPON_AMMO(player, weapon, maxAmmo);
			}
		}
	}
}
