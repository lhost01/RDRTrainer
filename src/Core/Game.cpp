#include "pch.h"

namespace
{
	constexpr int kMaxWorldActors = 70;   // RedHook: the world never holds more than 70 actors at once

	std::vector<ActorEntry> g_actors;
	std::map<Actor, std::string> g_nameCache;

	uint64_t g_frame = 0;
	bool     g_worldLoaded = false;
	bool     g_localValid = false;
	int      g_localSlot = 0;
	Actor    g_localPlayer = 0;
	Vector3  g_localPosition{};
	float    g_localHeading = 0.0f;
	Actor    g_aimbotTarget = 0;

	float g_headOffset = 1.78f;
	int   g_hostilityMode = 2;

	const std::string& ActorName(Actor actor)
	{
		auto it = g_nameCache.find(actor);

		if (it != g_nameCache.end())
			return it->second;

		const char* raw = OBJECT::GET_ACTOR_ENUM_STRING(actor);
		std::string name = (raw != nullptr) ? std::string(raw) : std::string();

		if (g_nameCache.size() > 512)
			g_nameCache.clear();

		return g_nameCache.emplace(actor, std::move(name)).first->second;
	}
}

namespace Game
{
	void SetHeadOffset(float offset) { g_headOffset = Utils::Clamp(offset, 0.5f, 4.0f); }
	float HeadOffset() { return g_headOffset; }
	void SetHostilityMode(int mode) { g_hostilityMode = Utils::ClampInt(mode, 0, 2); }
	int  HostilityMode() { return g_hostilityMode; }

	void Update()
	{
		++g_frame;
		Screen::Update();

		g_worldLoaded = STREAM::STREAMING_IS_WORLD_LOADED();
		g_localSlot = ACTOR::GET_LOCAL_SLOT();
		g_localPlayer = ACTOR::GET_PLAYER_ACTOR(g_localSlot);
		g_localValid = ENTITY::IS_ACTOR_VALID(g_localPlayer) && !HEALTH::IS_ACTOR_DEAD(g_localPlayer);

		g_actors.clear();

		if (!g_localValid)
			return;

		g_localPosition = ACTOR::GET_POSITION(g_localPlayer);
		g_localHeading = ACTOR::GET_HEADING(g_localPlayer);

		if (!g_worldLoaded)
			return;

		Actor rawActors[kMaxWorldActors] = {};
		const int count = Utils::ClampInt(REDHOOK::WORLD_GET_ALL_ACTORS(rawActors), 0, kMaxWorldActors);

		for (int index = 0; index < count; ++index)
		{
			const Actor actor = rawActors[index];

			if (actor == 0 || !ENTITY::IS_ACTOR_VALID(actor))
				continue;

			ActorEntry entry = {};
			entry.actor = actor;
			entry.dead = HEALTH::IS_ACTOR_DEAD(actor);
			entry.position = ACTOR::GET_POSITION(actor);
			entry.head = entry.position;
			entry.head.y += g_headOffset;
			entry.distance = Vector3::Distance(g_localPosition, entry.position);
			entry.health = HEALTH::GET_ACTOR_HEALTH(actor);
			entry.maxHealth = HEALTH::GET_ACTOR_MAX_HEALTH(actor);
			entry.isLocalPlayer = (actor == g_localPlayer);

			if (entry.isLocalPlayer)
			{
				entry.category = ActorCategory::LocalPlayer;
			}
			else if (ACTOR::IS_ACTOR_PLAYER(actor))
			{
				entry.category = ActorCategory::Player;
			}
			else if (ENTITY::IS_ACTOR_ANIMAL(actor))
			{
				entry.category = ActorCategory::Animal;
			}
			else if (ENTITY::IS_ACTOR_HUMAN(actor))
			{
				entry.category = IsHostile(actor) ? ActorCategory::Hostile : ActorCategory::Friendly;
			}
			else
			{
				entry.category = ActorCategory::Vehicle;
			}

			entry.valid = true;

			// Project feet and head separately: an enemy behind cover with only the
			// head exposed has an invisible feet point, which used to drop the whole
			// actor and made the aimbot ignore enemies poking their head over cover.
			const bool feetOnScreen = REDHOOK::WORLD_TO_SCREEN(&entry.position, &entry.screenX, &entry.screenY) != 0;
			entry.headValid = REDHOOK::WORLD_TO_SCREEN(&entry.head, &entry.headScreenX, &entry.headScreenY) != 0;
			entry.onScreen = feetOnScreen || entry.headValid;

			if (!entry.headValid && feetOnScreen)
			{
				// No visible head point: fall back to the feet position so the head
				// coordinates are never (0,0) (which would aim at the top-left corner).
				entry.headScreenX = entry.screenX;
				entry.headScreenY = entry.screenY;
			}

			entry.name = ActorName(actor);
			g_actors.push_back(std::move(entry));
		}
	}

	bool WorldLoaded() { return g_worldLoaded; }
	bool IsLocalPlayerValid() { return g_localValid; }
	Actor LocalPlayer() { return g_localPlayer; }
	int   LocalSlot() { return g_localSlot; }
	Vector3 LocalPlayerPosition() { return g_localPosition; }
	float LocalPlayerHeading() { return g_localHeading; }
	uint64_t Frame() { return g_frame; }

	const std::vector<ActorEntry>& Actors() { return g_actors; }

	const ActorEntry* FindActor(Actor actor)
	{
		for (const ActorEntry& entry : g_actors)
		{
			if (entry.actor == actor)
				return &entry;
		}

		return nullptr;
	}

	Actor AimbotTarget() { return g_aimbotTarget; }
	void  SetAimbotTarget(Actor actor) { g_aimbotTarget = actor; }

	bool IsHostile(Actor actor)
	{
		if (g_hostilityMode == 2)
			return true;

		if (g_hostilityMode == 1)
			return AI_MISC::AI_IS_HOSTILE_OR_ENEMY(actor, g_localPlayer) != 0;

		return AI_MISC::AI_IS_HOSTILE_OR_ENEMY(g_localPlayer, actor) != 0;
	}

	bool WorldToScreen(const Vector3& position, float& outX, float& outY)
	{
		Vector3 copy = position;

		return REDHOOK::WORLD_TO_SCREEN(&copy, &outX, &outY) != 0;
	}

	bool HasLineOfSight(const Vector3& from, const Vector3& to, Actor owner)
	{
		Vector3 hit = {};
		const Vector3 target = to;

		CAM::CAMERA_PROBE(&hit, from, target, owner, 0);

		// If the probe lands (almost) on the requested point, nothing was in the way.
		return Vector3::Distance(hit, target) < 0.75f;
	}
}
