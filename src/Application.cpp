#include "pch.h"

namespace
{
	constexpr const char* kVersion = "1.0.0";

	HMODULE   g_module = nullptr;
	bool      g_initialized = false;
	bool      g_stopping = false;
	int       g_consecutiveErrors = 0;

	uint64_t  g_dirtySince = 0;
	int       g_fps = 0;
	int       g_frameCounter = 0;
	uint64_t  g_fpsTimer = 0;

	void WatchHotkeys()
	{
		Input::Watch(GodMode::Get().Hotkey);
		Input::Watch(InfiniteAmmo::Get().Hotkey);
		Input::Watch(Aimbot::Get().Hotkey);
		Input::Watch(Aimbot::Get().AimKey);
		Input::Watch(Esp::Get().Hotkey);
		Input::Watch(Money::Get().Hotkey);
		Input::Watch(Misc::Get().TeleportHotkey);
	}

	void LoadAllSettings()
	{
		Menu::LoadSettings();
		GodMode::LoadSettings();
		InfiniteAmmo::LoadSettings();
		Aimbot::LoadSettings();
		Esp::LoadSettings();
		Money::LoadSettings();
		Misc::LoadSettings();
	}

	void SaveAllSettings()
	{
		Menu::SaveSettings();
		GodMode::SaveSettings();
		InfiniteAmmo::SaveSettings();
		Aimbot::SaveSettings();
		Esp::SaveSettings();
		Money::SaveSettings();
		Misc::SaveSettings();
	}

	void HandleHotkeys()
	{
		if (Input::Pressed(GodMode::Get().Hotkey))
		{
			GodMode::Get().Enabled = !GodMode::Get().Enabled;
			GodMode::Apply(GodMode::Get().Enabled);
			Notifications::Toggle(Menu::Text("无敌模式", "God mode"),
				GodMode::Get().Enabled ? "开启" : "关闭",
				GodMode::Get().Enabled ? "ON" : "OFF");
			Application::RequestSave();
		}

		if (Input::Pressed(InfiniteAmmo::Get().Hotkey))
		{
			InfiniteAmmo::Get().Enabled = !InfiniteAmmo::Get().Enabled;
			InfiniteAmmo::Apply(InfiniteAmmo::Get().Enabled);
			Notifications::Toggle(Menu::Text("无限子弹", "Infinite ammo"),
				InfiniteAmmo::Get().Enabled ? "开启" : "关闭",
				InfiniteAmmo::Get().Enabled ? "ON" : "OFF");
			Application::RequestSave();
		}

		if (Input::Pressed(Aimbot::Get().Hotkey))
		{
			Aimbot::Get().Enabled = !Aimbot::Get().Enabled;

			if (!Aimbot::Get().Enabled)
				Aimbot::Reset();

			Notifications::Toggle(Menu::Text("自瞄", "Aimbot"),
				Aimbot::Get().Enabled ? "开启" : "关闭",
				Aimbot::Get().Enabled ? "ON" : "OFF");
			Application::RequestSave();
		}

		if (Input::Pressed(Esp::Get().Hotkey))
		{
			Esp::Get().Enabled = !Esp::Get().Enabled;
			Notifications::Toggle(Menu::Text("敌人透视", "ESP"),
				Esp::Get().Enabled ? "开启" : "关闭",
				Esp::Get().Enabled ? "ON" : "OFF");
			Application::RequestSave();
		}

		if (Input::Pressed(Money::Get().Hotkey))
			Money::Add(Money::Get().QuickAmount);

		if (Input::Pressed(Misc::Get().TeleportHotkey))
			Misc::TeleportToWaypoint();
	}

	void UpdateFps()
	{
		++g_frameCounter;

		const uint64_t now = GetTickCount64();

		if (now - g_fpsTimer >= 1000)
		{
			g_fps = (int)((double)g_frameCounter * 1000.0 / (double)(now - g_fpsTimer));
			g_frameCounter = 0;
			g_fpsTimer = now;
		}
	}

	void HandleAutoSave()
	{
		if (g_dirtySince == 0)
			return;

		if (GetTickCount64() - g_dirtySince < 2000)
			return;

		Application::SaveConfig();
	}

	// Cheap per-frame sync so menu changes apply instantly.
	void SyncRuntimeSettings()
	{
		// If the user picked Chinese but the active font has no CJK glyphs, auto-select
		// a font that does. This lets the menu show Chinese without the user having to
		// try font ids one by one (hard to do when the menu is unreadable English).
		static bool s_chineseFontTried = false;

		if (!s_chineseFontTried && Menu::IsChinese() && Screen::IsValid())
		{
			s_chineseFontTried = true;

			if (!Font::SupportsChineseGlyphs())
			{
				const int found = Font::FindChineseFont();

				if (found >= 0 && found != Menu::Get().FontId)
				{
					Menu::Get().FontId = found;
					Config::SetInt("Menu.FontId", found);

					Log::Info("Menu: auto-selected CJK-capable font id %d", found);
				}
				else
				{
					Log::Info("Menu: no CJK-capable font found in 0..12, staying English");
				}
			}
		}

		Render::SetFontId(Menu::Get().FontId);
		Render::SetTextScale(Menu::Get().TextScale);
		Render::SetLineQuality(Menu::Get().LineQuality);

		Notifications::SetLanguage(Menu::UseChinese() ? 0 : 1);
		Notifications::SetGameHudMessages(Menu::Get().GameHudMessages);

		Game::SetHeadOffset(Esp::Get().HeadOffset);
		Game::SetHostilityMode(Esp::Get().HostilityMode);
	}
}

namespace
{
	DWORD g_lastExceptionCode = 0;

	// Kept free of objects that need unwinding so SEH can be used here (C2712).
	bool ExecuteFrameGuarded()
	{
		__try
		{
			Application::Frame();
			g_lastExceptionCode = 0;
			return true;
		}
		__except (g_lastExceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	void RunScriptThread()
	{
		Log::Info("script thread started (RedHook %d.%d)", RH_GetMajorVersion(), RH_GetMinorVersion());

		while (!g_stopping)
		{
			if (ExecuteFrameGuarded())
			{
				g_consecutiveErrors = 0;
			}
			else
			{
				++g_consecutiveErrors;

				Log::Error("exception 0x%08X in frame (%d in a row)",
					(unsigned int)g_lastExceptionCode, g_consecutiveErrors);

				if (g_consecutiveErrors >= 10)
				{
					Log::Error("too many errors, disabling the trainer");
					Notifications::Warn("RDR 修改器 出现异常，已自动停用");
					break;
				}
			}

			ScriptWait(0);
		}

		Log::Info("script thread stopped");
	}
}

void Application::Initialize(HMODULE module)
{
	g_module = module;
	Utils::SetModuleHandle(module);

	Log::Init();
	Log::Info("==================================================");
	Log::Info("RDR trainer v%s loading...", kVersion);
	Log::Info("module path: %s", Utils::ModuleDir().string().c_str());

	Config::Load();
	LoadAllSettings();

	Render::Initialize();
	Screen::Update();
	Menu::Initialize();
	ApplyMenuSettings();
	WatchHotkeys();

	Notifications::SetLanguage(Menu::Get().Language);

	g_fpsTimer = GetTickCount64();
	g_initialized = true;

	ScriptRegister(module, RunScriptThread);

	Log::Info("registered script thread, trainer ready");
}

void Application::Shutdown()
{
	if (!g_initialized)
		return;

	g_initialized = false;
	g_stopping = true;

	ScriptUnregister(g_module);

	SaveConfig();

	Notifications::Shutdown();
	Menu::Shutdown();
	Render::Shutdown();
	TextPool::Shutdown();

	Log::Info("RDR trainer unloaded");
	Log::Shutdown();
}

void Application::Frame()
{
	Input::Update();

	Render::BeginFrame();
	Screen::Update();
	SyncRuntimeSettings();

	Menu::Update();
	HandleHotkeys();

	Game::Update();

	Money::Update();
	GodMode::Update();
	InfiniteAmmo::Update();
	Aimbot::Update();
	Misc::Update();

	Notifications::Update();

	Esp::Draw();
	Menu::DrawHud();
	Menu::Draw();
	Notifications::Draw();

	Render::EndFrame();

	UpdateFps();
	HandleAutoSave();
}

void Application::RequestSave()
{
	g_dirtySince = GetTickCount64();
}

void Application::SaveConfig()
{
	g_dirtySince = 0;
	SaveAllSettings();
	Config::Save();
}

void Application::ReloadConfig()
{
	Config::Load();
	LoadAllSettings();
	ApplyMenuSettings();
	WatchHotkeys();

	Notifications::Info(Menu::Text("配置已重新加载", "config reloaded"));
	Log::Info("config reloaded");
}

void Application::ApplyMenuSettings()
{
	Render::SetFontId(Menu::Get().FontId);
	Render::SetTextScale(Menu::Get().TextScale);
	Render::SetLineQuality(Menu::Get().LineQuality);

	Notifications::SetLanguage(Menu::Get().Language);
	Notifications::SetGameHudMessages(Menu::Get().GameHudMessages);

	WatchHotkeys();
	RequestSave();
}

int Application::Fps()
{
	return g_fps;
}
