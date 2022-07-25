#include "World.hpp"

#include "Game/Game.hpp"
#include "Game/BlockMaterialDef.hpp"
#include "Game/BlockDef.hpp"
#include "Game/BlockSetDefinition.hpp"
#include "Game/Scene.hpp"
#include "Game/Player.hpp"
#include "Game/Actor.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/SoundClip.hpp"
#include "Game/GameCommon.hpp"
#include "Game/AI.hpp"
#include "Game/BlockSetDefinition.hpp"
#include "Game/ChunkProvider.hpp"
#include "Game/WorldGenerator.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/NamedStrings.hpp"

#include "ThirdParty/squirrel/SmoothNoise.hpp"

extern bool g_useSkyBlock;

constexpr bool SHOW_COLLISION_VOLUME = false;

SpawnInfo::SpawnInfo(const ActorDefinition* definition, const Vec3& position /*= Vec3::ZERO*/, const EulerAngles& orientation /*= EulerAngles()*/, const Vec3& velocity /*= Vec3::ZERO*/)
	: m_definition(definition)
	, m_position(position)
	, m_orientation(orientation)
	, m_velocity(velocity)
{
}

SpawnInfo::SpawnInfo(const char* definitionName, const Vec3& position /*= Vec3::ZERO*/, const EulerAngles& orientation /*= EulerAngles()*/, const Vec3& velocity /*= Vec3::ZERO*/)
	: SpawnInfo(ActorDefinition::GetByName(definitionName), position, orientation, velocity)
{
}

bool SpawnInfo::LoadFromXmlElement(const XmlElement& element)
{
	m_position = ParseXmlAttribute(element, "position", m_position);
	m_orientation = ParseXmlAttribute(element, "orientation", m_orientation);
	m_velocity = ParseXmlAttribute(element, "velocity", m_velocity);

	m_definition = ActorDefinition::GetByName(ParseXmlAttribute(element, "actor", ""));

	return true;
}

extern RandomNumberGenerator rng;
constexpr int ENV_CONSTANT_BUFFER_SLOT = CUSTOM_CONSTANT_BUFFER_SLOT_START + 1;

int InitializeShaderConsts()
{
	g_theRenderer->InitializeCustomConstantBuffer(ENV_CONSTANT_BUFFER_SLOT, sizeof(EnvironmentConstants));
	return 0;
}

World::World()
{
}

World::~World()
{
}

void World::Initialize()
{
	static int dummy = InitializeShaderConsts();

	for (int pass = 0; pass < RENDER_PASS_SIZE; pass++)
	{
		if (!m_renderTarget[pass])
		{
			m_renderTarget[pass] = g_theRenderer->CreateTexture(IntVec2(-1, -1), false, false, true);
			m_depthTarget[pass] = g_theRenderer->CreateTexture(IntVec2(-1, -1), true, false, true);
		}
	}

	m_worldTexture = &BlockSetDefinition::GetDefinition()->GetBlockMaterialAtlas()->m_spriteSheet->GetTexture();

	m_worldShader     = g_theRenderer->CreateOrGetShader(g_gameConfigBlackboard.GetValue("worldShader",     "World"    ).c_str());
	m_fluidShader     = g_theRenderer->CreateOrGetShader(g_gameConfigBlackboard.GetValue("fluidShader",     "Fluid"    ).c_str());
	m_worldPostShader = g_theRenderer->CreateOrGetShader(g_gameConfigBlackboard.GetValue("worldPostShader", "WorldPost").c_str());

	if (WORLD_DEBUG_NO_TEXTURE)
		m_worldTexture = nullptr;
	if (WORLD_DEBUG_NO_SHADER)
		m_worldShader = nullptr;
	if (WORLD_DEBUG_NO_SHADER)
		m_fluidShader = nullptr;

	if (g_useSkyBlock)
	{
		m_chunkManager = new ChunkProvider(this, "Map/SkyBlock", new SkyBlockWorldGenerator());
	}
	else
	{
		m_chunkManager = new ChunkProvider(this, "Map/World", new OverworldWorldGenerator());
	}

	m_player[0] = new Player(0, true, nullptr);

	SpawnInfo info;
	info.m_definition = ActorDefinition::GetByName("Player");
	info.m_position = Vec3(0, 0, 80);
	m_player[0]->Possess(AddEntity(info));

	m_chunkManager->SetHotspotSize(1);
	m_chunkManager->SetHotspot(0, m_player[0]->GetEyePosition());
}

void World::Shutdown()
{
	delete m_player[0];
	delete m_player[1];
	m_player[0] = nullptr;
	m_player[1] = nullptr;
	RemoveEntities();

	m_chunkManager->UnloadAllChunks();
	delete m_chunkManager;
	m_chunkManager = nullptr;

	m_worldShader = nullptr;
	m_worldTexture = nullptr;

	for (int pass = 0; pass < RENDER_PASS_SIZE; pass++)
	{
		g_theRenderer->DeleteTexture(m_renderTarget[pass]);
		g_theRenderer->DeleteTexture(m_depthTarget[pass]);
	}
}

void World::Update(float deltaSeconds)
{
// 	if (g_theInput->WasKeyJustPressed(KEYCODE_N))
// 	{
// 		Actor* actor = m_player[0]->GetActor();
// 		Actor* next = actor ? GetNextEntity(actor) : GetEntity(true);
// 		while (!next->m_definition->m_canBePossessed)
// 			next = GetNextEntity(next);
// 		m_player[0]->Possess(next);
// 		m_player[0]->SetFreeFlyMode(false);
// 	}
// 

	GetClock()->SetTimeDilation((g_theInput->IsKeyDown(KEYCODE_Y) ? 50.0 : 1.0) * (1.0 / 400.0));

	m_chunkManager->Update();

	UpdateEntities(deltaSeconds);
	DoCollisionForActors();
	DoGarbageCollection();
}

void World::Render() const
{

	g_theRenderer->BindShader(nullptr);
	RenderWorld();
	g_theRenderer->BindShader(nullptr);
	RenderEntities();
	g_theRenderer->BindShader(nullptr);

	if (m_debugRayVisible)
	{
		g_theRenderer->SetTintColor(Rgba8::WHITE);
		g_theRenderer->SetModelMatrix(Mat4x4::IDENTITY);
		g_theRenderer->BindTexture(nullptr);
		DebugRenderWorld(g_theGame->GetCurrentScene()->GetWorldCamera());
	}
}

void World::RenderPost() const
{
	// combine pass
	static Texture* const texNoise = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/noiseTexture.png");

	bool isInWater = g_theGame->GetCurrentScene()->GetWorldCameraEntity()->IsInWater();
	auto envConsts = m_envConsts;
	envConsts.LIGHTNING_VALUE = isInWater ? 1.0f : 0.0f; // borrow lightning value for is water in postprocess shader
	g_theRenderer->SetCustomConstantBuffer(ENV_CONSTANT_BUFFER_SLOT, &envConsts);
	g_theRenderer->SetTintColor(isInWater ? Rgba8(80, 80, 255) : Rgba8::WHITE);

	g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

	g_theRenderer->BindTexture(m_renderTarget[0], 1);
	g_theRenderer->BindTexture(m_renderTarget[1], 2);
	g_theRenderer->BindTexture(m_depthTarget[0], 3);
	g_theRenderer->BindTexture(m_depthTarget[1], 4);
	g_theRenderer->BindTexture(texNoise, 5);
	g_theRenderer->BindShader(m_worldPostShader);

	AABB2 viewport = g_theRenderer->GetViewport();

	Vertex_PCU verts[6] = {};
	verts[0].m_position = Vec3(viewport.m_mins.x, viewport.m_maxs.y, 0);
	verts[0].m_uvTexCoords = Vec2(0, 0);
	verts[1].m_position = Vec3(viewport.m_mins.x, viewport.m_mins.y, 0);
	verts[1].m_uvTexCoords = Vec2(0, 1);
	verts[2].m_position = Vec3(viewport.m_maxs.x, viewport.m_maxs.y, 0);
	verts[2].m_uvTexCoords = Vec2(1, 0);
	verts[3].m_position = Vec3(viewport.m_maxs.x, viewport.m_mins.y, 0);
	verts[3].m_uvTexCoords = Vec2(1, 1);
	verts[4].m_position = Vec3(viewport.m_maxs.x, viewport.m_maxs.y, 0);
	verts[4].m_uvTexCoords = Vec2(1, 0);
	verts[5].m_position = Vec3(viewport.m_mins.x, viewport.m_mins.y, 0);
	verts[5].m_uvTexCoords = Vec2(0, 1);

	g_theRenderer->DrawVertexArray(6, verts);

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(nullptr, 1);
	g_theRenderer->BindTexture(nullptr, 2);
	g_theRenderer->BindTexture(nullptr, 3);
	g_theRenderer->BindTexture(nullptr, 4);
	g_theRenderer->BindTexture(nullptr, 5);
}

void World::UpdateEnvVariables() const
{
	EnvironmentConstants& envConsts = (EnvironmentConstants&)m_envConsts;

	// Camera World Position
	envConsts.CAMERA_WORLD_POSITION = g_theGame->GetCurrentScene()->GetWorldCamera().GetCameraView().m_position;

	// World Time
	envConsts.WORLD_TIME = (float)GetClock()->GetTotalTime();
	double dummyIntegerPart;
	float timeOfDay = (float)modf(GetClock()->GetTotalTime() + 0.25f, &dummyIntegerPart);

	// Environment Colors
	float skyValue = Clamp(Max((1 - cosf(2 * timeOfDay * PI)) - 1, 0.0f) * 4.0f, 0.0f, 1.0f); // 0(0.25) ~ 1(0.5) ~ 0(0.25)
	envConsts.SKY_COLOR = RgbaF::LerpColor(RgbaF(Rgba8(20, 20, 40)), RgbaF(Rgba8(200, 230, 255)), skyValue);
	envConsts.SKYLIGHT_COLOR = RgbaF::LerpColor(RgbaF(Rgba8(20, 20, 40)), RgbaF::WHITE, skyValue);
	envConsts.GLOWLIGHT_COLOR = RgbaF(1.0f, 0.9f, 0.8f);

	// Environment Values
	envConsts.LIGHTNING_VALUE = Compute1dPerlinNoise(envConsts.WORLD_TIME * 200.0f, 1.f, 9, 0.5f, 2.0f, true, 0);
	envConsts.LIGHTNING_VALUE = RangeMapClamped(envConsts.LIGHTNING_VALUE, 0.6f, 0.9f);
	envConsts.FLICKER_VALUE = Compute1dPerlinNoise(envConsts.WORLD_TIME * 200.0f, 1.f, 9, 0.5f, 2.0f, true, 0);
	envConsts.FLICKER_VALUE = RangeMapClamped(envConsts.FLICKER_VALUE, -1.0f, 1.0f, 0.8f, 1.0f);

	envConsts.SKY_COLOR = RgbaF::LerpColor(envConsts.SKY_COLOR, RgbaF::WHITE, envConsts.LIGHTNING_VALUE);

	// Fog
	bool isInWater = g_theGame->GetCurrentScene()->GetWorldCameraEntity()->IsInWater();

	envConsts.FOG_DIST_FAR = (float)GetChunkManager()->GetChunkActiveRange() * (isInWater ? 0.25f : 1.0f) - 16.0f;
	envConsts.FOG_DIST_NEAR = envConsts.FOG_DIST_FAR * (isInWater ? 0.05f : 0.5f);

	g_theRenderer->SetCustomConstantBuffer(ENV_CONSTANT_BUFFER_SLOT, &envConsts);

	DebugAddMessage(Stringf("SkyValue: %.2f, Flicker: %.2f, Lightning: %.2f", skyValue, envConsts.FLICKER_VALUE, envConsts.LIGHTNING_VALUE), 0.0f, Rgba8::WHITE, Rgba8::WHITE);
}

void World::RenderWorld() const
{
	// multiple pass
	UpdateEnvVariables();
	bool isInWater = g_theGame->GetCurrentScene()->GetWorldCameraEntity()->IsInWater();
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	{
		// solid pass
		Texture* textures[2] = { m_renderTarget[RENDER_PASS_OPAQUE], m_depthTarget[RENDER_PASS_OPAQUE] };
		g_theRenderer->SetRenderTargets(2, textures);

		g_theRenderer->ClearScreen(m_envConsts.SKY_COLOR.GetAsRgba8(), m_renderTarget[RENDER_PASS_OPAQUE]);
		g_theRenderer->ClearScreen(Rgba8::WHITE, m_depthTarget[RENDER_PASS_OPAQUE]);
		g_theRenderer->ClearDepth();

		g_theRenderer->BindTexture(m_worldTexture);
		g_theRenderer->BindShader(m_worldShader);

		for (auto& chunkEntry : m_chunkManager->GetLoadedChunks())
			chunkEntry.second->Render(RENDER_PASS_OPAQUE);
	}

	{
		// fluid pass
		Texture* textures[2] = { m_renderTarget[RENDER_PASS_FLUID], m_depthTarget[RENDER_PASS_FLUID] };
		g_theRenderer->SetRenderTargets(2, textures);

		g_theRenderer->ClearScreen(m_envConsts.SKY_COLOR.GetAsRgba8(), m_renderTarget[RENDER_PASS_FLUID]);
		g_theRenderer->ClearScreen(Rgba8::WHITE, m_depthTarget[RENDER_PASS_FLUID]);
		g_theRenderer->ClearDepth();
		g_theRenderer->SetWindingOrder(isInWater ? WindingOrder::CLOCKWISE : WindingOrder::COUNTERCLOCKWISE);

		g_theRenderer->BindTexture(m_worldTexture);
		g_theRenderer->BindShader(m_fluidShader);
		for (auto& chunkEntry : m_chunkManager->GetLoadedChunks())
			chunkEntry.second->Render(RENDER_PASS_FLUID);

		g_theRenderer->SetWindingOrder(WindingOrder::COUNTERCLOCKWISE);
	}

	// reset render state
	g_theRenderer->SetRenderTargets(1, nullptr);
	g_theRenderer->SetDepthTarget(nullptr);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);
}

bool World::IsEntityNotGarbage(const Actor* entity) const
{
	return entity != nullptr && !entity->IsGarbage();
}

void World::RenderEntities(int entityTypeMask /*= 0xFFFFFFFF*/) const
{
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetTintColor(Rgba8::WHITE);
	g_theRenderer->SetModelMatrix(Mat4x4::IDENTITY);

	for (int i = 0; i < m_entityList.size(); i++)
	{
		RenderEntity(m_entityList[i], entityTypeMask);
	}
}

void World::RenderEntity(const Actor* entity, int entityTypeMask /*= 0xFFFFFFFF*/) const
{
	if (IsEntityNotGarbage(entity) && entity->IsOfTypeMask(entityTypeMask))
	{
		entity->Render();
	}
}

void World::DoGarbageCollection()
{
	for (auto& entry : m_entityList)
	{
		if (entry == nullptr)
			continue;
		if (!entry->IsGarbage())
			continue;

		delete entry;
		entry = nullptr;
	}
}

void World::DoEntityCollision(Actor* entity1, Actor* entity2)
{
	entity1->OnCollideWithEntity(entity2);
	entity2->OnCollideWithEntity(entity1);
}

Actor* World::AddEntity(const SpawnInfo& spawnInfo)
{
	for (int idx = 0; idx < m_entityList.size(); idx++)
	{
		auto& actorRef = m_entityList[idx];
		if (actorRef == nullptr)
		{
			ActorUID uid = ActorUID(idx, GetSaltForEntity());
			actorRef = new Actor(this, &spawnInfo, uid);
			return *uid;
		}
	}

	ActorUID uid = ActorUID((int)m_entityList.size(), GetSaltForEntity());
	m_entityList.push_back(new Actor(this, &spawnInfo, uid));
	return *uid;
}

void World::RemoveEntities(bool isGarbageCollect, int entityTypeMask /*= 0xFFFFFFFF*/)
{
	for (auto& entry : m_entityList)
	{
		if (entry == nullptr)
			continue;
		if (!entry->IsOfTypeMask(entityTypeMask))
			continue;
		if (isGarbageCollect && !entry->IsGarbage())
			continue;

		delete entry;
		entry = nullptr;
	}
}

void World::RemoveEntity(const ActorUID& uid, bool isGarbageCollect, int entityTypeMask /*= 0xFFFFFFFF*/)
{
	Actor* entity = GetEntityFromUID(uid);

	if (entity == nullptr)
		return;
	if (!entity->IsOfTypeMask(entityTypeMask))
		return;
	if (isGarbageCollect && !entity->IsGarbage())
		return;

	int index = entity->m_uid.GetIndex();
	delete entity;
	m_entityList[index] = nullptr;
}

void World::UpdateEntities(float deltaSeconds, int entityTypeMask /*= 0xFFFFFFFF*/)
{
	for (int i = 0; i < m_entityList.size(); i++)
	{
		UpdateEntity(m_entityList[i], deltaSeconds, entityTypeMask);
	}
}

void World::UpdateEntity(Actor* entity, float deltaSeconds, int entityTypeMask /*= 0xFFFFFFFF*/)
{
	if (IsEntityNotGarbage(entity) && entity->IsOfTypeMask(entityTypeMask))
	{
		entity->Update(deltaSeconds);
	}
}

Actor* World::GetEntity(bool noDead, int entityTypeMask /*= 0xFFFFFFFF*/) const
{
	for (int idx = 0; idx < m_entityList.size(); idx++)
	{
		const auto& actor = m_entityList[idx];
		if (actor == nullptr)
			continue;
		if (!IsEntityNotGarbage(actor)) continue;

		if (!actor->IsOfTypeMask(entityTypeMask)) continue;

		if (noDead && actor->IsDead()) continue;

		return actor;
	}
	return nullptr;
}

Actor* World::GetNextEntity(Actor* ref) const
{
	for (int index = ref->m_uid.GetIndex() + 1;; index++)
	{
		Actor* next = m_entityList[index % (int)m_entityList.size()];
		if (next)
			return next;
	}
}

void World::GetEntities(EntityList& entityList_out, bool noDead, int entityTypeMask /*= 0xFFFFFFFF*/) const
{
	for (int idx = 0; idx < m_entityList.size(); idx++)
	{
		const auto& actor = m_entityList[idx];
		if (actor == nullptr)
			continue;

		if (!IsEntityNotGarbage(actor)) continue;

		if (!actor->IsOfTypeMask(entityTypeMask)) continue;

		if (noDead && actor->IsDead()) continue;

		entityList_out.push_back(actor);
	}
}

EntityList World::GetEntities(bool noDead, int entityTypeMask /*= 0xFFFFFFFF*/) const
{
	EntityList list;
	GetEntities(list, noDead, entityTypeMask);
	return list;
}

EntityList World::GetEntities(bool noDead, const ActorDefinition* definition) const
{
	EntityList list;
	for (const auto& actor : m_entityList)
	{
		if (actor && (!noDead || !actor->IsDead()) && definition == actor->m_definition)
			list.push_back(actor);
	}
	return list;
}

WorldRaycastResult World::FastRaycastVsTiles(const Vec3& fromPosition, const Vec3& toPosition) const
{
	WorldRaycastResult result;

	const Vec3 startPosition = fromPosition;
	Vec3 forwardNormal = (toPosition - fromPosition);
	const float maxDistance = forwardNormal.NormalizeAndGetPreviousLength();

	const int tileX = Floor(startPosition.x);
	const int tileY = Floor(startPosition.y);
	const int tileZ = Floor(startPosition.z);
	BlockIterator ite = BlockIterator(m_chunkManager, IntVec3(tileX, tileY, tileZ));
	Block* blk = ite.GetBlock();
	if (blk->IsValid() && blk->IsSolid())
	{
		result.m_result = RaycastResult3D(0.0f, startPosition, -forwardNormal);
		result.m_hitBlock = true; // cannot figure out floor / ceiling / tile for now
		result.m_blockIte = ite;
	}

	const float distPerXCrossing = 1.0f / fabsf(forwardNormal.x);
	const float distPerYCrossing = 1.0f / fabsf(forwardNormal.y);
	const float distPerZCrossing = 1.0f / fabsf(forwardNormal.z);

	const int stepDirectionX = forwardNormal.x < 0 ? -1 : 1;
	const int stepDirectionY = forwardNormal.y < 0 ? -1 : 1;
	const int stepDirectionZ = forwardNormal.z < 0 ? -1 : 1;

	const float xOfFirstXCrossing = tileX + ((float)stepDirectionX + 1.0f) / 2.0f;
	const float yOfFirstYCrossing = tileY + ((float)stepDirectionY + 1.0f) / 2.0f;
	const float zOfFirstZCrossing = tileZ + ((float)stepDirectionZ + 1.0f) / 2.0f;

	const float xDistToFirstX = xOfFirstXCrossing - startPosition.x;
	const float yDistToFirstY = yOfFirstYCrossing - startPosition.y;
	const float zDistToFirstZ = zOfFirstZCrossing - startPosition.z;

	float distOfNextXCrossing = fabsf(xDistToFirstX) * distPerXCrossing;
	float distOfNextYCrossing = fabsf(yDistToFirstY) * distPerYCrossing;
	float distOfNextZCrossing = fabsf(zDistToFirstZ) * distPerZCrossing;

	while (true)
	{
		if (distOfNextXCrossing <= distOfNextYCrossing && distOfNextXCrossing <= distOfNextZCrossing)
		{
			// go for x step
			if (distOfNextXCrossing >= maxDistance) 
				return result;

			ite += IntVec3(stepDirectionX, 0, 0);
			blk = ite.GetBlock();
			if (blk->IsValid() && blk->IsSolid())
			{
				Vec3 hitPos = startPosition + forwardNormal * distOfNextXCrossing;
				Vec3 impactNormal = Vec3((float)-stepDirectionX, 0.0f, 0.0f);
				result.m_result = RaycastResult3D(distOfNextXCrossing, hitPos, impactNormal);
				result.m_hitBlock = true; // cannot figure out floor / ceiling / tile for now
				result.m_blockIte = ite;
				return result;
			}

			distOfNextXCrossing += distPerXCrossing;
		}
		else if (distOfNextYCrossing <= distOfNextXCrossing && distOfNextYCrossing <= distOfNextZCrossing)
		{
			// go for y step
			if (distOfNextYCrossing >= maxDistance) 
				return result;

			ite += IntVec3(0, stepDirectionY, 0);
			blk = ite.GetBlock();
			if (blk->IsValid() && blk->IsSolid())
			{
				Vec3 hitPos = startPosition + forwardNormal * distOfNextYCrossing;
				Vec3 impactNormal = Vec3(0.0f, (float)-stepDirectionY, 0.0f);
				result.m_result = RaycastResult3D(distOfNextYCrossing, hitPos, impactNormal);
				result.m_hitBlock = true; // cannot figure out floor / ceiling / tile for now
				result.m_blockIte = ite;
				return result;
			}

			distOfNextYCrossing += distPerYCrossing;
		}
		else
		{
			// go for z step
			if (distOfNextZCrossing >= maxDistance) 
				return result;

			ite += IntVec3(0, 0, stepDirectionZ);
			blk = ite.GetBlock();
			if (blk->IsValid() && blk->IsSolid())
			{
				Vec3 hitPos = startPosition + forwardNormal * distOfNextZCrossing;
				Vec3 impactNormal = Vec3(0.0f, 0.0f, (float)-stepDirectionZ);
				result.m_result = RaycastResult3D(distOfNextZCrossing, hitPos, impactNormal);
				result.m_hitBlock = true;
				result.m_blockIte = ite;
				return result;
			}

			distOfNextZCrossing += distPerZCrossing;
		}
	}
}

WorldRaycastResult World::DoRaycast(const Vec3& startPos, const Vec3& fwdNormal, float maxDist, Actor* actorToIgnore) const
{
	UNUSED(actorToIgnore);
	WorldRaycastResult result = FastRaycastVsTiles(startPos, startPos + fwdNormal * maxDist);

// 	float dist = (result.m_hitFloor || result.m_hitTile) ? result.m_result.GetImpactDistance() : maxDist;
// 
// 	WorldRaycastResult actorResult = RaycastVsActors(startPos, fwdNormal, dist, actorToIgnore);
// 
// 	if (actorResult.m_result.DidImpact())
// 	{
// 		result = actorResult;
// 	}

	return result;
}

void World::AddRaycast(const Vec3& startPos, const Vec3& fwdNormal, float maxDist, Actor* actorToIgnore) const
{
	DebugAddWorldRay(startPos, fwdNormal, maxDist, DoRaycast(startPos, fwdNormal, maxDist, actorToIgnore).m_result, 0.01f, 10.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);
}

WorldRaycastResult World::RaycastVsActors(const Vec3& startPos, const Vec3& fwdNormal, float maxDist, Actor* actorToIgnore) const
{
	WorldRaycastResult result;
	for (const auto& actor : m_entityList)
	{
		if (!actor || actor == actorToIgnore)
			continue;

		if (!actor->m_physics || actor->m_physics->m_physicsHeight <= 0 || actor->m_physics->m_physicsRadius <= 0)
			continue;

		const Vec3& center = actor->m_transform.m_position;
		const float& radius = actor->m_physics->m_physicsRadius;
		const float& height = actor->m_physics->m_physicsHeight;

		RaycastResult3D actorResult = RaycastVsZCylinder(startPos, fwdNormal, maxDist, center, radius, height);

		if (actorResult.DidImpact())
		{
			result.m_actors.emplace_back(actor);
			if (!result.m_result.DidImpact() || result.m_result.GetImpactDistance() > actorResult.GetImpactDistance())
			{
				result.m_result = actorResult;
				result.m_hitActor = true;
				result.m_actor = actor->GetUID();
			}
		}
	}

	return result;
}

Actor* World::FindNearestActor(Actor* actor, const Vec3& forward, Faction faction, bool ignoreProj) const
{
	if (actor->m_definition->m_faction == Faction::NEUTRAL)
		return nullptr;

	Actor* nearest = nullptr;
	EntityList actors = GetEntities(true, 1 << faction);
	float sightRadiusSq = actor->m_definition->m_sightRadius * actor->m_definition->m_sightRadius;
	for (auto& found : actors)
	{
		if (found->IsProjectile() && ignoreProj)
			continue; // ignore projectile

		Vec3 offset = found->m_transform.m_position - actor->m_transform.m_position;
		if (offset.GetLengthSquared() < sightRadiusSq && offset.GetNormalized().Dot2D(forward) > CosDegrees(actor->m_definition->m_sightAngle))
		{
			WorldRaycastResult raycast = FastRaycastVsTiles(actor->GetEyePosition(), found->GetEyePosition());
			// float length = offset.GetLength();
			// DebugAddWorldRay(actor->GetEyePosition(), offset / length, length, raycast.m_result, 0.01f, 10.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);

			if (raycast.m_hitBlock)
				continue;

			if (!nearest || (found->m_transform.m_position - actor->m_transform.m_position).GetLengthSquared() > offset.GetLengthSquared())
				nearest = found;
		}
	}

	return nearest;
}

void World::AttractActor(Actor* actor, float range, Faction faction, bool ignoreProj /*= true*/) const
{
	float radiusSq = range * range;

	EntityList actors = GetEntities(true, 1 << faction);
	for (auto& found : actors)
	{
		if (found->IsProjectile() && ignoreProj)
			continue; // ignore projectile

		Vec3 offset = found->m_transform.m_position - actor->m_transform.m_position;
		if (offset.GetLengthSquared() < radiusSq)
		{
			WorldRaycastResult raycast = FastRaycastVsTiles(actor->GetEyePosition(), found->GetEyePosition());

			if (raycast.m_hitBlock)
				continue;
		}
	}
}

bool World::IsInWater(const Vec3& position) const
{
	return m_chunkManager->GetBlockId(IntVec3(position)) == Blocks::BLOCK_WATER;
}

bool World::IsEntityUIDValid(const ActorUID& actorUID) const
{
	if (actorUID == ActorUID::INVALID)
		return false;
	if (actorUID.GetIndex() >= m_entityList.size())
		return false;
	const auto& actor = m_entityList[actorUID.GetIndex()];
	return actor != nullptr;
}

int World::GetSaltForEntity()
{
	int salt = 0xFFFF & m_entityUIDSalt++;
	return salt ? salt : GetSaltForEntity();
}

Chunk* World::FindChunk(const ChunkCoords& chunkCoords) const
{
	return m_chunkManager->FindLoadedChunk(chunkCoords);
}

ChunkProvider* World::GetChunkManager() const
{
	return m_chunkManager;
}

void DebugDrawBlock(const BlockIterator& ite)
{
	if (!SHOW_COLLISION_VOLUME)
		return;

	if (!ite.IsValid())
		return;
	Vec3 position = Vec3((float)ite.GetWorldCoords().x, (float)ite.GetWorldCoords().y, (float)ite.GetWorldCoords().z);
	AABB3 box(position, position + Vec3(1.0f, 1.0f, 1.0f));
	Rgba8 color = ite.GetBlock()->IsSolid() ? Rgba8::RED : Rgba8::WHITE;
	color.a = 80;
	DebugAddWorldBox(box, 0.0f, color, color, DebugRenderMode::XRAY);
}

void World::DoCollisionForActors()
{
	for (int i = 0; i < m_entityList.size(); i++)
	{
		auto* actor = m_entityList[i];
		if (!actor || !actor->m_physics || actor->m_physics->IsNoclip())
			continue;

// 		// collide with actor
// 		if (actor->m_definition->m_collidesWithActors)
// 		{
// 			for (int j = i + 1; j < m_entityList.size(); j++)
// 			{
// 				auto* actorInner = m_entityList[j];
// 				if (!actorInner)
// 					continue;
// 
// 				if (actorInner->m_definition->m_collidesWithActors)
// 				{
// 					bool touched = PushDiscsOutOfEachOther2D(actor->m_transform.GetPosition2D(), actor->m_collision->m_physicsRadius, actorInner->m_transform.GetPosition2D(), actorInner->m_collision->m_physicsRadius);
// 					if (touched)
// 					{
// 						actor->OnCollideWithEntity(actorInner);
// 						actorInner->OnCollideWithEntity(actor);
// 					}
// 				}
// 			}
// 		}

		// collide with world
		if (actor->m_definition->m_collidesWithWorld)
		{
			Vec3 positionOriginal = actor->m_transform.m_position;
			Vec3& position = actor->m_transform.m_position;
			float sizeHalfXY = actor->m_physics->m_physicsRadius;
			float sizeZ = actor->m_physics->m_physicsHeight;
			float sizeHalfZ = sizeZ * 0.5f;
			Vec3 center = position;
			center.z += sizeHalfZ;

			BlockIterator ite = BlockIterator(GetChunkManager(), IntVec3(center));
			BlockIterator neighbor;

			if (!ite.IsValid()) // actor chunk not loaded
			{
				continue;
			}

			if (ite.GetBlock()->IsSolid())
			{
				// player is in block, there's nothing we can do with collision
				// continue;
			}

			float centerBlkMinX = (float)ite.GetWorldCoords().x;
			float centerBlkMaxX = centerBlkMinX + 1.0f;
			float centerBlkMinY = (float)ite.GetWorldCoords().y;
			float centerBlkMaxY = centerBlkMinY + 1.0f;
			float centerBlkMinZ = (float)ite.GetWorldCoords().z;
			float centerBlkMaxZ = centerBlkMinZ + 1.0f;


#define entityMinX (position.x - sizeHalfXY)
#define entityMaxX (position.x + sizeHalfXY)
#define entityMinY (position.y - sizeHalfXY)
#define entityMaxY (position.y + sizeHalfXY)
#define entityMinZ (position.z)
#define entityMaxZ (position.z + sizeZ)

			// check UP DOWN
			if (entityMaxZ > centerBlkMaxZ)
			{
				neighbor = ite.GetBlockNeighbor(BLOCK_FACE_UP);
				DebugDrawBlock(neighbor);
				if (neighbor.GetBlock()->IsSolid())
					position.z -= entityMaxZ - centerBlkMaxZ;
			}
			if (entityMinZ < centerBlkMinZ)
			{
				neighbor = ite.GetBlockNeighbor(BLOCK_FACE_DOWN);
				DebugDrawBlock(neighbor);
				if (neighbor.GetBlock()->IsSolid())
					position.z -= entityMinZ - centerBlkMinZ;
			}

			// check NSWE
			if (entityMaxX > centerBlkMaxX) // north
			{
				neighbor = ite.GetBlockNeighbor(BLOCK_FACE_NORTH);
				DebugDrawBlock(neighbor);
				if (neighbor.GetBlock()->IsSolid())
					position.x -= entityMaxX - centerBlkMaxX;
			}
			if (entityMinX < centerBlkMinX) // south
			{
				neighbor = ite.GetBlockNeighbor(BLOCK_FACE_SOUTH);
				DebugDrawBlock(neighbor);
				if (neighbor.GetBlock()->IsSolid())
					position.x -= entityMinX - centerBlkMinX;
			}
			if (entityMaxY > centerBlkMaxY) // west
			{
				neighbor = ite.GetBlockNeighbor(BLOCK_FACE_WEST);
				DebugDrawBlock(neighbor);
				if (neighbor.GetBlock()->IsSolid())
					position.y -= entityMaxY - centerBlkMaxY;
			}
			if (entityMinY < centerBlkMinY) // east
			{
				neighbor = ite.GetBlockNeighbor(BLOCK_FACE_EAST);
				DebugDrawBlock(neighbor);
				if (neighbor.GetBlock()->IsSolid())
					position.y -= entityMinY - centerBlkMinY;
			}

			// check UP NSWE
			if (entityMaxZ > centerBlkMaxZ)
			{
				BlockIterator original = ite;
				ite = ite.GetBlockNeighbor(BLOCK_FACE_UP);
				if (entityMaxX > centerBlkMaxX) // north
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_NORTH);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
						position.x -= entityMaxX - centerBlkMaxX;
				}
				if (entityMinX < centerBlkMinX) // south
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_SOUTH);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
						position.x -= entityMinX - centerBlkMinX;
				}
				if (entityMaxY > centerBlkMaxY) // west
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_WEST);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
						position.y -= entityMaxY - centerBlkMaxY;
				}
				if (entityMinY < centerBlkMinY) // east
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_EAST);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
						position.y -= entityMinY - centerBlkMinY;
				}
				ite = original;
			}

			// check DOWN NSWE
			if (entityMinZ < centerBlkMinZ)
			{
				BlockIterator original = ite;
				ite = ite.GetBlockNeighbor(BLOCK_FACE_DOWN);
				if (entityMaxX > centerBlkMaxX) // north
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_NORTH);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
						position.x -= entityMaxX - centerBlkMaxX;
				}
				if (entityMinX < centerBlkMinX) // south
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_SOUTH);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
						position.x -= entityMinX - centerBlkMinX;
				}
				if (entityMaxY > centerBlkMaxY) // west
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_WEST);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
						position.y -= entityMaxY - centerBlkMaxY;
				}
				if (entityMinY < centerBlkMinY) // east
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_EAST);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
						position.y -= entityMinY - centerBlkMinY;
				}
				ite = original;
			}

			// check corner
			if (entityMaxX > centerBlkMaxX && entityMaxY > centerBlkMaxY) // north west ++
			{
				neighbor = ite.GetBlockNeighbor(BLOCK_FACE_NORTH).GetBlockNeighbor(BLOCK_FACE_WEST);
				DebugDrawBlock(neighbor);
				if (neighbor.GetBlock()->IsSolid())
				{
					position.x -= entityMaxX - centerBlkMaxX;
					position.y -= entityMaxY - centerBlkMaxY;
				}
			}
			if (entityMinX < centerBlkMinX && entityMinY < centerBlkMinY) // south east --
			{
				neighbor = ite.GetBlockNeighbor(BLOCK_FACE_SOUTH).GetBlockNeighbor(BLOCK_FACE_EAST);
				DebugDrawBlock(neighbor);
				if (neighbor.GetBlock()->IsSolid())
				{
					position.x -= entityMinX - centerBlkMinX;
					position.y -= entityMinY - centerBlkMinY;
				}
			}
			if (entityMaxY > centerBlkMaxY && entityMinX < centerBlkMinX) // west south +-
			{
				neighbor = ite.GetBlockNeighbor(BLOCK_FACE_WEST).GetBlockNeighbor(BLOCK_FACE_SOUTH);
				DebugDrawBlock(neighbor);
				if (neighbor.GetBlock()->IsSolid())
				{
					position.y -= entityMaxY - centerBlkMaxY;
					position.x -= entityMinX - centerBlkMinX;
				}
			}
			if (entityMinY < centerBlkMinY && entityMaxX > centerBlkMaxX) // east north -+
			{
				neighbor = ite.GetBlockNeighbor(BLOCK_FACE_EAST).GetBlockNeighbor(BLOCK_FACE_NORTH);
				DebugDrawBlock(neighbor);
				if (neighbor.GetBlock()->IsSolid())
				{
					position.x -= entityMaxX - centerBlkMaxX;
					position.y -= entityMinY - centerBlkMinY;
				}
			}

			// check UP corner
			if (entityMaxZ > centerBlkMaxZ)
			{
				BlockIterator original = ite;
				ite = ite.GetBlockNeighbor(BLOCK_FACE_UP);
				if (entityMaxX > centerBlkMaxX && entityMaxY > centerBlkMaxY) // north west ++
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_NORTH).GetBlockNeighbor(BLOCK_FACE_WEST);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
					{
						position.x -= entityMaxX - centerBlkMaxX;
						position.y -= entityMaxY - centerBlkMaxY;
					}
				}
				if (entityMinX < centerBlkMinX && entityMinY < centerBlkMinY) // south east --
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_SOUTH).GetBlockNeighbor(BLOCK_FACE_EAST);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
					{
						position.x -= entityMinX - centerBlkMinX;
						position.y -= entityMinY - centerBlkMinY;
					}
				}
				if (entityMaxY > centerBlkMaxY && entityMinX < centerBlkMinX) // west south +-
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_WEST).GetBlockNeighbor(BLOCK_FACE_SOUTH);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
					{
						position.y -= entityMaxY - centerBlkMaxY;
						position.x -= entityMinX - centerBlkMinX;
					}
				}
				if (entityMinY < centerBlkMinY && entityMaxX > centerBlkMaxX) // east north -+
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_EAST).GetBlockNeighbor(BLOCK_FACE_NORTH);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
					{
						position.x -= entityMaxX - centerBlkMaxX;
						position.y -= entityMinY - centerBlkMinY;
					}
				}
				ite = original;
			}

			// check DOWN corner
			if (entityMinZ < centerBlkMinZ)
			{
				BlockIterator original = ite;
				ite = ite.GetBlockNeighbor(BLOCK_FACE_DOWN);
				if (entityMaxX > centerBlkMaxX && entityMaxY > centerBlkMaxY) // north west ++
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_NORTH).GetBlockNeighbor(BLOCK_FACE_WEST);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
					{
						position.x -= entityMaxX - centerBlkMaxX;
						position.y -= entityMaxY - centerBlkMaxY;
					}
				}
				if (entityMinX < centerBlkMinX && entityMinY < centerBlkMinY) // south east --
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_SOUTH).GetBlockNeighbor(BLOCK_FACE_EAST);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
					{
						position.x -= entityMinX - centerBlkMinX;
						position.y -= entityMinY - centerBlkMinY;
					}
				}
				if (entityMaxY > centerBlkMaxY && entityMinX < centerBlkMinX) // west south +-
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_WEST).GetBlockNeighbor(BLOCK_FACE_SOUTH);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
					{
						position.y -= entityMaxY - centerBlkMaxY;
						position.x -= entityMinX - centerBlkMinX;
					}
				}
				if (entityMinY < centerBlkMinY && entityMaxX > centerBlkMaxX) // east north -+
				{
					neighbor = ite.GetBlockNeighbor(BLOCK_FACE_EAST).GetBlockNeighbor(BLOCK_FACE_NORTH);
					DebugDrawBlock(neighbor);
					if (neighbor.GetBlock()->IsSolid())
					{
						position.x -= entityMaxX - centerBlkMaxX;
						position.y -= entityMinY - centerBlkMinY;
					}
				}
				ite = original;
			}


#undef entityMinX
#undef entityMaxX
#undef entityMinY
#undef entityMaxY
#undef entityMinZ
#undef entityMaxZ


			if (position != positionOriginal)
			{
				Vec3& velocity = actor->m_physics->m_velocity;

				if (positionOriginal.x != position.x)
					velocity.x = 0;
				if (positionOriginal.y != position.y)
					velocity.y = 0;
				if (positionOriginal.z != position.z)
					velocity.z = 0;

				if (actor->m_definition->m_dieOnCollide)
				{
					actor->Die();
					continue;
				}
			}
		}
	}
}

void World::PushOutOfBlockHorizontal(Vec3& position, float halfSizeXY, const WorldCoords& blockPos)
{
	float maxX = position.x + halfSizeXY;
	float minX = position.x - halfSizeXY;
	float maxY = position.y + halfSizeXY;
	float minY = position.y - halfSizeXY;

	float blockMaxX = (float)blockPos.x + 1.0f;
	float blockMinX = (float)blockPos.x;
	float blockMaxY = (float)blockPos.y + 1.0f;
	float blockMinY = (float)blockPos.y;

	if (maxX > blockMinX && minX <= blockMinX)
		position.x -= maxX - blockMinX;
	if (minX < blockMaxX && maxX >= blockMaxX)
		position.x -= minX - blockMaxX;
	if (maxY > blockMinY && minY <= blockMinY)
		position.y -= maxY - blockMinY;
	if (minY < blockMaxY && maxY >= blockMaxY)
		position.y -= minY - blockMaxY;
}

bool World::IsSolidAtPosition(const Vec3& position)
{
	ChunkCoords chunkCoords = Chunk::GetChunkCoords(position);
	Chunk* chunk = FindChunk(chunkCoords);
	if (chunk)
	{
		return chunk->GetBlock(Chunk::GetLocalCoords(position)).IsSolid();
	}
	return false;
}

Actor* World::GetEntityFromUID(const ActorUID& actorUID)
{
	if (actorUID == ActorUID::INVALID)
		return nullptr;
	if (actorUID.GetIndex() >= m_entityList.size())
		return nullptr;
	const auto& actor = m_entityList[actorUID.GetIndex()];
	if (!actor || actor->m_uid != actorUID)
		return nullptr;
	return actor;
}

Chunk* World::GetChunkAtPosition(const Vec3& position) const
{
	return FindChunk(Chunk::GetChunkCoords(position));
}

Block World::GetBlockAtPosition(const Vec3& position) const
{
	Chunk* chunk = GetChunkAtPosition(position);
	if (!chunk)
		return Block::INVALID;

	return chunk->GetBlock(Chunk::GetLocalCoords(position));
}

