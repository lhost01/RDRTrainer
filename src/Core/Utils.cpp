#include "pch.h"

namespace
{
	HMODULE g_module = nullptr;
	std::filesystem::path g_moduleDir;
	bool g_moduleDirResolved = false;

	const char* const kMouseNames[Utils::MouseKeyCount] = { "MOUSE1", "MOUSE2", "MOUSE3", "MOUSE4", "MOUSE5" };
	const int         kMouseVirtualKeys[Utils::MouseKeyCount] = {
		VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2
	};
}

namespace Utils
{
	void SetModuleHandle(HMODULE module)
	{
		g_module = module;
		g_moduleDirResolved = false;
	}

	HMODULE ModuleHandle()
	{
		return g_module;
	}

	const std::filesystem::path& ModuleDir()
	{
		if (!g_moduleDirResolved)
		{
			wchar_t buffer[MAX_PATH * 2] = {};
			GetModuleFileNameW(g_module, buffer, (DWORD)std::size(buffer));

			std::filesystem::path path(buffer);
			g_moduleDir = path.parent_path();
			g_moduleDirResolved = true;
		}

		return g_moduleDir;
	}

	std::filesystem::path ModuleFile(const wchar_t* fileName)
	{
		return ModuleDir() / fileName;
	}

	bool WriteTextFile(const std::filesystem::path& file, const std::string& text)
	{
		FILE* handle = nullptr;

		if (_wfopen_s(&handle, file.c_str(), L"wb") != 0 || handle == nullptr)
			return false;

		const size_t written = fwrite(text.data(), 1, text.size(), handle);
		fclose(handle);

		return written == text.size();
	}

	bool ReadTextFile(const std::filesystem::path& file, std::string& out)
	{
		FILE* handle = nullptr;

		if (_wfopen_s(&handle, file.c_str(), L"rb") != 0 || handle == nullptr)
			return false;

		fseek(handle, 0, SEEK_END);
		const long size = ftell(handle);
		fseek(handle, 0, SEEK_SET);

		if (size <= 0)
		{
			fclose(handle);
			return false;
		}

		out.resize((size_t)size);
		const size_t read = fread(out.data(), 1, (size_t)size, handle);
		fclose(handle);

		out.resize(read);
		return true;
	}

	std::string Format(const char* format, ...)
	{
		char buffer[1024] = {};

		va_list args;
		va_start(args, format);
		_vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
		va_end(args);

		return std::string(buffer);
	}

	bool IsReadable(const void* address, size_t size)
	{
		if (address == nullptr || size == 0)
			return false;

		MEMORY_BASIC_INFORMATION info = {};

		if (VirtualQuery(address, &info, sizeof(info)) == 0)
			return false;

		if (info.State != MEM_COMMIT)
			return false;

		if (info.Protect & (PAGE_NOACCESS | PAGE_GUARD))
			return false;

		const uintptr_t start = (uintptr_t)address;
		const uintptr_t end = (uintptr_t)info.BaseAddress + info.RegionSize;

		return start + size <= end;
	}

	bool IsWritable(const void* address, size_t size)
	{
		if (address == nullptr || size == 0)
			return false;

		MEMORY_BASIC_INFORMATION info = {};

		if (VirtualQuery(address, &info, sizeof(info)) == 0)
			return false;

		if (info.State != MEM_COMMIT)
			return false;

		// Only allow genuinely writable pages. ReadOnly / NoAccess / Guard would
		// crash on write; ExecuteRead is usually code and should not be touched.
		const DWORD prot = info.Protect;

		if (!(prot & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY)))
			return false;

		if (prot & (PAGE_NOACCESS | PAGE_GUARD))
			return false;

		const uintptr_t start = (uintptr_t)address;
		const uintptr_t end = (uintptr_t)info.BaseAddress + info.RegionSize;

		return start + size <= end;
	}

	float NormalizeHeading(float degrees)
	{
		while (degrees < 0.0f)
			degrees += 360.0f;

		while (degrees >= 360.0f)
			degrees -= 360.0f;

		return degrees;
	}

	float HeadingDelta(float from, float to)
	{
		float delta = NormalizeHeading(to) - NormalizeHeading(from);

		if (delta > 180.0f)
			delta -= 360.0f;
		else if (delta < -180.0f)
			delta += 360.0f;

		return delta;
	}

	float Clamp(float value, float min, float max)
	{
		if (value < min)
			return min;

		if (value > max)
			return max;

		return value;
	}

	int ClampInt(int value, int min, int max)
	{
		if (value < min)
			return min;

		if (value > max)
			return max;

		return value;
	}

	int ScanCodeToVirtualKey(int scanCode)
	{
		if (scanCode <= 0 || scanCode >= 0xFF)
			return 0;

		// Extended keys (arrows, insert/delete, numpad enter, right ctrl/alt...) need the E0 flag.
		switch (scanCode)
		{
			case KEY_HOME:
			case KEY_UP_ARROW:
			case KEY_PAGE_UP:
			case KEY_LEFT_ARROW:
			case KEY_RIGHT_ARROW:
			case KEY_END:
			case KEY_DOWN_ARROW:
			case KEY_PAGE_DOWN:
			case KEY_INSERT:
			case KEY_DELETE:
			case KEY_NUMPAD_ENTER:
			case KEY_RIGHT_CTRL:
			case KEY_RIGHT_ALT:
				return MapVirtualKeyA((UINT)scanCode, MAPVK_VSC_TO_VK);

			default:
				return MapVirtualKeyA((UINT)scanCode, MAPVK_VSC_TO_VK_EX);
		}
	}

	bool IsKeyIdDownRaw(int keyId)
	{
		if (keyId >= MouseKeyBase && keyId < MouseKeyBase + MouseKeyCount)
			return (GetAsyncKeyState(kMouseVirtualKeys[keyId - MouseKeyBase]) & 0x8000) != 0;

		const int virtualKey = ScanCodeToVirtualKey(keyId);

		if (virtualKey == 0)
			return false;

		return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
	}

	const char* KeyIdName(int keyId)
	{
		if (keyId >= MouseKeyBase && keyId < MouseKeyBase + MouseKeyCount)
			return kMouseNames[keyId - MouseKeyBase];

		struct Entry { int code; const char* name; };

		static const Entry kNames[] = {
			{ KEY_ESCAPE, "ESC" }, { KEY_TAB, "TAB" }, { KEY_SPACE, "SPACE" }, { KEY_ENTER, "ENTER" },
			{ KEY_BACKSPACE, "BACKSPACE" }, { KEY_INSERT, "INSERT" }, { KEY_DELETE, "DELETE" },
			{ KEY_HOME, "HOME" }, { KEY_END, "END" }, { KEY_PAGE_UP, "PGUP" }, { KEY_PAGE_DOWN, "PGDN" },
			{ KEY_UP_ARROW, "UP" }, { KEY_DOWN_ARROW, "DOWN" }, { KEY_LEFT_ARROW, "LEFT" }, { KEY_RIGHT_ARROW, "RIGHT" },
			{ KEY_LEFT_CTRL, "LCTRL" }, { KEY_RIGHT_CTRL, "RCTRL" }, { KEY_LEFT_SHIFT, "LSHIFT" },
			{ KEY_RIGHT_SHIFT, "RSHIFT" }, { KEY_LEFT_ALT, "LALT" }, { KEY_RIGHT_ALT, "RALT" },
			{ KEY_MINUS, "MINUS" }, { KEY_EQUAL, "EQUAL" }, { KEY_COMMA, "COMMA" }, { KEY_PERIOD, "PERIOD" },
			{ KEY_FORWARD_SLASH, "SLASH" }, { KEY_SEMICOLON, "SEMICOLON" }, { KEY_APOSTROPHE, "APOSTROPHE" },
			{ KEY_GRAVE, "GRAVE" }, { KEY_LEFT_SQUARE_BRACKET, "LBRACKET" }, { KEY_RIGHT_SQUARE_BRACKET, "RBRACKET" },
			{ KEY_BACKSLASH, "BACKSLASH" }, { KEY_ASTERISK, "NUMPAD_STAR" }, { KEY_CAPS_LOCK, "CAPSLOCK" },
			{ KEY_NUMLOCK, "NUMLOCK" }, { KEY_SCROLL_LOCK, "SCROLLLOCK" },
			{ KEY_F1, "F1" }, { KEY_F2, "F2" }, { KEY_F3, "F3" }, { KEY_F4, "F4" }, { KEY_F5, "F5" }, { KEY_F6, "F6" },
			{ KEY_F7, "F7" }, { KEY_F8, "F8" }, { KEY_F9, "F9" }, { KEY_F10, "F10" }, { KEY_F11, "F11" }, { KEY_F12, "F12" },
			{ KEY_NUMPAD_0, "NUMPAD0" }, { KEY_NUMPAD_1, "NUMPAD1" }, { KEY_NUMPAD_2, "NUMPAD2" }, { KEY_NUMPAD_3, "NUMPAD3" },
			{ KEY_NUMPAD_4, "NUMPAD4" }, { KEY_NUMPAD_5, "NUMPAD5" }, { KEY_NUMPAD_6, "NUMPAD6" }, { KEY_NUMPAD_7, "NUMPAD7" },
			{ KEY_NUMPAD_8, "NUMPAD8" }, { KEY_NUMPAD_9, "NUMPAD9" }, { KEY_NUMPAD_MINUS, "NUMPAD_MINUS" },
			{ KEY_NUMPAD_PLUS, "NUMPAD_PLUS" }, { KEY_NUMPAD_PERIOD, "NUMPAD_PERIOD" },
			{ KEY_NUMPAD_FORWARD_SLASH, "NUMPAD_SLASH" },
			{ KEY_1, "1" }, { KEY_2, "2" }, { KEY_3, "3" }, { KEY_4, "4" }, { KEY_5, "5" },
			{ KEY_6, "6" }, { KEY_7, "7" }, { KEY_8, "8" }, { KEY_9, "9" }, { KEY_0, "0" },
			{ KEY_Q, "Q" }, { KEY_W, "W" }, { KEY_E, "E" }, { KEY_R, "R" }, { KEY_T, "T" }, { KEY_Y, "Y" },
			{ KEY_U, "U" }, { KEY_I, "I" }, { KEY_O, "O" }, { KEY_P, "P" },
			{ KEY_A, "A" }, { KEY_S, "S" }, { KEY_D, "D" }, { KEY_F, "F" }, { KEY_G, "G" }, { KEY_H, "H" },
			{ KEY_J, "J" }, { KEY_K, "K" }, { KEY_L, "L" },
			{ KEY_Z, "Z" }, { KEY_X, "X" }, { KEY_C, "C" }, { KEY_V, "V" }, { KEY_B, "B" }, { KEY_N, "N" }, { KEY_M, "M" },
		};

		for (const Entry& entry : kNames)
		{
			if (entry.code == keyId)
				return entry.name;
		}

		return "NONE";
	}

	int KeyIdFromName(const char* name)
	{
		if (name == nullptr || name[0] == '\0')
			return KEY_NONE;

		for (int keyId = 0; keyId < KeyIdMax; ++keyId)
		{
			if (_stricmp(name, KeyIdName(keyId)) == 0)
				return keyId;
		}

		return KEY_NONE;
	}
}
