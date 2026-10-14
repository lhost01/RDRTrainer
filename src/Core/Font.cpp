#include "pch.h"

namespace
{
	constexpr float kTargetCharWidthPx = 9.0f;    // one character should be about this wide
	constexpr float kFallbackScale = 0.025f;      // used until the first measurement succeeds
	constexpr int   kMaxFontId = 12;
	constexpr int   kFontCount = kMaxFontId + 1;

	int   g_fontId = 0;
	float g_userScale = 1.0f;
	float g_scaleByFont[kFontCount] = {};
	float g_charWidthNormAtOne = 0.0f;   // per character, normalized, measured at scale 1.0
	float g_lineHeightNorm = 0.0f;       // cached RH_GetTextHeight result
	uint64_t g_nextProbe = 0;
	float g_probedScreenWidth = 0.0f;
	bool  g_loggedCalibration = false;
	bool  g_supportsChinese[kFontCount] = {};   // probed per font id

	float MeasureScaleForFont(int fontId)
	{
		if (fontId < 0 || fontId > kMaxFontId)
			fontId = 0;

		if (g_scaleByFont[fontId] > 0.0f)
			return g_scaleByFont[fontId];

		if (!Screen::IsValid())
			return 0.0f;

		// Ten characters gives a stable average width.
		const float widthAtOne = RH_GetTextWidth("MMMMMMMMMM", fontId, 1.0f);

		if (!std::isfinite(widthAtOne) || widthAtOne <= 0.0f)
			return 0.0f;

		const float perCharNorm = widthAtOne / 10.0f;
		const float perCharPx = perCharNorm * Screen::Width();

		if (!(perCharPx > 0.0001f) || perCharPx > 100000.0f)
			return 0.0f;

		const float scale = Utils::Clamp(kTargetCharWidthPx / perCharPx, 0.00005f, 16.0f);

		g_scaleByFont[fontId] = scale;

		if (fontId == g_fontId)
			g_charWidthNormAtOne = perCharNorm;

		Log::Info("Font: id %d measures %.3f px/char at scale 1.0 -> using scale %.5f",
			fontId, perCharPx, scale);

		return scale;
	}

	// Probe a single CJK character "永" against single ASCII 'M' and '?'. Deliberately
	// strict: a false "supports" would render a wall of '?', so we only accept a font
	// when the CJK glyph is clearly a real full-width glyph, never a fallback.
	bool ProbeChineseSupport(int fontId)
	{
		if (fontId < 0 || fontId > kMaxFontId)
			fontId = 0;

		if (g_supportsChinese[fontId])
			return true;

		if (!Screen::IsValid())
			return false;

		const float wYong = RH_GetTextWidth("\xE6\xB0\xB8", fontId, 1.0f); // single "永"
		const float wM = RH_GetTextWidth("M", fontId, 1.0f);
		const float wQ = RH_GetTextWidth("?", fontId, 1.0f);

		if (!(wYong > 0.0f) || !(wM > 0.0f) || !(wQ > 0.0f))
			return false;

		// Reject byte-fallback: if RedHook treats the 3 UTF-8 bytes of "永" as 3
		// separate missing-glyph '?', wYong ~= 3*wQ. That is NOT real CJK support.
		if (wYong >= wQ * 2.7f && wYong <= wQ * 3.3f)
			return false;

		// A real CJK glyph is full-width and clearly wider than half-width ASCII 'M'.
		// If "永" was replaced by a single '?', wYong ~= wM and this fails.
		if (wYong < wM * 1.4f)
			return false;

		g_supportsChinese[fontId] = true;

		Log::Info("Font: id %d carries CJK glyphs (yong=%.4f M=%.4f ?=%.4f)",
			fontId, wYong, wM, wQ);

		return true;
	}
}

namespace Font
{
	void SetFontId(int fontId)
	{
		g_fontId = Utils::ClampInt(fontId, 0, kMaxFontId);
		g_charWidthNormAtOne = 0.0f;
		g_nextProbe = 0;

		if (g_scaleByFont[g_fontId] > 0.0f)
		{
			// Already measured earlier, rebuild the normalized value from it.
			const float scale = g_scaleByFont[g_fontId];
			g_charWidthNormAtOne = kTargetCharWidthPx / (scale * Screen::Width());
		}
	}

	int FontId() { return g_fontId; }

	void SetUserScale(float multiplier) { g_userScale = Utils::Clamp(multiplier, 0.2f, 4.0f); }
	float UserScale() { return g_userScale; }

	bool IsCalibrated() { return g_scaleByFont[g_fontId] > 0.0f; }

	float CalibratedScale()
	{
		return g_scaleByFont[g_fontId];
	}

	float CalibratedScaleForFont(int fontId)
	{
		const float scale = MeasureScaleForFont(fontId);

		return scale > 0.0f ? scale : kFallbackScale;
	}

	float Scale(float extra)
	{
		const float calibrated = MeasureScaleForFont(g_fontId);
		const float base = (calibrated > 0.0f) ? calibrated : kFallbackScale;
		float scale = base * g_userScale * extra;

		// Hard ceiling: one character may never be wider than 1/70 of the screen.
		if (g_charWidthNormAtOne > 0.0f)
		{
			const float maxScale = 1.0f / (70.0f * g_charWidthNormAtOne);

			if (scale > maxScale)
				scale = maxScale;
		}

		return Utils::Clamp(scale, 0.00005f, 6.0f);
	}

	float CharWidthPx()
	{
		if (g_charWidthNormAtOne > 0.0f)
			return g_charWidthNormAtOne * Scale() * Screen::Width();

		return kTargetCharWidthPx * g_userScale;
	}

	float LineHeightPx()
	{
		const float charWidth = CharWidthPx();

		if (g_lineHeightNorm > 0.0f)
		{
			const float lineHeight = g_lineHeightNorm * Screen::Height();

			// Only trust the measured height when it is plausible for the current width.
			if (lineHeight > charWidth * 0.9f && lineHeight < charWidth * 3.5f)
				return lineHeight;
		}

		return charWidth * 1.9f;
	}

	bool SupportsChineseGlyphs()
	{
		// Best effort probe; stays pessimistic until a font is proven capable, so the
		// UI falls back to English instead of a wall of '?'.
		return ProbeChineseSupport(g_fontId);
	}

	int FindChineseFont()
	{
		for (int id = 0; id <= kMaxFontId; ++id)
		{
			if (ProbeChineseSupport(id))
				return id;
		}

		return -1;
	}

	void Update()
	{
		if (!Screen::IsValid())
			return;

		const uint64_t now = GetTickCount64();

		if (now < g_nextProbe)
			return;

		g_nextProbe = now + 1000;

		// A resolution change invalidates the measurements.
		if (fabsf(Screen::Width() - g_probedScreenWidth) > 16.0f)
		{
			for (int index = 0; index < kFontCount; ++index)
			{
				g_scaleByFont[index] = 0.0f;
				g_supportsChinese[index] = false;
			}

			g_charWidthNormAtOne = 0.0f;
			g_lineHeightNorm = 0.0f;
		}

		g_probedScreenWidth = Screen::Width();

		const float scale = MeasureScaleForFont(g_fontId);

		ProbeChineseSupport(g_fontId);

		RH_GetTextHeight("Ag", g_fontId, Scale(1.0f), &g_lineHeightNorm);

		if (!g_loggedCalibration && scale > 0.0f)
		{
			g_loggedCalibration = true;
			Log::Info("Font: calibrated, character = %.1f px, line height = %.1f px, cjk = %s",
				CharWidthPx(), LineHeightPx(), g_supportsChinese[g_fontId] ? "yes" : "no");
		}
	}
}
