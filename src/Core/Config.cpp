#include "pch.h"

namespace
{
	std::map<std::string, std::string> g_values;
	std::filesystem::path g_file;
	uint64_t g_dirtySince = 0;

	std::string ToLower(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return (char)tolower(c); });
		return text;
	}
}

namespace Config
{
	void Load()
	{
		g_file = Utils::ModuleFile(L"RDRTrainer.ini");
		g_values.clear();

		std::string text;

		if (!Utils::ReadTextFile(g_file, text))
		{
			Log::Info("Config: no settings file yet (%s), defaults will be used.", g_file.string().c_str());
			return;
		}

		size_t position = 0;

		while (position <= text.size())
		{
			size_t end = text.find('\n', position);

			if (end == std::string::npos)
				end = text.size();

			std::string line = text.substr(position, end - position);
			position = end + 1;

			if (!line.empty() && line.back() == '\r')
				line.pop_back();

			const size_t trimmed = line.find_first_not_of(" \t");

			if (trimmed == std::string::npos || line[trimmed] == '#' || line[trimmed] == ';')
				continue;

			const size_t separator = line.find('=', trimmed);

			if (separator == std::string::npos)
				continue;

			std::string key = line.substr(trimmed, separator - trimmed);
			std::string value = line.substr(separator + 1);

			// trim
			while (!key.empty() && (key.back() == ' ' || key.back() == '\t'))
				key.pop_back();

			const size_t valueStart = value.find_first_not_of(" \t");
			value = (valueStart == std::string::npos) ? std::string() : value.substr(valueStart);

			if (!key.empty())
				g_values[key] = value;
		}

		Log::Info("Config: loaded %zu keys from %s", g_values.size(), g_file.string().c_str());
	}

	void Save()
	{
		if (g_file.empty())
			g_file = Utils::ModuleFile(L"RDRTrainer.ini");

		std::string text;
		text += "# RDR 修改器 (Red Dead Redemption trainer) - settings\n";
		text += "# 热键请填写键盘按键名，例如 F1 / INSERT / NUMPAD1 / MOUSE2\n";
		text += "# Hotkey names: F1..F12, INSERT, DELETE, HOME, END, PGUP, PGDN, NUMPAD0..NUMPAD9, MOUSE1..MOUSE5, letters...\n";
		text += "# Delete this file to restore all defaults.\n\n";

		for (const auto& entry : g_values)
			text += Utils::Format("%s=%s\n", entry.first.c_str(), entry.second.c_str());

		if (Utils::WriteTextFile(g_file, text))
		{
			g_dirtySince = 0;
			Log::Info("Config: saved %zu keys.", g_values.size());
		}
		else
		{
			Log::Error("Config: failed to write %s", g_file.string().c_str());
		}
	}

	void MarkDirty()
	{
		g_dirtySince = GetTickCount64();
	}

	bool GetBool(const char* key, bool defaultValue)
	{
		auto it = g_values.find(key);

		if (it == g_values.end())
			return defaultValue;

		const std::string value = ToLower(it->second);

		return value == "1" || value == "true" || value == "on" || value == "yes";
	}

	int GetInt(const char* key, int defaultValue)
	{
		auto it = g_values.find(key);

		if (it == g_values.end() || it->second.empty())
			return defaultValue;

		return (int)strtol(it->second.c_str(), nullptr, 0);
	}

	float GetFloat(const char* key, float defaultValue)
	{
		auto it = g_values.find(key);

		if (it == g_values.end() || it->second.empty())
			return defaultValue;

		return (float)atof(it->second.c_str());
	}

	std::string GetString(const char* key, const char* defaultValue)
	{
		auto it = g_values.find(key);

		return it == g_values.end() ? std::string(defaultValue) : it->second;
	}

	void SetBool(const char* key, bool value)
	{
		g_values[key] = value ? "1" : "0";
	}

	void SetInt(const char* key, int value)
	{
		g_values[key] = Utils::Format("%d", value);
	}

	void SetFloat(const char* key, float value)
	{
		g_values[key] = Utils::Format("%.4f", value);
	}

	void SetString(const char* key, const char* value)
	{
		g_values[key] = value;
	}

	const std::filesystem::path& FilePath()
	{
		return g_file;
	}
}
