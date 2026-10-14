#include "pch.h"

namespace
{
	int   g_lineQuality = 1;

	float Clamp01(float value)
	{
		return Utils::Clamp(value, 0.0f, 1.0f);
	}

	TextAlignment ToAlignment(int alignment)
	{
		if (alignment == 1)
			return TextAlignment_Center;

		if (alignment == 2)
			return TextAlignment_Right;

		return TextAlignment_Left;
	}

	void DrawTextInternal(const std::string& text, float px, float py, float fontScale, int fontId,
		const Color& color, int alignment, bool shadow)
	{
		if (text.empty() || !Screen::IsValid())
			return;

		const float x = Screen::ToNormalizedX(px);
		const float y = Screen::ToNormalizedY(py);

		if (shadow)
		{
			const float offsetX = Screen::ToNormalizedX(px + 1.0f);
			const float offsetY = Screen::ToNormalizedY(py + 1.0f);

			RH_DrawText(TextPool::Intern(text), offsetX, offsetY, fontId, fontScale,
				Theme::Shadow.r, Theme::Shadow.g, Theme::Shadow.b, Theme::Shadow.a, ToAlignment(alignment));
		}

		RH_DrawText(TextPool::Intern(text), x, y, fontId, fontScale,
			color.r, color.g, color.b, color.a, ToAlignment(alignment));
	}
}

namespace Render
{
	void Initialize()
	{
	}

	void Shutdown()
	{
	}

	void SetFontId(int fontId) { Font::SetFontId(fontId); }
	int  FontId() { return Font::FontId(); }
	void SetTextScale(float scale) { Font::SetUserScale(scale); }
	float TextScale() { return Font::UserScale(); }
	void SetLineQuality(int quality) { g_lineQuality = Utils::ClampInt(quality, 0, 2); }
	int  LineQuality() { return g_lineQuality; }
	bool  IsFontCalibrated() { return Font::IsCalibrated(); }
	float EffectiveScale(float extra) { return Font::Scale(extra); }
	float CharWidthPx() { return Font::CharWidthPx(); }
	float LineHeightPx() { return Font::LineHeightPx(); }
	float ScaleForFont(int fontId) { return Font::CalibratedScaleForFont(fontId); }

	void BeginFrame()
	{
		TextPool::BeginFrame();
		Font::Update();
	}

	void EndFrame()
	{
		TextPool::EndFrame();
	}

	void RectNorm(float x, float y, float width, float height, const Color& color)
	{
		if (width <= 0.0f || height <= 0.0f || color.a <= 0)
			return;

		RH_DrawRect(Clamp01(x), Clamp01(y), width, height, color.r, color.g, color.b, color.a);
	}

	void Rect(float px, float py, float width, float height, const Color& color)
	{
		if (width <= 0.0f || height <= 0.0f || color.a <= 0 || !Screen::IsValid())
			return;

		RectNorm(Screen::ToNormalizedX(px), Screen::ToNormalizedY(py),
			Screen::ToNormalizedX(width), Screen::ToNormalizedY(height), color);
	}

	void Outline(float px, float py, float width, float height, float thickness, const Color& color)
	{
		if (width <= 0.0f || height <= 0.0f || thickness <= 0.0f)
			return;

		Rect(px, py, width, thickness, color);                      // top
		Rect(px, py + height - thickness, width, thickness, color); // bottom
		Rect(px, py + thickness, thickness, height - 2.0f * thickness, color);
		Rect(px + width - thickness, py + thickness, thickness, height - 2.0f * thickness, color);
	}

	void Line(float x1, float y1, float x2, float y2, float thickness, const Color& color)
	{
		const float dx = x2 - x1;
		const float dy = y2 - y1;
		const float length = sqrtf(dx * dx + dy * dy);

		if (length < 1.0f || thickness <= 0.0f)
			return;

		const int maxSteps = (g_lineQuality == 0) ? 48 : (g_lineQuality == 1 ? 128 : 320);
		const float stepSize = std::max(thickness * 0.75f, length / (float)maxSteps);
		const int steps = Utils::ClampInt((int)(length / stepSize), 1, maxSteps);
		const float half = thickness * 0.5f;

		for (int index = 0; index <= steps; ++index)
		{
			const float t = (float)index / (float)steps;
			const float x = x1 + dx * t;
			const float y = y1 + dy * t;

			Rect(x - half, y - half, thickness, thickness, color);
		}
	}

	void Circle(float centerX, float centerY, float radius, float thickness, const Color& color, int segments)
	{
		segments = Utils::ClampInt(segments, 8, 128);

		float previousX = centerX + radius;
		float previousY = centerY;

		for (int index = 1; index <= segments; ++index)
		{
			const float angle = (float)index / (float)segments * 6.2831853f;
			const float x = centerX + cosf(angle) * radius;
			const float y = centerY + sinf(angle) * radius;

			Line(previousX, previousY, x, y, thickness, color);

			previousX = x;
			previousY = y;
		}
	}

	void Text(const std::string& text, float px, float py, float scale, const Color& color, int alignment, bool shadow)
	{
		DrawTextInternal(text, px, py, Font::Scale(scale), Font::FontId(), color, alignment, shadow);
	}

	void TextWithFont(const std::string& text, float px, float py, float absoluteScale, const Color& color,
		int alignment, int fontId, bool shadow)
	{
		DrawTextInternal(text, px, py, Utils::Clamp(absoluteScale, 0.00005f, 6.0f), fontId, color, alignment, shadow);
	}

	float TextWidth(const std::string& text, float scale)
	{
		if (text.empty() || !Screen::IsValid())
			return 0.0f;

		const float fontScale = Font::Scale(scale);
		const float normalized = RH_GetTextWidth(TextPool::Intern(text), Font::FontId(), fontScale);

		return normalized * Screen::Width();
	}

	float TextHeight(const std::string& text, float scale)
	{
		return Font::LineHeightPx() * scale;
	}
}
