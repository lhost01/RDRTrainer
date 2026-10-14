#pragma once

// RedHook's RH_DrawText takes a "font scale" whose unit is not documented.
// On this game 1.0 turns out to be enormous (normalized to the screen), which is
// what produced the "wall of question marks" report. Instead of guessing we measure
// the font once with RH_GetTextWidth and derive the scale that yields ~9 px per
// character, then let the user fine tune a multiplier around that.
//
// CJK note: most game fonts only carry Latin glyphs, so Chinese text collapses to
// '?' fallbacks. SupportsChineseGlyphs() probes the active font so the UI can fall
// back to English labels instead of covering the screen with question marks.

namespace Font
{
	void Update();                 // call once per frame, cheap (throttled internally)

	void  SetFontId(int fontId);
	int   FontId();

	void  SetUserScale(float multiplier);
	float UserScale();

	bool  IsCalibrated();

	// Absolute font scale for the active font (0 when not measured yet).
	float CalibratedScale();
	// Absolute font scale that suits the given font id (~9 px per character).
	float CalibratedScaleForFont(int fontId);

	// Final value handed to RH_DrawText: calibrated scale * user multiplier * extra,
	// always clamped so a single character can never take over the screen.
	float Scale(float extra = 1.0f);

	float CharWidthPx();
	float LineHeightPx();

	// True when the active font id actually contains CJK glyphs (probed at runtime).
	// Use this to decide whether Chinese labels can be rendered.
	bool  SupportsChineseGlyphs();

	// First font id (0..12) proven to carry CJK glyphs, or -1 if none. Lets the
	// menu auto-pick a Chinese-capable font so the user does not have to try them.
	int   FindChineseFont();
}
