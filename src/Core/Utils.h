#pragma once

namespace Utils
{
	// Module / filesystem -----------------------------------------------------
	void SetModuleHandle(HMODULE module);
	HMODULE ModuleHandle();
	const std::filesystem::path& ModuleDir();
	std::filesystem::path ModuleFile(const wchar_t* fileName);

	bool WriteTextFile(const std::filesystem::path& file, const std::string& text);
	bool ReadTextFile(const std::filesystem::path& file, std::string& out);

	// Strings -----------------------------------------------------------------
	std::string Format(const char* format, ...);

	// Memory ------------------------------------------------------------------
	// Returns true when [address, address + size) is committed and readable.
	bool IsReadable(const void* address, size_t size);
	// Returns true when [address, address + size) is committed AND writable
	// (PAGE_READWRITE / PAGE_EXECUTE_READWRITE / PAGE_WRITECOPY). Use this before
	// writing to game memory to avoid ACCESS_VIOLATION on read-only / code pages.
	bool IsWritable(const void* address, size_t size);

	// Math --------------------------------------------------------------------
	float NormalizeHeading(float degrees);
	// Shortest signed difference (to - from) in degrees, range [-180, 180].
	float HeadingDelta(float from, float to);
	float Clamp(float value, float min, float max);
	int   ClampInt(int value, int min, int max);

	// Keys --------------------------------------------------------------------
	// Key ids: 0 .. 0xD3 are RedHook/DirectInput scan codes (see KeyCode in Enums.h),
	// 0x100 + n are mouse buttons (0 = left, 1 = right, 2 = middle, 3 = X1, 4 = X2).
	constexpr int MouseKeyBase = 0x100;
	constexpr int MouseKeyCount = 5;
	constexpr int KeyIdMax = MouseKeyBase + MouseKeyCount;

	int         ScanCodeToVirtualKey(int scanCode);
	bool        IsKeyIdDownRaw(int keyId);
	const char* KeyIdName(int keyId);
	int         KeyIdFromName(const char* name);
}
