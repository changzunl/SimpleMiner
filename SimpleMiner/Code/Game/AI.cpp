#include "AI.hpp"

#include "GameCommon.hpp"
#include "Game.hpp"
#include "World.hpp"
#include "Actor.hpp"
#include "Player.hpp"
#include "ActorDefinition.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/DebugRender.hpp"

extern RandomNumberGenerator rng;

AI::AI()
{
}

AI::~AI()
{
}

void AI::Update(float deltaSeconds)
{
	if (m_skipFrame)
	{
		m_skipFrame = false;
		return;
	}

	UNUSED(deltaSeconds);

	Actor* actor = GetActor();
	if (!actor || actor->IsDead())
		return;

	if (m_meleeStopwatch.IsStopped())
	{
		m_meleeStopwatch = Stopwatch(actor->m_lifetimeStopwatch.m_clock);
		m_meleeStopwatch.Start(actor->m_definition->m_meleeDelay);
	}

	constexpr float AI_UPDATE_RATE = 0.0009f;
	if (m_updateStopwatch.IsStopped())
	{
		m_updateStopwatch = Stopwatch(actor->m_lifetimeStopwatch.m_clock);
		m_updateStopwatch.Start(AI_UPDATE_RATE);
	}
	if (!m_updateStopwatch.CheckDurationElapsedAndDecrement())
	{
		return;
	}

	// TODO tick ai behavior
}

