#include "pch.h"

namespace
{
	Esp::Settings g_settings;

	Color ColorForCategory(const ActorEntry& entry)
	{
		const int alpha = 235;

		switch (entry.category)
		{
			case ActorCategory::Animal:
				return Theme::PresetColor(g_settings.ColorAnimal, alpha);

			case ActorCategory::Friendly:
			case ActorCategory::Player:
			case ActorCategory::LocalPlayer:
				return Theme::PresetColor(g_settings.ColorFriendly, alpha);

			case ActorCategory::Hostile:
			default:
				return Theme::PresetColor(g_settings.ColorHostile, alpha);
		}
	}

	bool ShouldDraw(const ActorEntry& entry)
	{
		if (!entry.valid)
			return false;

		if (entry.isLocalPlayer && !g_settings.ShowLocalPlayer)
			return false;

		if (entry.dead && !g_settings.ShowDead)
			return false;

		switch (entry.category)
		{
			case ActorCategory::Animal:
				if (!g_settings.ShowAnimals)
					return false;
				break;

			case ActorCategory::Friendly:
			case ActorCategory::Player:
				if (!g_settings.ShowFriendly)
					return false;
				break;

			case ActorCategory::Vehicle:
				return false;

			default:
				break;
		}

		return entry.distance <= g_settings.MaxDistance;
	}

	void DrawBox(float left, float top, float width, float height, float thickness, const Color& color)
	{
		if (!g_settings.Corners)
		{
			Render::Outline(left, top, width, height, thickness, color);
			return;
		}

		const float corner = std::max(6.0f, std::min(width, height) * 0.25f);

		Render::Rect(left, top, corner, thickness, color);
		Render::Rect(left, top, thickness, corner, color);
		Render::Rect(left + width - corner, top, corner, thickness, color);
		Render::Rect(left + width - thickness, top, thickness, corner, color);
		Render::Rect(left, top + height - thickness, corner, thickness, color);
		Render::Rect(left, top + height - corner, thickness, corner, color);
		Render::Rect(left + width - corner, top + height - thickness, corner, thickness, color);
		Render::Rect(left + width - thickness, top + height - corner, thickness, corner, color);
	}

	void DrawTargetMarker(float centerX, float top, float size, const Color& color)
	{
		const float half = size * 0.5f;
		const float thickness = std::max(1.5f, Esp::Get().LineThickness);
		const float base = top - size * 1.5f;

		Render::Line(centerX, base, centerX - half, base + half, thickness, color);
		Render::Line(centerX - half, base + half, centerX, base + size, thickness, color);
		Render::Line(centerX, base + size, centerX + half, base + half, thickness, color);
		Render::Line(centerX + half, base + half, centerX, base, thickness, color);
	}
}

namespace Esp
{
	Settings& Get() { return g_settings; }

	void LoadSettings()
	{
		g_settings.Enabled = Config::GetBool("Esp.Enabled", g_settings.Enabled);
		g_settings.Hotkey = Config::GetInt("Esp.Hotkey", g_settings.Hotkey);
		g_settings.Box = Config::GetBool("Esp.Box", g_settings.Box);
		g_settings.Corners = Config::GetBool("Esp.Corners", g_settings.Corners);
		g_settings.Name = Config::GetBool("Esp.Name", g_settings.Name);
		g_settings.Distance = Config::GetBool("Esp.Distance", g_settings.Distance);
		g_settings.Health = Config::GetBool("Esp.Health", g_settings.Health);
		g_settings.Snapline = Config::GetBool("Esp.Snapline", g_settings.Snapline);
		g_settings.HighlightTarget = Config::GetBool("Esp.HighlightTarget", g_settings.HighlightTarget);
		g_settings.ShowDead = Config::GetBool("Esp.ShowDead", g_settings.ShowDead);
		g_settings.ShowLocalPlayer = Config::GetBool("Esp.ShowLocalPlayer", g_settings.ShowLocalPlayer);
		g_settings.ShowAnimals = Config::GetBool("Esp.ShowAnimals", g_settings.ShowAnimals);
		g_settings.ShowFriendly = Config::GetBool("Esp.ShowFriendly", g_settings.ShowFriendly);
		g_settings.MaxDistance = Config::GetFloat("Esp.MaxDistance", g_settings.MaxDistance);
		g_settings.LineThickness = Config::GetFloat("Esp.LineThickness", g_settings.LineThickness);
		g_settings.LabelScale = Config::GetFloat("Esp.LabelScale", g_settings.LabelScale);
		g_settings.HeadOffset = Config::GetFloat("Esp.HeadOffset", g_settings.HeadOffset);
		g_settings.ColorHostile = Config::GetInt("Esp.ColorHostile", g_settings.ColorHostile);
		g_settings.ColorAnimal = Config::GetInt("Esp.ColorAnimal", g_settings.ColorAnimal);
		g_settings.ColorFriendly = Config::GetInt("Esp.ColorFriendly", g_settings.ColorFriendly);
		g_settings.ColorTarget = Config::GetInt("Esp.ColorTarget", g_settings.ColorTarget);
		g_settings.HostilityMode = Config::GetInt("Esp.HostilityMode", g_settings.HostilityMode);
		g_settings.ShowWatermark = Config::GetBool("Esp.ShowWatermark", g_settings.ShowWatermark);

		Game::SetHeadOffset(g_settings.HeadOffset);
		Game::SetHostilityMode(g_settings.HostilityMode);
	}

	void SaveSettings()
	{
		Config::SetBool("Esp.Enabled", g_settings.Enabled);
		Config::SetInt("Esp.Hotkey", g_settings.Hotkey);
		Config::SetBool("Esp.Box", g_settings.Box);
		Config::SetBool("Esp.Corners", g_settings.Corners);
		Config::SetBool("Esp.Name", g_settings.Name);
		Config::SetBool("Esp.Distance", g_settings.Distance);
		Config::SetBool("Esp.Health", g_settings.Health);
		Config::SetBool("Esp.Snapline", g_settings.Snapline);
		Config::SetBool("Esp.HighlightTarget", g_settings.HighlightTarget);
		Config::SetBool("Esp.ShowDead", g_settings.ShowDead);
		Config::SetBool("Esp.ShowLocalPlayer", g_settings.ShowLocalPlayer);
		Config::SetBool("Esp.ShowAnimals", g_settings.ShowAnimals);
		Config::SetBool("Esp.ShowFriendly", g_settings.ShowFriendly);
		Config::SetFloat("Esp.MaxDistance", g_settings.MaxDistance);
		Config::SetFloat("Esp.LineThickness", g_settings.LineThickness);
		Config::SetFloat("Esp.LabelScale", g_settings.LabelScale);
		Config::SetFloat("Esp.HeadOffset", g_settings.HeadOffset);
		Config::SetInt("Esp.ColorHostile", g_settings.ColorHostile);
		Config::SetInt("Esp.ColorAnimal", g_settings.ColorAnimal);
		Config::SetInt("Esp.ColorFriendly", g_settings.ColorFriendly);
		Config::SetInt("Esp.ColorTarget", g_settings.ColorTarget);
		Config::SetInt("Esp.HostilityMode", g_settings.HostilityMode);
		Config::SetBool("Esp.ShowWatermark", g_settings.ShowWatermark);

		Game::SetHeadOffset(g_settings.HeadOffset);
		Game::SetHostilityMode(g_settings.HostilityMode);
	}

	void Draw()
	{
		if (!Screen::IsValid())
			return;

		if (Aimbot::Get().DrawFov)
		{
			Render::Circle(Screen::Width() * 0.5f, Screen::Height() * 0.5f,
				Aimbot::Get().FovPixels, 1.4f, Theme::Title.WithAlpha(140));
		}

		if (!g_settings.Enabled || !Game::IsLocalPlayerValid())
			return;

		const float width = Screen::Width();
		const float height = Screen::Height();
		const float thickness = Utils::Clamp(g_settings.LineThickness, 1.0f, 6.0f);
		const Actor aimbotTarget = Game::AimbotTarget();

		for (const ActorEntry& entry : Game::Actors())
		{
			if (!ShouldDraw(entry) || !entry.onScreen || !entry.headValid)
				continue;

			const float feetX = entry.screenX * width;
			const float feetY = entry.screenY * height;
			const float headY = entry.headScreenY * height;
			const float boxHeight = fabsf(feetY - headY);

			if (boxHeight < 4.0f)
				continue;

			const bool isTarget = (aimbotTarget != 0 && entry.actor == aimbotTarget);
			Color color = ColorForCategory(entry);

			if (isTarget && g_settings.HighlightTarget)
				color = Theme::PresetColor(g_settings.ColorTarget, 255);

			const float boxWidth = boxHeight * 0.52f;
			const float left = feetX - boxWidth * 0.5f;
			const float top = std::min(feetY, headY);

			if (g_settings.Box)
				DrawBox(left, top, boxWidth, boxHeight, thickness, color);

			if (g_settings.Health)
			{
				const float ratio = entry.HealthRatio();
				const float barX = left - 7.0f;
				const float barWidth = 3.5f;

				Render::Rect(barX, top, barWidth, boxHeight, Theme::HealthBack);
				Render::Rect(barX, top + boxHeight * (1.0f - ratio), barWidth, boxHeight * ratio,
					ratio > 0.35f ? Theme::HealthGood : Theme::HealthBad);
			}

			if (g_settings.Snapline)
				Render::Line(width * 0.5f, height - 2.0f, feetX, feetY, 1.2f, color.WithAlpha(150));

			if (g_settings.Name && !entry.name.empty())
			{
				std::string label = entry.name;
				const size_t underscore = label.rfind('_');

				if (underscore != std::string::npos && label.size() > 6)
					label = label.substr(underscore + 1);

				Render::Text(label, feetX, top - 18.0f, g_settings.LabelScale, color, 1, true);
			}

			if (g_settings.Distance)
			{
				Render::Text(Utils::Format("%.0fm", entry.distance), feetX, top + boxHeight + 2.0f,
					g_settings.LabelScale * 0.9f, Theme::ItemText, 1, true);
			}

			if (isTarget && g_settings.HighlightTarget)
				DrawTargetMarker(feetX, top, 8.0f, color);
		}
	}
}
