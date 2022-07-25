#include "GameCommon.hpp"

#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/Vec3.hpp"

RandomNumberGenerator rng;

bool WORLD_DEBUG_NO_TEXTURE = false;
bool WORLD_DEBUG_NO_SHADER = false;
bool WORLD_DEBUG_STEP_LIGHTING = false;

Vec3 CAMERA_WORLD_POSITION;

const char* GetNameFromType(BillboardType type)
{
	static const char* const names[3] = { "none", "facing", "aligned" };
	return names[(unsigned int)type];
}

BillboardType GetTypeByName(const char* name, BillboardType defaultType)
{
	static const BillboardType types[3] = { BillboardType::NONE, BillboardType::FACING, BillboardType::ALIGNED };
	for (BillboardType type : types)
	{
		if (_stricmp(GetNameFromType(type), name) == 0)
			return type;
	}
	return defaultType;
}

bool operator<(const IntVec2& a, const IntVec2& b)
{
	return (a.y == b.y) ? (a.x < b.x) : (a.y < b.y);
}