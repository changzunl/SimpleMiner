#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Block.hpp"
#include "Game/Chunk.hpp"
#include "Game/BlockIterator.hpp"
#include "Game/ActorUID.hpp"
#include "Game/Faction.hpp"
#include "Game/Components.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/RgbaF.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/VertexFormat.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include <vector>
#include <map>
#include <deque>

//------------------------------------------------------------------------------------------------
class Game;
class World;
class Actor;
class Player;
class SoundClip;
class SoundInst;
class ActorDefinition;
class WorldDef;
class Shader;
class Texture;
struct AABB2;
struct Vertex_PCU;
class IndexBuffer;
class VertexBuffer;
class Clock;
class SpawnInfo;
struct PlayerJoin;
class Chunk;

namespace tinyxml2
{
	class XMLElement;
}
typedef tinyxml2::XMLElement XmlElement;
typedef std::vector<Actor*> EntityList;

class SpawnInfo
{
public:
	SpawnInfo() {}
	SpawnInfo(const ActorDefinition* definition, const Vec3& position = Vec3::ZERO, const EulerAngles& orientation = EulerAngles(), const Vec3& velocity = Vec3::ZERO);
	SpawnInfo(const char* definitionName, const Vec3& position = Vec3::ZERO, const EulerAngles& orientation = EulerAngles(), const Vec3& velocity = Vec3::ZERO);

	bool LoadFromXmlElement(const XmlElement& element);

public:
	const ActorDefinition* m_definition = nullptr;
	Vec3                   m_position;
	EulerAngles            m_orientation;
	Vec3                   m_velocity;
	bool                   m_isProjectile = false;
};

struct WorldRaycastResult
{
	RaycastResult3D m_result;
	bool            m_hitActor = false;
	ActorUID        m_actor;
	bool            m_hitBlock = false;
	BlockFace       m_hitBlockFace = BLOCK_FACE_NORTH;
	BlockIterator   m_blockIte;
	EntityList      m_actors;
};

struct EnvironmentConstants
{
	RgbaF SKY_COLOR;
	RgbaF SKYLIGHT_COLOR;
	RgbaF GLOWLIGHT_COLOR;
	Vec3 CAMERA_WORLD_POSITION;
	float WORLD_TIME = 0.0f;
	float LIGHTNING_VALUE = 0.0f;
	float FLICKER_VALUE = 0.0f;
	float FOG_DIST_FAR = 0.0f;
	float FOG_DIST_NEAR = 0.0f;
};

enum RenderPass
{
	RENDER_PASS_OPAQUE,
	RENDER_PASS_FLUID,
	RENDER_PASS_SIZE,
};


//------------------------------------------------------------------------------------------------
class World
{
public:
	World();
	~World();

	// lifecycle handlers
	void                         Initialize();
	void                         Shutdown();
	void                         Update(float deltaSeconds);
	void                         Render() const;
	void                         RenderPost() const;

	// map geometry
	void                         RenderWorld() const;

	// entity services
	bool                         IsEntityNotGarbage(const Actor* entity) const;
	void                         RenderEntities(int entityTypeMask = 0xFFFFFFFF) const;
	void                         RenderEntity(const Actor* entity, int entityTypeMask = 0xFFFFFFFF) const;
	void                         DoGarbageCollection();
	void                         DoEntityCollision(Actor* entity1, Actor* entity2);
	Actor*                       AddEntity(const SpawnInfo& spawnInfo);
	void                         RemoveEntities(bool isGarbageCollect = false, int entityTypeMask = 0xFFFFFFFF);
	void                         RemoveEntity(const ActorUID& uid, bool isGarbageCollect, int entityTypeMask = 0xFFFFFFFF);
	void                         UpdateEntities(float deltaSeconds, int entityTypeMask = 0xFFFFFFFF);
	void                         UpdateEntity(Actor* entity, float deltaSeconds, int entityTypeMask = 0xFFFFFFFF);
	void                         GetEntities(EntityList& entityList_out, bool noDead, int entityTypeMask = 0xFFFFFFFF) const;
	Actor*                       GetEntity(bool noDead, int entityTypeMask = 0xFFFFFFFF) const;
	Actor*                       GetNextEntity(Actor* ref) const;
	EntityList                   GetEntities(bool noDead, int entityTypeMask = 0xFFFFFFFF) const;
	EntityList                   GetEntities(bool noDead, const ActorDefinition* definition) const;

	// utilities
	WorldRaycastResult            FastRaycastVsTiles(const Vec3& fromPosition, const Vec3& toPosition) const;
	WorldRaycastResult            DoRaycast(const Vec3& startPos, const Vec3& fwdNormal, float maxDist, Actor* actorToIgnore = nullptr) const;
	void                          AddRaycast(const Vec3& startPos, const Vec3& fwdNormal, float maxDist, Actor* actorToIgnore = nullptr) const;
	WorldRaycastResult            RaycastVsActors(const Vec3& startPos, const Vec3& fwdNormal, float maxDist, Actor* actorToIgnore = nullptr) const;
	Actor*                        FindNearestActor(Actor* actor, const Vec3& forward, Faction faction, bool ignoreProj = true) const;
	void                          AttractActor(Actor* actor, float range, Faction faction, bool ignoreProj = true) const;
	bool                          IsInWater(const Vec3& position) const;

	bool                          IsEntityUIDValid(const ActorUID& actorUID) const;
	Actor*                        GetEntityFromUID(const ActorUID& actorUID);
	const Clock*                  GetClock() const { return &m_clock; }
	Clock*                        GetClock() { return &m_clock; }
	Chunk*                        GetChunkAtPosition(const Vec3& position) const;
	Block                         GetBlockAtPosition(const Vec3& position) const;
	Chunk*                        FindChunk(const ChunkCoords& chunkCoords) const;
					              
	ChunkProvider*                GetChunkManager() const;


public:
	// Entity
	Player* m_player[2] = {};
	bool    m_debugRayVisible = true;
	EnvironmentConstants m_envConsts;

protected:
	Clock m_clock;

	// Map
	ChunkProvider* m_chunkManager = nullptr;

	// Entity
	int        m_entityUIDSalt = 12;
	EntityList m_entityList = EntityList();

	// Rendering
	Shader* m_worldShader = nullptr;
	Shader* m_fluidShader = nullptr;
	Shader* m_worldPostShader = nullptr;
	Texture* m_worldTexture = nullptr;
	Texture* m_renderTarget[RENDER_PASS_SIZE] = {};
	Texture* m_depthTarget[RENDER_PASS_SIZE] = {};

private:
	void UpdateEnvVariables() const;
	int  GetSaltForEntity();
	void DoCollisionForActors();
	void PushOutOfBlockHorizontal(Vec3& position, float halfSizeXY, const WorldCoords& blockPos);
	bool IsSolidAtPosition(const Vec3& position);
};
