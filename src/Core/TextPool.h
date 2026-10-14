#pragma once

// RH_DrawText / RH_GetTextHeight keep the char* they are given (they are consumed by
// the render thread), so every string handed to the renderer must stay alive for a
// few frames. This pool owns those copies and recycles them.

namespace TextPool
{
	void BeginFrame();
	void EndFrame();
	void Shutdown();

	// Returns a pointer that stays valid for at least the next few frames.
	const char* Intern(const std::string& text);
}
