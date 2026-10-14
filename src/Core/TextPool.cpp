#include "pch.h"

namespace
{
	// How long a string stays alive after it was last used (in frames).
	constexpr uint32_t kKeepFrames = 8;

	struct Entry
	{
		char* text;
		uint64_t lastFrame;
		size_t size;
	};

	std::vector<Entry> g_entries;
	std::map<std::string, size_t> g_lookup;   // text -> index in g_entries
	uint64_t g_frame = 0;
}

namespace TextPool
{
	void BeginFrame()
	{
		++g_frame;
		g_lookup.clear();
	}

	void EndFrame()
	{
		// Recycle everything that has not been used for kKeepFrames frames.
		if (g_frame % 30 != 0)
			return;

		size_t writeIndex = 0;

		for (size_t readIndex = 0; readIndex < g_entries.size(); ++readIndex)
		{
			Entry& entry = g_entries[readIndex];

			if (g_frame - entry.lastFrame > kKeepFrames)
			{
				free(entry.text);
				continue;
			}

			if (writeIndex != readIndex)
				g_entries[writeIndex] = entry;

			++writeIndex;
		}

		g_entries.resize(writeIndex);
	}

	void Shutdown()
	{
		for (Entry& entry : g_entries)
			free(entry.text);

		g_entries.clear();
		g_lookup.clear();
	}

	const char* Intern(const std::string& text)
	{
		// String literals and long-lived constants can be handed out directly.
		auto cached = g_lookup.find(text);

		if (cached != g_lookup.end())
		{
			g_entries[cached->second].lastFrame = g_frame;
			return g_entries[cached->second].text;
		}

		const size_t size = text.size() + 1;
		char* copy = (char*)malloc(size);

		if (copy == nullptr)
			return "";

		memcpy(copy, text.c_str(), size);

		g_entries.push_back({ copy, g_frame, size });
		g_lookup[text] = g_entries.size() - 1;

		return copy;
	}
}
