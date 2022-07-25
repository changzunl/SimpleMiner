#include "Scene.hpp"

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "App.hpp"
#include "Game.hpp"
#include "GameCommon.hpp"

Scene::Scene(Game* game)
	: m_game(game)
{
}

Scene::~Scene()
{
}

void Scene::Update()
{
}

void Scene::UpdateCamera()
{
	if (m_worldCameraShakeDisp != 0.0f)
	{
		float offset = m_game->m_rng->RollRandomFloatInRange(-m_worldCameraShakeDisp, m_worldCameraShakeDisp);
		m_worldCamera[0].Translate2D(Vec2(offset, offset));
		m_worldCamera[1].Translate2D(Vec2(offset, offset));
		m_worldCameraShakeDisp *= 1.0f - m_worldCameraShakeDecay;
	}
}

void Scene::Render()
{
	if (m_cameraEntity[1] == nullptr)
	{
		m_cameraIndex = 0;
		g_theRenderer->BeginCamera(m_worldCamera[0]);
		RenderWorld();
		g_theRenderer->EndCamera(m_worldCamera[0]);

		g_theRenderer->BeginCamera(m_screenCamera[0]);
		RenderUI();
		g_theRenderer->EndCamera(m_screenCamera[0]);
	}
	else
	{
		for (int i : {0, 1})
		{
			m_cameraIndex = i;
			g_theRenderer->BeginCamera(m_worldCamera[i]);
			RenderWorld();
			g_theRenderer->EndCamera(m_worldCamera[i]);

			g_theRenderer->BeginCamera(m_screenCamera[i]);
			RenderUI();
			g_theRenderer->EndCamera(m_screenCamera[i]);
		}
	}

	m_cameraIndex = 0;
}

void Scene::ShakeScreen(float maxDisplacement, float decay)
{
	m_worldCameraShakeDisp = maxDisplacement;
	m_worldCameraShakeDecay = decay;
}

float Scene::GetLifeTime() const
{
	return (float)m_clock.GetTotalTime();
}

const Camera& Scene::GetWorldCamera() const
{
	return m_worldCamera[m_cameraIndex];
}

const Camera& Scene::GetScreenCamera() const
{
	return m_screenCamera[m_cameraIndex];
}

void Scene::SetWorldCameraEntity(Player* actor, Player* secondActor /* = nullptr */)
{
	m_cameraEntity[0] = actor;
	m_cameraEntity[1] = secondActor;
}

Player* Scene::GetWorldCameraEntity() const
{
	return m_cameraEntity[m_cameraIndex];
}

const Clock* Scene::GetClock() const
{
	return &m_clock;
}

Clock* Scene::GetClock()
{
	return &m_clock;
}

