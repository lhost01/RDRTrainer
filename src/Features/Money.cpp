#include "pch.h"

namespace
{
	Money::Settings g_settings;

	uintptr_t g_base = 0;
	uintptr_t g_address = 0;
	int       g_value = 0;
	bool      g_available = false;
	bool      g_baseIsPageTable = false;
	int       g_pageCount = 0;
	std::string g_status = "未初始化 / not resolved";

	uintptr_t ResolveAddress()
	{
		const uintptr_t base = GetGlobalPtr();

		if (base == 0)
		{
			g_available = false;
			g_status = "GetGlobalPtr() == 0";
			return 0;
		}

		if (base != g_base)
		{
			g_base = base;

			// Detect whether RedHook handed us the page table (array of page pointers)
			// or the first global page itself.
			int readable = 0;

			if (Utils::IsReadable((const void*)base, 64 * sizeof(uintptr_t)))
			{
				const uintptr_t* pages = (const uintptr_t*)base;

				for (int index = 0; index < 64; ++index)
				{
					if (pages[index] != 0 && Utils::IsReadable((const void*)pages[index], 0x1000))
						++readable;
				}
			}

			g_pageCount = readable;
			g_baseIsPageTable = readable >= 8;

			Log::Info("Money: globals base = 0x%p, readable page pointers = %d -> %s",
				(void*)base, readable, g_baseIsPageTable ? "page table" : "single page");
		}

		const int globalIndex = Utils::ClampInt(g_settings.GlobalIndex, 0, 0x1000000);
		const uintptr_t page = (uintptr_t)globalIndex / 0x40000;
		const uintptr_t offset = (uintptr_t)globalIndex % 0x40000;

		uintptr_t address = 0;

		if (g_baseIsPageTable)
		{
			if (!Utils::IsReadable((const void*)(base + page * sizeof(uintptr_t)), sizeof(uintptr_t)))
			{
				g_available = false;
				g_status = "page table not readable";
				return 0;
			}

			const uintptr_t pagePointer = ((const uintptr_t*)base)[page];
			address = pagePointer + offset * sizeof(uintptr_t);
		}
		else
		{
			address = base + offset * sizeof(uintptr_t);
		}

		if (!Utils::IsReadable((const void*)address, sizeof(int32_t)))
		{
			g_available = false;
			g_status = "money address not readable";
			return 0;
		}

		return address;
	}

	void Refresh()
	{
		g_address = ResolveAddress();
		g_available = g_address != 0;

		if (!g_available)
			return;

		const int32_t raw = *(const int32_t*)g_address;

		if (raw >= 0 && raw <= 100000000)
		{
			g_value = raw;
			g_status = Utils::Format("ok (0x%p = %d)", (void*)g_address, g_value);
		}
		else
		{
			g_status = Utils::Format("value looks wrong (0x%p = %d) - try another index",
				(void*)g_address, raw);
		}
	}

	void Write(int value)
	{
		if (!g_available || g_address == 0)
			Refresh();

		if (!g_available || g_address == 0)
		{
			Notifications::Warn("金钱地址不可用 / money address unavailable");
			return;
		}

		// Always ask the game to set the money through its own native first - this
		// is the safest path and works even when we cannot resolve the global.
		value = Utils::ClampInt(value, 0, 99999999);
		UI::UI_SET_MONEY("", "", value);

		// Memory write: only when the page is genuinely writable AND the current
		// value already looks like a money amount. This guards against writing to
		// a wrong / read-only address (which crashed RDR with ACCESS_VIOLATION).
		if (g_address != 0 && Utils::IsWritable((const void*)g_address, sizeof(int32_t)))
		{
			const int32_t current = *(const int32_t*)g_address;

			if (current >= 0 && current <= 100000000)
			{
				*(int32_t*)g_address = value;
				g_value = value;
				g_status = Utils::Format("ok (0x%p = %d)", (void*)g_address, value);
				Log::Info("Money: set to %d (mem write ok)", value);
			}
			else
			{
				g_value = value;
				g_status = Utils::Format("native set %d, mem skipped (0x%p = %d looks wrong)",
					value, (void*)g_address, current);
				Log::Info("Money: native set %d, mem write skipped (current=%d)", value, current);
			}
		}
		else
		{
			g_value = value;
			g_status = Utils::Format("native set %d (mem unavailable)", value);
			Log::Info("Money: native set %d (address not writable)", value);
		}
	}
}

namespace Money
{
	Settings& Get() { return g_settings; }

	void LoadSettings()
	{
		g_settings.Hotkey = Config::GetInt("Money.Hotkey", g_settings.Hotkey);
		g_settings.QuickAmount = Config::GetInt("Money.QuickAmount", g_settings.QuickAmount);
		g_settings.SetAmount = Config::GetInt("Money.SetAmount", g_settings.SetAmount);
		g_settings.GlobalIndex = Config::GetInt("Money.GlobalIndex", g_settings.GlobalIndex);
		g_settings.UseNative = Config::GetBool("Money.UseNative", g_settings.UseNative);
		g_settings.ShowOnHud = Config::GetBool("Money.ShowOnHud", g_settings.ShowOnHud);
	}

	void SaveSettings()
	{
		Config::SetInt("Money.Hotkey", g_settings.Hotkey);
		Config::SetInt("Money.QuickAmount", g_settings.QuickAmount);
		Config::SetInt("Money.SetAmount", g_settings.SetAmount);
		Config::SetInt("Money.GlobalIndex", g_settings.GlobalIndex);
		Config::SetBool("Money.UseNative", g_settings.UseNative);
		Config::SetBool("Money.ShowOnHud", g_settings.ShowOnHud);
	}

	void Update()
	{
		// The script globals exist as soon as a save is loaded, no need to wait for an actor.
		if (g_address == 0 || Game::Frame() % 45 == 0)
			Refresh();
	}

	void Add(int amount)
	{
		if (!g_available)
			Refresh();

		if (!g_available)
		{
			Notifications::Warn("金钱地址不可用 / money address unavailable");
			return;
		}

		const int before = g_value;
		Write(before + amount);

		Notifications::Info(Utils::Format("金钱 +%d -> %d", amount, before + amount));
	}

	void Set(int amount)
	{
		Write(amount);
		Notifications::Info(Utils::Format("金钱设置为 %d", amount));
	}

	int Current()
	{
		return g_value;
	}

	bool Available()
	{
		return g_available;
	}

	const char* Status()
	{
		return g_status.c_str();
	}
}
