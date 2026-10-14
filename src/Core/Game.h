#pragma once

enum class ActorCategory
{
	Unknown,
	LocalPlayer,
	Player,
	Friendly,
	Hostile,
	Animal,
	Vehicle
};

struct ActorEntry
{
	Actor          actor = 0;
	Vector3        position{};
	Vector3        head{};
	float          health = 0.0f;
	float          maxHealth = 0.0f;
	float          distance = 0.0f;
	float          screenX = 0.0f;
	float          screenY = 0.0f;
	float          headScreenX = 0.0f;
	float          headScreenY = 0.0f;
	ActorCategory  category = ActorCategory::Unknown;
	bool           valid = false;
	bool           dead = false;
	bool           isLocalPlayer = false;
	bool           onScreen = false;
	bool           headValid = false;   // head screen point is a real projection (head visible)
	std::string    name;

	float HealthRatio() const
	{
		return (maxHealth > 1.0f) ? Utils::Clamp(health / maxHealth, 0.0f, 1.0f) : 0.0f;
	}
};

namespace Game
{
	// Call once per frame (after Input::Update()).
	void Update();

	// Settings that influence world queries.
	void  SetHeadOffset(float offset);
	float HeadOffset();
	void  SetHostilityMode(int mode);   // 0 = native, 1 = native with swapped args, 2 = treat all as hostile
	int   HostilityMode();

	bool  WorldLoaded();
	bool  IsLocalPlayerValid();
	Actor LocalPlayer();
	int   LocalSlot();
	Vector3 LocalPlayerPosition();
	float LocalPlayerHeading();
	uint64_t Frame();

	const std::vector<ActorEntry>& Actors();
	const ActorEntry* FindActor(Actor actor);

	// The aimbot writes its current target here, the ESP reads it to highlight it.
	Actor AimbotTarget();
	void  SetAimbotTarget(Actor actor);

	bool IsHostile(Actor actor);
	bool WorldToScreen(const Vector3& position, float& outX, float& outY);
	// Cheap line of sight test using the game's own camera probe native.
	bool HasLineOfSight(const Vector3& from, const Vector3& to, Actor owner);
}
