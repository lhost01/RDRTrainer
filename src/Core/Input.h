#pragma once

namespace Input
{
	// Samples every watched key. Call once per frame, before anything reads key state.
	void Update();

	// Start watching a key id so Down()/Pressed() work for it.
	void Watch(int keyId);

	bool Down(int keyId);
	bool Pressed(int keyId);    // true only on the frame the key went down
	bool Released(int keyId);   // true only on the frame the key went up

	// Rebind helper: while active, returns the id of the first newly pressed key.
	void StartCapture();
	void StopCapture();
	bool IsCapturing();
	int  CapturedKey();         // KEY_NONE while nothing was pressed

	// Mouse cursor helpers (screen pixels relative to the game client area).
	bool  CursorPosition(float& clientX, float& clientY);
	void  MoveMouseRelative(float deltaX, float deltaY);
}
