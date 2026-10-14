#pragma once

struct Color
{
	int r = 255;
	int g = 255;
	int b = 255;
	int a = 255;

	Color() = default;
	Color(int red, int green, int blue, int alpha = 255) : r(red), g(green), b(blue), a(alpha) {}

	Color WithAlpha(int alpha) const { return Color(r, g, b, alpha); }
	Color Darken(float factor) const
	{
		return Color((int)(r * factor), (int)(g * factor), (int)(b * factor), a);
	}
};

namespace Theme
{
	// ---- panel / menu -------------------------------------------------------
	inline const Color Panel          = Color(10, 8, 8, 235);
	inline const Color PanelBorder    = Color(120, 24, 24, 255);
	inline const Color Header         = Color(138, 22, 22, 245);
	inline const Color HeaderText     = Color(255, 236, 210, 255);
	inline const Color Title          = Color(255, 90, 60, 255);
	inline const Color ItemText       = Color(228, 224, 218, 255);
	inline const Color ItemTextOff    = Color(150, 146, 140, 255);
	inline const Color Selected       = Color(190, 40, 32, 230);
	inline const Color SelectedText   = Color(255, 250, 240, 255);
	inline const Color ValueText      = Color(255, 206, 110, 255);
	inline const Color HintText       = Color(170, 166, 158, 230);
	inline const Color Shadow         = Color(0, 0, 0, 190);
	inline const Color SubTitle       = Color(210, 205, 198, 255);

	inline const Color HealthBack     = Color(20, 18, 18, 210);
	inline const Color HealthGood     = Color(80, 220, 90, 255);
	inline const Color HealthBad      = Color(230, 60, 50, 255);

	// ---- ESP / actor colour presets ----------------------------------------
	struct Preset
	{
		const char* nameEn;
		const char* nameZh;
		Color       color;
	};

	inline const Preset Presets[] = {
		{ "Red",     "红色", Color(235, 55, 45) },
		{ "Orange",  "橙色", Color(245, 140, 35) },
		{ "Yellow",  "黄色", Color(245, 220, 60) },
		{ "Green",   "绿色", Color(80, 230, 90) },
		{ "Cyan",    "青色", Color(70, 225, 225) },
		{ "Blue",    "蓝色", Color(80, 150, 250) },
		{ "Purple",  "紫色", Color(190, 90, 245) },
		{ "Pink",    "粉色", Color(250, 110, 190) },
		{ "White",   "白色", Color(240, 240, 240) },
		{ "Gray",    "灰色", Color(150, 150, 150) },
	};

	inline const int PresetCount = (int)(sizeof(Presets) / sizeof(Presets[0]));

	inline Color PresetColor(int index, int alpha = 255)
	{
		if (index < 0 || index >= PresetCount)
			index = 0;

		return Presets[index].color.WithAlpha(alpha);
	}

	inline const char* PresetName(int index, bool chinese)
	{
		if (index < 0 || index >= PresetCount)
			index = 0;

		return chinese ? Presets[index].nameZh : Presets[index].nameEn;
	}
}
