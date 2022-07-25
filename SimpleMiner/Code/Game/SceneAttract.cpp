#include "SceneAttract.hpp"

#include "App.hpp"
#include "GameCommon.hpp"
#include "RenderUtils.hpp"
#include "Game.hpp"
#include "SoundClip.hpp"
#include "RenderUtils.hpp"
#include "Networking.hpp"
#include "ScenePlaying.hpp"
#include "SceneTestJobs.hpp"

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SimpleTriangleFont.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"

#include <math.h>
#include <vector>

bool g_useSkyBlock = false;

SceneAttract::SceneAttract(Game* game) : Scene(game)
{
	m_startVertices[0] = Vertex_PCU(Vec3(-0.7f,  1.0f, 0.0f), Rgba8(0, 255, 0, 255), Vec2());
	m_startVertices[1] = Vertex_PCU(Vec3(-0.7f, -1.0f, 0.0f), Rgba8(0, 255, 0, 255), Vec2());
	m_startVertices[2] = Vertex_PCU(Vec3( 1.0f,  0.0f, 0.0f), Rgba8(0, 255, 0, 255), Vec2());
}

SceneAttract::~SceneAttract()
{
}

void SceneAttract::Initialize()
{
	g_useSkyBlock = false;

	g_theGame->PlayBGM(g_theGame->m_bgmAttract);
}

void SceneAttract::Shutdown()
{
}

void SceneAttract::Update()
{
	if (m_isQuitting)
	{
		m_quitTimeCounter -= (float)m_clock.GetDeltaTime();
		if (m_quitTimeCounter <= 0.0f)
		{
			g_theApp->Quit();
			return;
		}
	}
	else
	{
		HandleInput();
	}

	float shake = SinDegrees(GetLifeTime() * 50) * .25f;
	if (shake > 0)
	{
		// ShakeScreen(shake);
	}
}

void SceneAttract::UpdateCamera()
{
	m_worldCamera[0].SetPerspectiveView(float(g_theWindow->GetClientDimensions().x) / float(g_theWindow->GetClientDimensions().y), 60.0f, 0.1f, 100.0f);
	m_worldCamera[0].SetRenderTransform(Vec3(0, -1, 0), Vec3(0, 0, 1), Vec3(1, 0, 0));
	m_worldCamera[0].SetViewTransform(Vec3::ZERO, EulerAngles(0, 0, GetLifeTime() * 50));

	m_screenCamera[0].SetOrthoView(g_theRenderer->GetViewport().m_mins, g_theRenderer->GetViewport().m_maxs);

	Scene::UpdateCamera();
}

void SceneAttract::RenderWorld() const
{
	g_theRenderer->SetFillMode(FillMode::SOLID);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMask(false);
	g_theRenderer->SetDepthTest(DepthTest::ALWAYS);
	g_theRenderer->SetCullMode(CullMode::NONE);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

//   m_game->RenderEntities(ENTITY_MASK_EFFECT);
//   m_game->RenderEntities(ENTITY_MASK_ENEMY);
	g_theRenderer->SetTintColor(Rgba8::WHITE);
	g_theRenderer->BindTexture(nullptr);


	g_theRenderer->SetFillMode(FillMode::WIREFRAME);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetDepthMask(true);
	g_theRenderer->SetDepthTest(DepthTest::LESSEQUAL);
	g_theRenderer->SetTintColor(Rgba8::WHITE);
	g_theRenderer->BindTexture(nullptr);
}

void SceneAttract::RenderUI() const
{
	DebugAddMessage(Stringf("Chat Client State: %s", GetNameFromType(NET_CLIENT->m_state)), 0.0f, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddMessage(Stringf("Chat Server State: %s(connections=%d)", GetNameFromType(NET_SERVER->m_state), NET_SERVER->GetActiveConnections()), 0.0f, Rgba8::WHITE, Rgba8::WHITE);

	g_theRenderer->SetFillMode(FillMode::SOLID);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMask(false);
	g_theRenderer->SetDepthTest(DepthTest::ALWAYS);
	g_theRenderer->SetCullMode(CullMode::NONE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	RenderUILogoText();
	RenderUIMenuButton();

	DebugRenderScreen(GetScreenCamera());
}

void SceneAttract::RenderUILogoText() const
{
	std::vector<Vertex_PCU> verts;

	// Logo
	Vec2 textPosition = g_theRenderer->GetViewport().GetPointAtUV(Vec2(0.5f, 0.8f));
	float fontSize = Clamp(GetLifeTime() * 300.0f, 0.0f, 100.0f);
	AddVertsForText(verts, "SIMPLE MINER", 1.0f, 0.5f);
	RenderFontVertices((int) verts.size(), &verts[0], textPosition, Rgba8(), fontSize, true);
	verts.clear();

	// Propaganda
	if (fontSize >= 100.0f)
	{
		fontSize = RangeMap(SinDegrees(GetLifeTime() * 180.0f), -1.0f, 1.0f, 15.0f, 25.0f);
	}
	else
	{
		fontSize *= 0.25f;
	}
	textPosition = g_theRenderer->GetViewport().GetPointAtUV(Vec2(0.5f, 0.5f));
	AddVertsForText(verts, "SPACE/[START] TO START!", 2.0f, 0.5f);
	RenderFontVertices((int)verts.size(), &verts[0], textPosition, Rgba8(255, 255, 0, 255), fontSize, true);
	verts.clear();
	textPosition = g_theRenderer->GetViewport().GetPointAtUV(Vec2(0.5f, 0.45f));
	AddVertsForText(verts, "S FOR SKYBLOCK!", 2.0f, 0.5f);
	RenderFontVertices((int)verts.size(), &verts[0], textPosition, Rgba8(255, 255, 0, 255), fontSize, true);
	verts.clear();
}

void SceneAttract::RenderUIMenuButton() const
{
}

void SceneAttract::HandleInput()
{
	// input handle, return = handled one event


	// controller join/quit
	for (int i = 0; i < NUM_XBOX_CONTROLLERS; i++)
	{
		const auto& controller = g_theInput->GetXboxController(i);

		if (controller.IsConnected())
		{
			if (controller.WasButtonJustPressed(XboxButtonID::XBOXBTN_START))
			{
				m_controller = (XboxController*)&controller;

				m_menuSelectionIdx = 0;
				SelectCurrentMenu();
				return;
			}
			if (controller.WasButtonJustPressed(XboxButtonID::XBOXBTN_BACK))
			{
				g_theApp->Quit();
				return;
			}
		}
	}

	// keyboard join/quit
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{
		g_theApp->Quit();
		return;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_S))
	{
		g_useSkyBlock = true;
		m_menuSelectionIdx = 0;
		SelectCurrentMenu();
		return;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE))
	{
		m_menuSelectionIdx = 0;
		SelectCurrentMenu();
		return;
	}

	// HandleMenuCursor();
	// HandleMenuSelection();

	if (g_theInput->WasKeyJustPressed(KEYCODE_J))
	{
		SceneTestJobs* testJobs = new SceneTestJobs(m_game);
		m_game->LoadScene(testJobs);
		m_game->m_clickSound->PlaySound();
		return;
	}

	g_theInput->SetMouseMode(false, false, false);
}

void SceneAttract::HandleMenuCursor()
{
// 	XboxController controller = g_theInput->GetFirstAvaliableXboxController();
// 	if (controller.IsConnected())
// 	{
// 		if (controller.WasButtonJustPressed(XboxButtonID::XBOXBTN_UP))
// 		{
// 			m_menuSelectionIdx = ClampInt(m_menuSelectionIdx - 1, 0, 2);
// 			return;
// 		}
// 		if (controller.WasButtonJustPressed(XboxButtonID::XBOXBTN_DOWN))
// 		{
// 			m_menuSelectionIdx = ClampInt(m_menuSelectionIdx + 1, 0, 2);
// 			return;
// 		}
// 	}
// 
// 	if (g_theInput->WasKeyJustPressed(KEYCODE_UP))
// 	{
// 		m_menuSelectionIdx = ClampInt(m_menuSelectionIdx - 1, 0, 2);
// 		return;
// 	}
// 	if (g_theInput->WasKeyJustPressed(KEYCODE_DOWN))
// 	{
// 		m_menuSelectionIdx = ClampInt(m_menuSelectionIdx + 1, 0, 2);
// 		return;
// 	}
}

void SceneAttract::HandleMenuSelection()
{
// 	XboxController controller = g_theInput->GetFirstAvaliableXboxController();
// 	if (controller.IsConnected())
// 	{
// 		if (controller.WasButtonJustPressed(XboxButtonID::XBOXBTN_START) || controller.WasButtonJustPressed(XboxButtonID::XBOXBTN_A))
// 		{
// 			SelectCurrentMenu();
// 			return;
// 		}
// 	}
// 
// 	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE) || g_theInput->WasKeyJustPressed(KEYCODE_ENTER))
// 	{
// 		SelectCurrentMenu();
// 		return;
// 	}
}

void SceneAttract::SelectCurrentMenu()
{
	switch (m_menuSelectionIdx)
	{
	case 0: // start
	{
		ScenePlaying* game = new ScenePlaying(m_game);
		m_game->LoadScene(game);
		m_game->m_clickSound->PlaySound();
		return;
	}
	case 2: // quit
	{
		m_isQuitting = true;
		m_quitTimeCounter = 1.0f;
		return;
	}
	}
}

