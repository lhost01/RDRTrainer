#include "pch.h"

namespace
{
	bool g_down[Utils::KeyIdMax] = {};
	bool g_previous[Utils::KeyIdMax] = {};
	bool g_watched[Utils::KeyIdMax] = {};
	bool g_capturing = false;
	int  g_captured = KEY_NONE;

	void WatchInternal(int keyId)
	{
		if (keyId > 0 && keyId < Utils::KeyIdMax)
			g_watched[keyId] = true;
	}
}

namespace Input
{
	void Watch(int keyId)
	{
		WatchInternal(keyId);
	}

	void Update()
	{
		for (int keyId = 1; keyId < Utils::KeyIdMax; ++keyId)
		{
			if (!g_watched[keyId])
				continue;

			g_previous[keyId] = g_down[keyId];
			g_down[keyId] = Utils::IsKeyIdDownRaw(keyId);
		}

		if (g_capturing)
		{
			// Watch a wide set of candidates only while the user is binding a key.
			static const int kCandidates[] = {
				KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
				KEY_INSERT, KEY_DELETE, KEY_HOME, KEY_END, KEY_PAGE_UP, KEY_PAGE_DOWN,
				KEY_UP_ARROW, KEY_DOWN_ARROW, KEY_LEFT_ARROW, KEY_RIGHT_ARROW, KEY_SPACE, KEY_ESCAPE, KEY_TAB,
				KEY_NUMPAD_0, KEY_NUMPAD_1, KEY_NUMPAD_2, KEY_NUMPAD_3, KEY_NUMPAD_4,
				KEY_NUMPAD_5, KEY_NUMPAD_6, KEY_NUMPAD_7, KEY_NUMPAD_8, KEY_NUMPAD_9,
				KEY_NUMPAD_MINUS, KEY_NUMPAD_PLUS, KEY_NUMPAD_PERIOD, KEY_NUMPAD_FORWARD_SLASH,
				KEY_LEFT_CTRL, KEY_RIGHT_CTRL, KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT, KEY_LEFT_ALT, KEY_RIGHT_ALT,
				KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P,
				KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L,
				KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M,
				KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
			};

			for (int candidate : kCandidates)
			{
				if (!g_watched[candidate])
				{
					g_previous[candidate] = false;
					g_down[candidate] = Utils::IsKeyIdDownRaw(candidate);
					g_watched[candidate] = true;
				}
			}

			for (int index = 0; index < Utils::MouseKeyCount; ++index)
			{
				const int keyId = Utils::MouseKeyBase + index;

				if (!g_watched[keyId])
				{
					g_previous[keyId] = false;
					g_down[keyId] = Utils::IsKeyIdDownRaw(keyId);
					g_watched[keyId] = true;
				}
			}

			if (g_captured == KEY_NONE)
			{
				for (int keyId = 1; keyId < Utils::KeyIdMax; ++keyId)
				{
					if (!g_watched[keyId])
						continue;

					if (g_down[keyId] && !g_previous[keyId] && keyId != KEY_ESCAPE && keyId != KEY_ENTER)
					{
						g_captured = keyId;
						break;
					}
				}
			}
		}
	}

	bool Down(int keyId)
	{
		if (keyId <= 0 || keyId >= Utils::KeyIdMax)
			return false;

		return g_down[keyId];
	}

	bool Pressed(int keyId)
	{
		if (keyId <= 0 || keyId >= Utils::KeyIdMax)
			return false;

		return g_down[keyId] && !g_previous[keyId];
	}

	bool Released(int keyId)
	{
		if (keyId <= 0 || keyId >= Utils::KeyIdMax)
			return false;

		return !g_down[keyId] && g_previous[keyId];
	}

	void StartCapture()
	{
		g_capturing = true;
		g_captured = KEY_NONE;
	}

	void StopCapture()
	{
		g_capturing = false;
	}

	bool IsCapturing()
	{
		return g_capturing;
	}

	int CapturedKey()
	{
		const int keyId = g_captured;
		g_captured = KEY_NONE;
		return keyId;
	}

	bool CursorPosition(float& clientX, float& clientY)
	{
		HWND window = Screen::Window();

		if (window == nullptr)
			return false;

		POINT cursor = {};

		if (!GetCursorPos(&cursor))
			return false;

		if (!ScreenToClient(window, &cursor))
			return false;

		clientX = (float)cursor.x;
		clientY = (float)cursor.y;

		return true;
	}

	void MoveMouseRelative(float deltaX, float deltaY)
	{
		INPUT input = {};
		input.type = INPUT_MOUSE;
		input.mi.dx = (LONG)deltaX;
		input.mi.dy = (LONG)deltaY;
		input.mi.dwFlags = MOUSEEVENTF_MOVE;

		SendInput(1, &input, sizeof(INPUT));
	}
}
