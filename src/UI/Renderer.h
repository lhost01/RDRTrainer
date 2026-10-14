#pragma once

// Thin wrapper around the RedHook 2D drawing API.
// Everything here is expressed in screen pixels; the wrapper converts to the
// 0.0 - 1.0 normalized space RedHook expects.

namespace Render
{
	void Initialize();
	void Shutdown();

	void SetFontId(int fontId);
	int  FontId();
	void SetTextScale(float scale);       // user multiplier around the auto calibrated size
	float TextScale();
	void SetLineQuality(int quality);     // 0 = low, 1 = medium, 2 = high
	int  LineQuality();

	// Font sizing (auto calibrated, see Core/Font.h).
	bool  IsFontCalibrated();
	float EffectiveScale(float extra = 1.0f);
	float CharWidthPx();
	float LineHeightPx();
	float ScaleForFont(int fontId);

	// Draw list lifetime: BeginFrame -> draw -> EndFrame.
	void BeginFrame();
	void EndFrame();

	void Rect(float px, float py, float width, float height, const Color& color);
	void RectNorm(float x, float y, float width, float height, const Color& color);
	void Outline(float px, float py, float width, float height, float thickness, const Color& color);
	void Line(float x1, float y1, float x2, float y2, float thickness, const Color& color);
	void Circle(float centerX, float centerY, float radius, float thickness, const Color& color, int segments = 48);

	// scale is a multiplier applied on top of TextScale().
	void  Text(const std::string& text, float px, float py, float scale, const Color& color,
	           int alignment /* 0 = left, 1 = center, 2 = right */, bool shadow = true);
	// Draws with an explicit font id and an absolute font scale (used by the font picker).
	void  TextWithFont(const std::string& text, float px, float py, float absoluteScale, const Color& color,
	                   int alignment, int fontId, bool shadow = true);
	float TextWidth(const std::string& text, float scale);
	float TextHeight(const std::string& text, float scale);
}
