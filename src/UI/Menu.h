#pragma once

struct MenuItem
{
	enum class Type
	{
		Header,
		Toggle,
		IntSlider,
		FloatSlider,
		Choice,
		Button,
		SubMenu,
		Keybind,
		Label
	};

	Type        type = Type::Header;
	std::string labelZh;
	std::string labelEn;

	bool*  boolValue = nullptr;
	int*   intValue = nullptr;
	float* floatValue = nullptr;

	int    minInt = 0;
	int    maxInt = 100;
	int    stepInt = 1;
	float  minFloat = 0.0f;
	float  maxFloat = 1.0f;
	float  stepFloat = 0.05f;
	int    decimals = 2;
	bool   integerFloat = false;      // display float sliders as integers
	const char* suffix = "";

	std::vector<std::pair<std::string, std::string>> choices;   // { chinese, english }
	std::function<void()> action;
	std::function<std::string()> valueText;

	// Font picker rows are drawn with their own font id and that font's own size.
	bool useOwnFontId = false;
	int  ownFontId = 0;
	int subMenuIndex = -1;
};

struct MenuPage
{
	std::string titleZh;
	std::string titleEn;
	std::vector<MenuItem> items;
};

namespace Menu
{
	struct Settings
	{
		int   MenuKey = KEY_F7;
		// 0 = Chinese (needs a game font that contains CJK glyphs),
		// 1 = English (pure ASCII, always renders).
		int   Language = 1;
		int   FontId = 0;
		bool  ForceChinese = false;   // render Chinese even if the font probe says no CJK
		float TextScale = 1.0f;
		float PanelScale = 1.0f;
		int   LineQuality = 1;
		bool  GameHudMessages = true;
		bool  ShowWatermark = true;
		bool  ShowFps = true;
		bool  ShowMoney = true;
	};

	Settings& Get();
	void LoadSettings();
	void SaveSettings();

	void Initialize();
	void Update();
	void Draw();
	void DrawHud();
	void Shutdown();

	void SetOpen(bool open);
	bool IsOpen();
	void Toggle();

	const char* Text(const char* chinese, const char* english);
	bool IsChinese();
	bool UseChinese();
}
