#include "pch.h"

namespace
{
	Menu::Settings g_settings;

	std::vector<MenuPage> g_pages;
	std::vector<int> g_stack;      // page stack, back = g_stack.back()
	int  g_selection = 0;
	int  g_scroll = 0;
	bool g_open = false;
	int  g_capturingItem = -1;
	int  g_frameCounter = 0;

	constexpr float kPanelMarginX = 46.0f;
	constexpr float kPanelMarginY = 90.0f;

	// The panel geometry is derived from the measured font size every frame, so the menu
	// can never take over the screen no matter how large the game's font turns out to be.
	struct Layout
	{
		float charWidth = 9.0f;
		float lineHeight = 17.0f;
		float rowHeight = 26.0f;
		float titleHeight = 40.0f;
		float footerHeight = 34.0f;
		float panelWidth = 430.0f;
		int   maxVisibleRows = 15;
	};

	Layout g_layout;

	void UpdateLayout()
	{
		const float panelScale = Utils::Clamp(Menu::Get().PanelScale, 0.5f, 2.0f);

		Layout layout;
		layout.charWidth = std::max(3.0f, Render::CharWidthPx());
		layout.lineHeight = std::max(8.0f, Render::LineHeightPx());
		layout.rowHeight = layout.lineHeight * 1.5f * panelScale;
		layout.titleHeight = layout.lineHeight * 2.2f * panelScale;
		layout.footerHeight = layout.lineHeight * 2.0f * panelScale;
		layout.panelWidth = Utils::Clamp(layout.charWidth * 42.0f * panelScale, 240.0f,
			std::max(240.0f, Screen::Width() * 0.42f));

		const float available = Screen::Height() * 0.78f - layout.titleHeight - layout.footerHeight;
		layout.maxVisibleRows = Utils::ClampInt((int)(available / std::max(1.0f, layout.rowHeight)), 5, 40);

		g_layout = layout;
	}

	MenuPage& CurrentPage()
	{
		return g_pages[(size_t)g_stack.back()];
	}

	bool IsSelectable(const MenuItem& item)
	{
		return item.type != MenuItem::Type::Header && item.type != MenuItem::Type::Label;
	}

	std::string ItemLabel(const MenuItem& item)
	{
		return Menu::UseChinese() ? item.labelZh : item.labelEn;
	}

	void MoveSelection(int direction)
	{
		MenuPage& page = CurrentPage();

		if (page.items.empty())
			return;

		int count = (int)page.items.size();

		for (int attempt = 0; attempt < count; ++attempt)
		{
			g_selection += direction;

			if (g_selection < 0)
				g_selection = count - 1;
			else if (g_selection >= count)
				g_selection = 0;

			if (IsSelectable(page.items[(size_t)g_selection]))
				break;
		}

		if (g_selection < g_scroll)
			g_scroll = g_selection;
		else if (g_selection >= g_scroll + g_layout.maxVisibleRows)
			g_scroll = g_selection - g_layout.maxVisibleRows + 1;
	}

	void AdjustItem(MenuItem& item, int direction)
	{
		switch (item.type)
		{
			case MenuItem::Type::Toggle:
				if (item.boolValue != nullptr)
					*item.boolValue = !*item.boolValue;
				break;

			case MenuItem::Type::IntSlider:
				if (item.intValue != nullptr)
					*item.intValue = Utils::ClampInt(*item.intValue + direction * item.stepInt, item.minInt, item.maxInt);
				break;

			case MenuItem::Type::FloatSlider:
				if (item.floatValue != nullptr)
					*item.floatValue = Utils::Clamp(*item.floatValue + (float)direction * item.stepFloat, item.minFloat, item.maxFloat);
				break;

			case MenuItem::Type::Choice:
				if (item.intValue != nullptr && !item.choices.empty())
				{
					int count = (int)item.choices.size();
					*item.intValue = (*item.intValue + direction + count) % count;
				}
				break;

			case MenuItem::Type::Keybind:
				if (item.intValue != nullptr)
				{
					g_capturingItem = -1;
					Input::StartCapture();
					g_capturingItem = (int)(&item - &CurrentPage().items[0]);
				}
				break;

			default:
				break;
		}

		Application::RequestSave();
	}

	std::string ValueString(const MenuItem& item)
	{
		if (item.valueText)
			return item.valueText();

		switch (item.type)
		{
			case MenuItem::Type::Toggle:
				if (item.boolValue != nullptr)
					return *item.boolValue ? (Menu::IsChinese() ? "[ 开 ]" : "[ ON ]")
					                       : (Menu::IsChinese() ? "[ 关 ]" : "[ OFF ]");
				break;

			case MenuItem::Type::IntSlider:
				if (item.intValue != nullptr)
					return Utils::Format("< %d%s >", *item.intValue, item.suffix);
				break;

			case MenuItem::Type::FloatSlider:
				if (item.floatValue != nullptr)
				{
					if (item.integerFloat)
						return Utils::Format("< %d%s >", (int)*item.floatValue, item.suffix);

					return Utils::Format("< %.*f%s >", item.decimals, *item.floatValue, item.suffix);
				}
				break;

			case MenuItem::Type::Choice:
				if (item.intValue != nullptr && !item.choices.empty())
				{
					int index = Utils::ClampInt(*item.intValue, 0, (int)item.choices.size() - 1);
					return Menu::IsChinese() ? item.choices[(size_t)index].first : item.choices[(size_t)index].second;
				}
				break;

			case MenuItem::Type::Keybind:
				if (item.intValue != nullptr)
					return Utils::KeyIdName(*item.intValue);
				break;

			case MenuItem::Type::SubMenu:
				return ">";

			case MenuItem::Type::Button:
				return Menu::IsChinese() ? "[ 执行 ]" : "[ RUN ]";

			default:
				break;
		}

		return std::string();
	}

	void ActivateItem(MenuItem& item)
	{
		switch (item.type)
		{
			case MenuItem::Type::SubMenu:
				if (item.subMenuIndex >= 0)
				{
					g_stack.push_back(item.subMenuIndex);
					g_selection = 0;
					g_scroll = 0;
				}
				break;

			case MenuItem::Type::Button:
				if (item.action)
					item.action();
				break;

			default:
				AdjustItem(item, 1);
				break;
		}
	}

	void AddToggle(MenuPage& page, const char* zh, const char* en, bool* value)
	{
		MenuItem item;
		item.type = MenuItem::Type::Toggle;
		item.labelZh = zh;
		item.labelEn = en;
		item.boolValue = value;
		page.items.push_back(std::move(item));
	}

	MenuItem& AddItem(MenuPage& page, MenuItem::Type type, const char* zh, const char* en)
	{
		MenuItem item;
		item.type = type;
		item.labelZh = zh;
		item.labelEn = en;
		page.items.push_back(std::move(item));
		return page.items.back();
	}

	std::vector<std::pair<std::string, std::string>> PresetChoices()
	{
		std::vector<std::pair<std::string, std::string>> choices;

		for (int index = 0; index < Theme::PresetCount; ++index)
			choices.emplace_back(Theme::Presets[index].nameZh, Theme::Presets[index].nameEn);

		return choices;
	}

	void BuildPages()
	{
		g_pages.clear();

		const int pageGod = 1;
		const int pageAmmo = 2;
		const int pageAimbot = 3;
		const int pageEsp = 4;
		const int pageMoney = 5;
		const int pageMisc = 6;
		const int pageSettings = 7;

		// ---------------- main ----------------
		MenuPage main;
		main.titleZh = "RDR 修改器";
		main.titleEn = "RDR TRAINER";

		MenuItem title = {};
		title.type = MenuItem::Type::Header;
		title.labelZh = "按 左右方向键 调整数值，回车 确认";
		title.labelEn = "Arrows to adjust, ENTER to confirm";
		main.items.push_back(title);

		AddToggle(main, "无敌模式", "God Mode", &GodMode::Get().Enabled);
		AddToggle(main, "无限子弹", "Infinite Ammo", &InfiniteAmmo::Get().Enabled);
		AddToggle(main, "自瞄", "Aimbot", &Aimbot::Get().Enabled);
		AddToggle(main, "敌人透视", "ESP", &Esp::Get().Enabled);
		AddToggle(main, "不会被通缉", "Never wanted", &Misc::Get().NoWanted);

		MenuItem& toGod = AddItem(main, MenuItem::Type::SubMenu, "玩家 / 无敌设置", "Player / God options");
		toGod.subMenuIndex = pageGod;

		MenuItem& toAmmo = AddItem(main, MenuItem::Type::SubMenu, "武器 / 子弹设置", "Weapon / Ammo options");
		toAmmo.subMenuIndex = pageAmmo;

		MenuItem& toAimbot = AddItem(main, MenuItem::Type::SubMenu, "自瞄设置", "Aimbot options");
		toAimbot.subMenuIndex = pageAimbot;

		MenuItem& toEsp = AddItem(main, MenuItem::Type::SubMenu, "透视设置", "ESP options");
		toEsp.subMenuIndex = pageEsp;

		MenuItem& toMoney = AddItem(main, MenuItem::Type::SubMenu, "金钱", "Money");
		toMoney.subMenuIndex = pageMoney;

		MenuItem& toMisc = AddItem(main, MenuItem::Type::SubMenu, "杂项", "Misc");
		toMisc.subMenuIndex = pageMisc;

		MenuItem& toSettings = AddItem(main, MenuItem::Type::SubMenu, "界面与热键", "Interface & hotkeys");
		toSettings.subMenuIndex = pageSettings;

		MenuItem& saveButton = AddItem(main, MenuItem::Type::Button, "保存配置", "Save config");
		saveButton.action = [] { Application::SaveConfig(); };

		g_pages.push_back(std::move(main));

		// ---------------- god ----------------
		MenuPage god;
		god.titleZh = "玩家 / 无敌";
		god.titleEn = "Player / God";
		AddToggle(god, "每帧补满血量", "Keep health at max", &GodMode::Get().FullHealth);
		AddToggle(god, "不灭之躯 (无法被击杀)", "Unkillable", &GodMode::Get().Unkillable);
		AddToggle(god, "保护坐骑", "Protect mount", &GodMode::Get().Horse);
		AddToggle(god, "无限死神之眼", "Infinite deadeye", &GodMode::Get().InfiniteDeadeye);
		AddToggle(god, "死神之眼强化", "Deadeye power", &GodMode::Get().DeadeyePower);
		MenuItem& godKey = AddItem(god, MenuItem::Type::Keybind, "无敌热键", "God mode hotkey");
		godKey.intValue = &GodMode::Get().Hotkey;
		MenuItem& godRefill = AddItem(god, MenuItem::Type::Button, "立即补满血量", "Refill health now");
		godRefill.action = [] { Misc::RefillHealth(); };
		MenuItem& godDeadeye = AddItem(god, MenuItem::Type::Button, "立即补满死神之眼", "Refill deadeye now");
		godDeadeye.action = [] { Misc::RefillDeadeye(); };
		g_pages.push_back(std::move(god));

		// ---------------- ammo ----------------
		MenuPage ammo;
		ammo.titleZh = "武器 / 子弹";
		ammo.titleEn = "Weapon / Ammo";
		AddToggle(ammo, "自动补满武器弹药", "Keep weapon topped up", &InfiniteAmmo::Get().RefillClip);
		AddToggle(ammo, "秒换弹", "Instant reload", &InfiniteAmmo::Get().NoReload);
		MenuItem& ammoRange = AddItem(ammo, MenuItem::Type::IntSlider, "武器编号上限", "Max weapon id");
		ammoRange.intValue = &InfiniteAmmo::Get().MaxWeaponModel;
		ammoRange.minInt = 10;
		ammoRange.maxInt = 80;
		ammoRange.stepInt = 5;
		MenuItem& ammoKey = AddItem(ammo, MenuItem::Type::Keybind, "无限子弹热键", "Ammo hotkey");
		ammoKey.intValue = &InfiniteAmmo::Get().Hotkey;
		MenuItem& ammoGive = AddItem(ammo, MenuItem::Type::Button, "补满当前武器弹药", "Top up current weapon");
		ammoGive.action = [] { Misc::GiveAllWeapons(); };
		g_pages.push_back(std::move(ammo));

		// ---------------- aimbot ----------------
		MenuPage aimbot;
		aimbot.titleZh = "自瞄";
		aimbot.titleEn = "Aimbot";

		MenuItem& aimMode = AddItem(aimbot, MenuItem::Type::Choice, "自瞄模式", "Aimbot mode");
		aimMode.intValue = &Aimbot::Get().Mode;
		aimMode.choices = {
			{ "辅助锁定 (游戏自带吸附)", "Assist lock (game snap)" },
			{ "鼠标自瞄 (推荐, 会动准星)", "Mouse aim (recommended)" },
			{ "AI 强制瞄准 (实验)", "AI force aim (experimental)" },
		};

		MenuItem& aimTrigger = AddItem(aimbot, MenuItem::Type::Choice, "触发方式", "Trigger");
		aimTrigger.intValue = &Aimbot::Get().Trigger;
		aimTrigger.choices = {
			{ "按住瞄准键时", "While aim key held" },
			{ "始终开启", "Always on" },
		};

		MenuItem& aimKey = AddItem(aimbot, MenuItem::Type::Keybind, "瞄准键", "Aim key");
		aimKey.intValue = &Aimbot::Get().AimKey;

		MenuItem& aimPriority = AddItem(aimbot, MenuItem::Type::Choice, "目标优先级", "Target priority");
		aimPriority.intValue = &Aimbot::Get().TargetPriority;
		aimPriority.choices = {
			{ "准星最近", "Closest to crosshair" },
			{ "距离最近", "Closest distance" },
			{ "血量最低", "Lowest health" },
		};

		MenuItem& aimFov = AddItem(aimbot, MenuItem::Type::FloatSlider, "自瞄范围 (像素)", "FOV (pixels)");
		aimFov.floatValue = &Aimbot::Get().FovPixels;
		aimFov.minFloat = 30.0f;
		aimFov.maxFloat = 900.0f;
		aimFov.stepFloat = 10.0f;
		aimFov.integerFloat = true;

		MenuItem& aimDistance = AddItem(aimbot, MenuItem::Type::FloatSlider, "最大距离 (米)", "Max distance (m)");
		aimDistance.floatValue = &Aimbot::Get().MaxDistance;
		aimDistance.minFloat = 10.0f;
		aimDistance.maxFloat = 500.0f;
		aimDistance.stepFloat = 10.0f;
		aimDistance.integerFloat = true;

		AddToggle(aimbot, "只瞄准敌对目标", "Hostile targets only", &Aimbot::Get().HostileOnly);
		AddToggle(aimbot, "需要视线 (穿墙不打)", "Require line of sight", &Aimbot::Get().RequireLineOfSight);
		AddToggle(aimbot, "忽略动物", "Ignore animals", &Aimbot::Get().IgnoreAnimals);
		AddToggle(aimbot, "自动开火", "Auto fire", &Aimbot::Get().AutoFire);
		AddToggle(aimbot, "完美精准", "Perfect accuracy", &Aimbot::Get().PerfectAccuracy);
		AddToggle(aimbot, "瞄准辅助标记", "Aim assist flag", &Aimbot::Get().AimAssistFlag);
		AddToggle(aimbot, "硬锁定偏置 (实验)", "Hardlock bias (experimental)", &Aimbot::Get().HardlockBias);
		AddToggle(aimbot, "转向目标", "Turn body to target", &Aimbot::Get().TurnBody);
		AddToggle(aimbot, "显示自瞄范围圈", "Draw FOV circle", &Aimbot::Get().DrawFov);

		MenuItem& aimSmooth = AddItem(aimbot, MenuItem::Type::FloatSlider, "鼠标平滑 (越低越慢)", "Mouse smoothing");
		aimSmooth.floatValue = &Aimbot::Get().MouseSmooth;
		aimSmooth.minFloat = 0.02f;
		aimSmooth.maxFloat = 1.0f;
		aimSmooth.stepFloat = 0.02f;

		MenuItem& aimStep = AddItem(aimbot, MenuItem::Type::FloatSlider, "鼠标每帧步长", "Mouse max step");
		aimStep.floatValue = &Aimbot::Get().MouseMaxStep;
		aimStep.minFloat = 1.0f;
		aimStep.maxFloat = 200.0f;
		aimStep.stepFloat = 2.0f;
		aimStep.integerFloat = true;

		MenuItem& aimHotkey = AddItem(aimbot, MenuItem::Type::Keybind, "自瞄开关热键", "Aimbot hotkey");
		aimHotkey.intValue = &Aimbot::Get().Hotkey;
		g_pages.push_back(std::move(aimbot));

		// ---------------- esp ----------------
		MenuPage esp;
		esp.titleZh = "敌人透视";
		esp.titleEn = "ESP";
		AddToggle(esp, "方框", "Box", &Esp::Get().Box);
		AddToggle(esp, "边角模式", "Corner style", &Esp::Get().Corners);
		AddToggle(esp, "名称", "Name", &Esp::Get().Name);
		AddToggle(esp, "距离", "Distance", &Esp::Get().Distance);
		AddToggle(esp, "血量条", "Health bar", &Esp::Get().Health);
		AddToggle(esp, "底部连线", "Snapline", &Esp::Get().Snapline);
		AddToggle(esp, "高亮自瞄目标", "Highlight aimbot target", &Esp::Get().HighlightTarget);
		AddToggle(esp, "显示尸体", "Show dead", &Esp::Get().ShowDead);
		AddToggle(esp, "显示自己", "Show local player", &Esp::Get().ShowLocalPlayer);
		AddToggle(esp, "显示动物", "Show animals", &Esp::Get().ShowAnimals);
		AddToggle(esp, "显示中立 / 友军", "Show friendly", &Esp::Get().ShowFriendly);

		MenuItem& espDistance = AddItem(esp, MenuItem::Type::FloatSlider, "最大显示距离 (米)", "Max distance (m)");
		espDistance.floatValue = &Esp::Get().MaxDistance;
		espDistance.minFloat = 20.0f;
		espDistance.maxFloat = 1000.0f;
		espDistance.stepFloat = 10.0f;
		espDistance.integerFloat = true;

		MenuItem& espThickness = AddItem(esp, MenuItem::Type::FloatSlider, "线条粗细", "Line thickness");
		espThickness.floatValue = &Esp::Get().LineThickness;
		espThickness.minFloat = 1.0f;
		espThickness.maxFloat = 5.0f;
		espThickness.stepFloat = 0.2f;
		espThickness.decimals = 1;

		MenuItem& espLabel = AddItem(esp, MenuItem::Type::FloatSlider, "标签大小", "Label scale");
		espLabel.floatValue = &Esp::Get().LabelScale;
		espLabel.minFloat = 0.4f;
		espLabel.maxFloat = 2.5f;
		espLabel.stepFloat = 0.1f;
		espLabel.decimals = 1;

		MenuItem& espHead = AddItem(esp, MenuItem::Type::FloatSlider, "人物高度 (米)", "Actor height (m)");
		espHead.floatValue = &Esp::Get().HeadOffset;
		espHead.minFloat = 0.8f;
		espHead.maxFloat = 3.5f;
		espHead.stepFloat = 0.02f;
		espHead.decimals = 2;

		MenuItem& espColorHostile = AddItem(esp, MenuItem::Type::Choice, "敌人颜色", "Hostile colour");
		espColorHostile.intValue = &Esp::Get().ColorHostile;
		espColorHostile.choices = PresetChoices();

		MenuItem& espColorAnimal = AddItem(esp, MenuItem::Type::Choice, "动物颜色", "Animal colour");
		espColorAnimal.intValue = &Esp::Get().ColorAnimal;
		espColorAnimal.choices = PresetChoices();

		MenuItem& espColorFriendly = AddItem(esp, MenuItem::Type::Choice, "友军颜色", "Friendly colour");
		espColorFriendly.intValue = &Esp::Get().ColorFriendly;
		espColorFriendly.choices = PresetChoices();

		MenuItem& espColorTarget = AddItem(esp, MenuItem::Type::Choice, "目标颜色", "Target colour");
		espColorTarget.intValue = &Esp::Get().ColorTarget;
		espColorTarget.choices = PresetChoices();

		MenuItem& espHostility = AddItem(esp, MenuItem::Type::Choice, "敌对判定方式", "Hostility detection");
		espHostility.intValue = &Esp::Get().HostilityMode;
		espHostility.choices = {
			{ "游戏原生 (玩家, 目标)", "Native (player, actor)" },
			{ "游戏原生 (目标, 玩家)", "Native (actor, player)" },
			{ "全部视为敌人", "Everyone is hostile" },
		};

		AddToggle(esp, "显示水印 / 状态", "Show watermark", &Esp::Get().ShowWatermark);

		MenuItem& espHotkey = AddItem(esp, MenuItem::Type::Keybind, "透视开关热键", "ESP hotkey");
		espHotkey.intValue = &Esp::Get().Hotkey;
		g_pages.push_back(std::move(esp));

		// ---------------- money ----------------
		MenuPage money;
		money.titleZh = "金钱";
		money.titleEn = "Money";

		MenuItem& moneyCurrent = AddItem(money, MenuItem::Type::Label, "当前金额", "Current money");
		moneyCurrent.valueText = [] {
			if (!Money::Available())
				return std::string(Money::Status());

			return Utils::Format("%d", Money::Current());
		};

		MenuItem& moneyState = AddItem(money, MenuItem::Type::Label, "地址状态", "Address status");
		moneyState.valueText = [] { return std::string(Money::Status()); };

		MenuItem& moneyQuick = AddItem(money, MenuItem::Type::IntSlider, "快捷加钱数额", "Quick add amount");
		moneyQuick.intValue = &Money::Get().QuickAmount;
		moneyQuick.minInt = 100;
		moneyQuick.maxInt = 100000;
		moneyQuick.stepInt = 500;

		MenuItem& moneySet = AddItem(money, MenuItem::Type::IntSlider, "设定金额", "Set amount");
		moneySet.intValue = &Money::Get().SetAmount;
		moneySet.minInt = 0;
		moneySet.maxInt = 9999999;
		moneySet.stepInt = 10000;

		MenuItem& moneyGlobal = AddItem(money, MenuItem::Type::IntSlider, "全局变量编号", "Global index");
		moneyGlobal.intValue = &Money::Get().GlobalIndex;
		moneyGlobal.minInt = 0;
		moneyGlobal.maxInt = 70000;
		moneyGlobal.stepInt = 1;

		AddToggle(money, "同时调用游戏原生接口", "Also call UI_SET_MONEY", &Money::Get().UseNative);
		AddToggle(money, "在 HUD 上显示金额", "Show money on HUD", &Money::Get().ShowOnHud);

		MenuItem& moneyAdd = AddItem(money, MenuItem::Type::Button, "加钱 (快捷数额)", "Add quick amount");
		moneyAdd.action = [] { Money::Add(Money::Get().QuickAmount); };

		MenuItem& moneyApply = AddItem(money, MenuItem::Type::Button, "设定为上方金额", "Set to amount");
		moneyApply.action = [] { Money::Set(Money::Get().SetAmount); };

		MenuItem& moneyMax = AddItem(money, MenuItem::Type::Button, "金钱拉满 (9999999)", "Max money");
		moneyMax.action = [] { Money::Set(9999999); };

		MenuItem& moneyHotkey = AddItem(money, MenuItem::Type::Keybind, "加钱热键", "Money hotkey");
		moneyHotkey.intValue = &Money::Get().Hotkey;
		g_pages.push_back(std::move(money));

		// ---------------- misc ----------------
		MenuPage misc;
		misc.titleZh = "杂项";
		misc.titleEn = "Misc";

		MenuItem& teleport = AddItem(misc, MenuItem::Type::Button, "传送到地图标记点", "Teleport to waypoint");
		teleport.action = [] { Misc::TeleportToWaypoint(); };

		MenuItem& teleportKey = AddItem(misc, MenuItem::Type::Keybind, "传送热键", "Teleport hotkey");
		teleportKey.intValue = &Misc::Get().TeleportHotkey;

		AddToggle(misc, "启用时间缩放", "Enable time scale", &Misc::Get().TimeScaleEnabled);

		AddToggle(misc, "不被通缉", "Never wanted", &Misc::Get().NoWanted);

		AddToggle(misc, "荣誉值锁定最高", "Lock honor to max", &Misc::Get().MaxHonor);

		MenuItem& timeScale = AddItem(misc, MenuItem::Type::FloatSlider, "时间缩放倍数", "Time scale");
		timeScale.floatValue = &Misc::Get().TimeScale;
		timeScale.minFloat = 0.0f;
		timeScale.maxFloat = 3.0f;
		timeScale.stepFloat = 0.05f;
		timeScale.decimals = 2;

		MenuItem& honor = AddItem(misc, MenuItem::Type::FloatSlider, "荣誉值", "Honor");
		honor.floatValue = &Misc::Get().Honor;
		honor.minFloat = -1.0f;
		honor.maxFloat = 1.0f;
		honor.stepFloat = 0.05f;
		honor.decimals = 2;

		MenuItem& honorApply = AddItem(misc, MenuItem::Type::Button, "应用荣誉值", "Apply honor");
		honorApply.action = [] { Misc::ApplyHonor(Misc::Get().Honor); };

		MenuItem& notoriety = AddItem(misc, MenuItem::Type::FloatSlider, "恶名值", "Notoriety");
		notoriety.floatValue = &Misc::Get().Notoriety;
		notoriety.minFloat = 0.0f;
		notoriety.maxFloat = 1.0f;
		notoriety.stepFloat = 0.05f;
		notoriety.decimals = 2;

		MenuItem& notorietyApply = AddItem(misc, MenuItem::Type::Button, "应用恶名值", "Apply notoriety");
		notorietyApply.action = [] { Misc::ApplyNotoriety(Misc::Get().Notoriety); };

		MenuItem& debugReload = AddItem(misc, MenuItem::Type::Button, "重新加载配置", "Reload config");
		debugReload.action = [] { Application::ReloadConfig(); };
		g_pages.push_back(std::move(misc));

		// ---------------- settings ----------------
		MenuPage settings;
		settings.titleZh = "界面与热键";
		settings.titleEn = "Interface & hotkeys";

		MenuItem& menuKey = AddItem(settings, MenuItem::Type::Keybind, "菜单开关热键", "Menu hotkey");
		menuKey.intValue = &Menu::Get().MenuKey;

		MenuItem& language = AddItem(settings, MenuItem::Type::Choice, "语言", "Language");
		language.intValue = &Menu::Get().Language;
		language.choices = { { "中文", "Chinese" }, { "English", "English" } };
		language.action = nullptr;

		MenuItem& fontId = AddItem(settings, MenuItem::Type::IntSlider, "字体编号 (逐个尝试)", "Font id");
		fontId.intValue = &Menu::Get().FontId;
		fontId.minInt = 0;
		fontId.maxInt = 12;
		fontId.stepInt = 1;

		AddToggle(settings, "强制中文渲染 (跳过字形检测)", "Force Chinese render", &Menu::Get().ForceChinese);

		MenuItem& textScale = AddItem(settings, MenuItem::Type::FloatSlider, "文字大小", "Text scale");
		textScale.floatValue = &Menu::Get().TextScale;
		textScale.minFloat = 0.05f;
		textScale.maxFloat = 4.0f;
		textScale.stepFloat = 0.05f;
		textScale.decimals = 2;

		MenuItem& panelScale = AddItem(settings, MenuItem::Type::FloatSlider, "面板大小", "Panel scale");
		panelScale.floatValue = &Menu::Get().PanelScale;
		panelScale.minFloat = 0.5f;
		panelScale.maxFloat = 2.0f;
		panelScale.stepFloat = 0.05f;
		panelScale.decimals = 2;

		MenuItem& lineQuality = AddItem(settings, MenuItem::Type::Choice, "线条质量", "Line quality");
		lineQuality.intValue = &Menu::Get().LineQuality;
		lineQuality.choices = { { "低", "Low" }, { "中", "Medium" }, { "高", "High" } };

		AddToggle(settings, "游戏内文字提示", "In-game toast", &Menu::Get().GameHudMessages);
		AddToggle(settings, "显示 FPS", "Show FPS", &Menu::Get().ShowFps);
		AddToggle(settings, "显示金钱", "Show money", &Menu::Get().ShowMoney);

		MenuItem& saveConfig = AddItem(settings, MenuItem::Type::Button, "保存配置", "Save config");
		saveConfig.action = [] { Application::SaveConfig(); };

		MenuItem& reloadConfig = AddItem(settings, MenuItem::Type::Button, "重新加载配置", "Reload config");
		reloadConfig.action = [] { Application::ReloadConfig(); };

		MenuItem& info = AddItem(settings, MenuItem::Type::Label, "未加载世界时功能可能无效", "Features need a loaded world / save");
		info.valueText = [] { return Game::WorldLoaded() ? std::string("OK") : std::string("world not loaded"); };
		g_pages.push_back(std::move(settings));
	}
}

namespace Menu
{
	Settings& Get() { return g_settings; }

	void LoadSettings()
	{
		g_settings.MenuKey = Config::GetInt("Menu.Hotkey", g_settings.MenuKey);
		g_settings.Language = Config::GetInt("Menu.Language", g_settings.Language);
		g_settings.FontId = Config::GetInt("Menu.FontId", g_settings.FontId);
		g_settings.ForceChinese = Config::GetBool("Menu.ForceChinese", g_settings.ForceChinese);
		g_settings.TextScale = Config::GetFloat("Menu.TextScale", g_settings.TextScale);
		g_settings.PanelScale = Config::GetFloat("Menu.PanelScale", g_settings.PanelScale);
		g_settings.LineQuality = Config::GetInt("Menu.LineQuality", g_settings.LineQuality);
		g_settings.GameHudMessages = Config::GetBool("Menu.GameHudMessages", g_settings.GameHudMessages);
		g_settings.ShowWatermark = Config::GetBool("Menu.ShowWatermark", g_settings.ShowWatermark);
		g_settings.ShowFps = Config::GetBool("Menu.ShowFps", g_settings.ShowFps);
		g_settings.ShowMoney = Config::GetBool("Menu.ShowMoney", g_settings.ShowMoney);
	}

	void SaveSettings()
	{
		Config::SetInt("Menu.Hotkey", g_settings.MenuKey);
		Config::SetInt("Menu.Language", g_settings.Language);
		Config::SetInt("Menu.FontId", g_settings.FontId);
		Config::SetBool("Menu.ForceChinese", g_settings.ForceChinese);
		Config::SetFloat("Menu.TextScale", g_settings.TextScale);
		Config::SetFloat("Menu.PanelScale", g_settings.PanelScale);
		Config::SetInt("Menu.LineQuality", g_settings.LineQuality);
		Config::SetBool("Menu.GameHudMessages", g_settings.GameHudMessages);
		Config::SetBool("Menu.ShowWatermark", g_settings.ShowWatermark);
		Config::SetBool("Menu.ShowFps", g_settings.ShowFps);
		Config::SetBool("Menu.ShowMoney", g_settings.ShowMoney);
	}

	void Initialize()
	{
		BuildPages();

		if (g_stack.empty())
			g_stack.push_back(0);

		Input::Watch(g_settings.MenuKey);
		Input::Watch(KEY_UP_ARROW);
		Input::Watch(KEY_DOWN_ARROW);
		Input::Watch(KEY_LEFT_ARROW);
		Input::Watch(KEY_RIGHT_ARROW);
		Input::Watch(KEY_ENTER);
		Input::Watch(KEY_BACKSPACE);
		Input::Watch(KEY_NUMPAD_8);
		Input::Watch(KEY_NUMPAD_2);
		Input::Watch(KEY_NUMPAD_4);
		Input::Watch(KEY_NUMPAD_6);
		Input::Watch(KEY_NUMPAD_ENTER);
	}

	void Shutdown()
	{
		g_pages.clear();
		g_stack.clear();
	}

	void SetOpen(bool open)
	{
		g_open = open;

		if (!open)
		{
			Input::StopCapture();
			g_capturingItem = -1;
		}
	}

	bool IsOpen() { return g_open; }

	void Toggle()
	{
		SetOpen(!g_open);
	}

	bool IsChinese()
	{
		return g_settings.Language == 0;
	}

	bool UseChinese()
	{
		// Render Chinese when the user picked Chinese AND either the active font is
		// proven to carry CJK glyphs, or the user forced it (to override a probe that
		// missed a capable font). Without one of these, Chinese would be a wall of '?'.
		return IsChinese() && (g_settings.ForceChinese || Font::SupportsChineseGlyphs());
	}

	const char* Text(const char* chinese, const char* english)
	{
		return UseChinese() ? chinese : english;
	}

	void Update()
	{
		++g_frameCounter;
		UpdateLayout();

		const int menuKey = g_settings.MenuKey;

		if (Input::Pressed(menuKey))
			Toggle();

		if (!g_open)
			return;

		if (Input::IsCapturing())
		{
			const int captured = Input::CapturedKey();

			if (captured != KEY_NONE)
			{
				Input::StopCapture();

				if (g_capturingItem >= 0 && g_capturingItem < (int)CurrentPage().items.size())
				{
					MenuItem& item = CurrentPage().items[(size_t)g_capturingItem];

					if (item.intValue != nullptr)
						*item.intValue = captured;

					if (item.intValue == &g_settings.MenuKey)
						Input::Watch(g_settings.MenuKey);
				}

				g_capturingItem = -1;
				Application::RequestSave();
				Application::ApplyMenuSettings();
			}

			return;
		}

		const bool up = Input::Pressed(KEY_UP_ARROW) || Input::Pressed(KEY_NUMPAD_8);
		const bool down = Input::Pressed(KEY_DOWN_ARROW) || Input::Pressed(KEY_NUMPAD_2);
		const bool left = Input::Pressed(KEY_LEFT_ARROW) || Input::Pressed(KEY_NUMPAD_4) || Input::Pressed(KEY_NUMPAD_MINUS);
		const bool right = Input::Pressed(KEY_RIGHT_ARROW) || Input::Pressed(KEY_NUMPAD_6) || Input::Pressed(KEY_NUMPAD_PLUS);
		const bool enter = Input::Pressed(KEY_ENTER) || Input::Pressed(KEY_NUMPAD_ENTER);

		MenuPage& page = CurrentPage();

		if (page.items.empty())
			return;

		g_selection = Utils::ClampInt(g_selection, 0, (int)page.items.size() - 1);

		if (up)
			MoveSelection(-1);

		if (down)
			MoveSelection(1);

		if (left && IsSelectable(page.items[(size_t)g_selection]))
			AdjustItem(page.items[(size_t)g_selection], -1);

		if (right && IsSelectable(page.items[(size_t)g_selection]))
			AdjustItem(page.items[(size_t)g_selection], 1);

		if (enter)
			ActivateItem(page.items[(size_t)g_selection]);

		if (Input::Pressed(KEY_BACKSPACE) || Input::Pressed(KEY_ESCAPE))
		{
			if (g_stack.size() > 1)
			{
				g_stack.pop_back();
				g_selection = 0;
				g_scroll = 0;
			}
			else
			{
				SetOpen(false);
			}
		}
	}

	void Draw()
	{
		if (!g_open || !Screen::IsValid() || g_stack.empty())
			return;

		const MenuPage& page = g_pages[(size_t)g_stack.back()];
		UpdateLayout();

		const float rowHeight = g_layout.rowHeight;
		const float titleHeight = g_layout.titleHeight;
		const float footerHeight = g_layout.footerHeight;
		const float width = g_layout.panelWidth;
		const float padding = g_layout.charWidth * 1.6f;
		const float textOffset = (rowHeight - g_layout.lineHeight) * 0.5f;
		const float x = kPanelMarginX;
		const float y = kPanelMarginY;

		const int totalRows = (int)page.items.size();
		const int visibleRows = std::min(totalRows, g_layout.maxVisibleRows);
		const float height = titleHeight + rowHeight * (float)visibleRows + footerHeight;

		Render::Rect(x - 2.0f, y - 2.0f, width + 4.0f, height + 4.0f, Theme::PanelBorder);
		Render::Rect(x, y, width, height, Theme::Panel);
		Render::Rect(x, y, width, titleHeight, Theme::Header);

		const std::string title = Menu::UseChinese() ? page.titleZh : page.titleEn;
		Render::Text(title, x + padding, y + (titleHeight - g_layout.lineHeight) * 0.5f, 1.15f, Theme::HeaderText, 0, true);

		std::string rightTitle = Utils::Format("%d/%d", (int)g_stack.size(), (int)g_pages.size());
		Render::Text(rightTitle, x + width - padding, y + (titleHeight - g_layout.lineHeight) * 0.5f, 0.9f, Theme::SubTitle, 2, true);

		float rowY = y + titleHeight;

		for (int index = g_scroll; index < g_scroll + visibleRows && index < totalRows; ++index)
		{
			const MenuItem& item = page.items[(size_t)index];
			const bool selected = (index == g_selection);
			const std::string label = ItemLabel(item);
			const std::string value = ValueString(item);

			if (selected)
				Render::Rect(x + 2.0f, rowY, width - 4.0f, rowHeight, Theme::Selected);

			Color textColor = Theme::ItemText;

			if (item.type == MenuItem::Type::Header)
				textColor = Theme::HintText;
			else if (!selected)
				textColor = Theme::ItemTextOff;

			if (selected)
				textColor = Theme::SelectedText;

			const int savedFontId = Render::FontId();

			if (item.useOwnFontId)
			{
				// Font picker row: draw with that font at its own calibrated size.
				const float ownScale = Render::ScaleForFont(item.ownFontId) * Render::TextScale();

				Render::TextWithFont(label, x + padding, rowY + textOffset, 1.0f, textColor, 0, item.ownFontId, true);

				if (!value.empty())
					Render::TextWithFont(value, x + width - padding, rowY + textOffset, ownScale,
						Theme::PresetColor(1), 2, item.ownFontId, false);
			}
			else
			{
				Render::Text(label, x + padding, rowY + textOffset, 1.0f, textColor, 0, true);

				if (!value.empty())
				{
					Color valueColor = selected ? Theme::SelectedText : Theme::ValueText;

					if (item.type == MenuItem::Type::Toggle && item.boolValue != nullptr)
						valueColor = *item.boolValue ? Theme::HealthGood : Theme::ItemTextOff;

					Render::Text(value, x + width - padding, rowY + textOffset, 1.0f, valueColor, 2, true);
				}
			}

			Render::SetFontId(savedFontId);
			rowY += rowHeight;
		}

		// scrollbar
		if (totalRows > visibleRows)
		{
			const float trackHeight = rowHeight * (float)visibleRows;
			const float thumbHeight = trackHeight * (float)visibleRows / (float)totalRows;
			const float thumbY = y + titleHeight + trackHeight * (float)g_scroll / (float)totalRows;

			Render::Rect(x + width - 4.0f, y + titleHeight, 3.0f, trackHeight, Theme::Panel);
			Render::Rect(x + width - 4.0f, thumbY, 3.0f, thumbHeight, Theme::PanelBorder);
		}

		const float footerY = y + height - footerHeight;
		Render::Rect(x, footerY, width, footerHeight, Theme::Header.WithAlpha(180));

		if (Input::IsCapturing())
		{
			Render::Text(Menu::UseChinese() ? "请按下要绑定的按键..." : "Press a key to bind...",
				x + padding, footerY + (footerHeight - g_layout.lineHeight) * 0.5f, 1.0f, Theme::ValueText, 0, true);
		}
		else
		{
			const char* hint = Menu::UseChinese()
				? "上下选择  左右调整  回车确认  Backspace 返回"
				: "UP/DOWN select  LEFT/RIGHT change  ENTER confirm  BACKSPACE back";

			Render::Text(hint, x + padding, footerY + (footerHeight - g_layout.lineHeight) * 0.5f, 0.85f, Theme::HintText, 0, true);
		}
	}

	void DrawHud()
	{
		if (!Screen::IsValid())
			return;

		if (!Esp::Get().ShowWatermark)
			return;

		std::vector<std::string> active;

		if (GodMode::Get().Enabled)
			active.push_back(Menu::UseChinese() ? "无敌" : "God");

		if (InfiniteAmmo::Get().Enabled)
			active.push_back(Menu::UseChinese() ? "无限子弹" : "Ammo");

		if (Aimbot::Get().Enabled)
			active.push_back(Menu::UseChinese() ? "自瞄" : "Aimbot");

		if (Esp::Get().Enabled)
			active.push_back(Menu::UseChinese() ? "透视" : "ESP");

		if (Misc::Get().TimeScaleEnabled)
			active.push_back(Menu::UseChinese() ? "时间缩放" : "Timescale");

		UpdateLayout();

		const float lineStep = g_layout.lineHeight * 1.25f;
		float y = lineStep * 0.8f;
		const float x = g_layout.charWidth * 2.2f;

		Render::Text(Menu::UseChinese() ? "RDR 修改器" : "RDR TRAINER", x, y, 1.15f, Theme::Title, 0, true);
		y += lineStep * 1.1f;

		// Chinese mode but the active font is not proven to carry CJK glyphs (and not
		// forced): labels would be '?', so we show English and tell the user how to
		// get Chinese (try another font id, or enable "force Chinese render").
		if (Menu::IsChinese() && !Menu::UseChinese())
		{
			Render::Text("font has no CJK - showing English. try font id 0-12 or enable force-Chinese",
				x, y, 0.7f, Theme::ValueText, 0, true);
			y += lineStep;
		}

		if (g_settings.ShowFps)
		{
			Render::Text(Utils::Format("FPS %d", Application::Fps()), x, y, 0.85f, Theme::HintText, 0, true);
			y += lineStep;
		}

		if (g_settings.ShowMoney)
		{
			const std::string money = Money::Available()
				? Utils::Format(Menu::UseChinese() ? "金钱 %d" : "$ %d", Money::Current())
				: Utils::Format(Menu::UseChinese() ? "金钱: %s" : "money: %s", Money::Status());

			Render::Text(money, x, y, 0.85f, Money::Available() ? Theme::ValueText : Theme::HintText, 0, true);
			y += lineStep;
		}

		if (!active.empty())
		{
			std::string line;

			for (size_t index = 0; index < active.size(); ++index)
			{
				if (index > 0)
					line += "  ";

				line += active[index];
			}

			Render::Text(line, x, y, 0.9f, Theme::HealthGood, 0, true);
			y += lineStep;
		}

		const Actor target = Game::AimbotTarget();

		if (target != 0)
		{
			const ActorEntry* entry = Game::FindActor(target);

			if (entry != nullptr)
			{
				Render::Text(Utils::Format(Menu::UseChinese() ? "目标 %s  %.0fm" : "target %s  %.0fm",
					entry->name.c_str(), entry->distance),
					x, y, 0.85f, Theme::PresetColor(Esp::Get().ColorTarget), 0, true);
			}
		}
	}
}
