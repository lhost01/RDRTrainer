#pragma once

// Very small key/value settings store backed by "RDRTrainer.ini" next to the plugin.
// Keys are dotted, e.g. "Aimbot.Enabled" or "GodMode.Hotkey".

namespace Config
{
	void Load();
	void Save();
	void MarkDirty();

	bool        GetBool(const char* key, bool defaultValue);
	int         GetInt(const char* key, int defaultValue);
	float       GetFloat(const char* key, float defaultValue);
	std::string GetString(const char* key, const char* defaultValue);

	void SetBool(const char* key, bool value);
	void SetInt(const char* key, int value);
	void SetFloat(const char* key, float value);
	void SetString(const char* key, const char* value);

	const std::filesystem::path& FilePath();
}
