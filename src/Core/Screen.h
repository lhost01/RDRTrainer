#pragma once

// Screen / game window helper. The RedHook draw API works in 0.0 - 1.0 normalized
// coordinates, so pixel accurate layout needs the client size of the game window.

namespace Screen
{
	void  Update();          // cheap, re-queries the window every ~2 seconds
	HWND  Window();

	float Width();           // client area in pixels
	float Height();
	float Aspect();          // width / height
	bool  IsValid();

	// normalized <-> pixels
	float ToPixelX(float normalizedX);
	float ToPixelY(float normalizedY);
	float ToNormalizedX(float pixelX);
	float ToNormalizedY(float pixelY);
}
