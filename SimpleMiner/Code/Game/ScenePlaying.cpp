#include "ScenePlaying.hpp"

#include "GameCommon.hpp"
#include "RenderUtils.hpp"
#include "App.hpp"
#include "Game.hpp"
#include "SceneAttract.hpp"
#include "World.hpp"
#include "Actor.hpp"
#include "ActorDefinition.hpp"
#include "Player.hpp"
#include "Faction.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Window/Window.hpp"

ScenePlaying::ScenePlaying(Game* game) : Scene(game)
{
}

ScenePlaying::~ScenePlaying()
{
}

void ScenePlaying::Initialize()
{
	InitializeDebugRender();
	InitializeMap();
	InitializeEntities();

	g_theGame->PlayBGM(g_theGame->m_bgmPlaying);
}

void ScenePlaying::InitializeEntities()
{
}

void ScenePlaying::InitializeMap()
{
	m_map = new World();
	m_game->SetCurrentMap(m_map);
	m_map->Initialize();
	SetWorldCameraEntity(m_map->m_player[0], m_map->m_player[1]);

	g_theAudio->SetNumListeners(1);
}

void ScenePlaying::InitializeDebugRender()
{
	DebugRenderClear();

	Mat4x4 matrix;
	DebugAddWorldText(" x - axis", matrix, 1.0f, Vec2(), -1.0f, Rgba8(255, 0, 0), Rgba8(255, 0, 0), DebugRenderMode::USEDEPTH);
	matrix.AppendZRotation(90.0f);
	DebugAddWorldText(" y - axis", matrix, 1.0f, Vec2(), -1.0f, Rgba8(0, 255, 0), Rgba8(0, 255, 0), DebugRenderMode::USEDEPTH);
	matrix.SetIdentity();
	matrix.AppendYRotation(-90.0f);
	DebugAddWorldText(" z - axis", matrix, 1.0f, Vec2(), -1.0f, Rgba8(0, 0, 255), Rgba8(0, 0, 255), DebugRenderMode::USEDEPTH);

	DebugAddWorldBasis(Mat4x4::IDENTITY, -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);
}

void ScenePlaying::Shutdown()
{
	m_cameraEntity[0] = nullptr;
	m_cameraEntity[1] = nullptr;
	m_map->Shutdown();
	delete m_map;
	m_map = nullptr;

	DebugRenderClear();
}

void ScenePlaying::Update()
{
	m_map->Update((float)m_clock.GetDeltaTime());
	
	HandleInput();
}

void ScenePlaying::UpdateCamera()
{
	if (m_cameraEntity[1] == nullptr)
	{
		m_worldCamera[0].SetPerspectiveView(float(g_theWindow->GetClientDimensions().x) / float(g_theWindow->GetClientDimensions().y), m_cameraEntity[0]->GetCameraFOV(), 0.1f, 1000.0f);
		m_worldCamera[0].SetRenderTransform(Vec3(0, -1, 0), Vec3(0, 0, 1), Vec3(1, 0, 0));
		Transformation camTrans = m_cameraEntity[0]->GetCameraTransform();
		m_worldCamera[0].SetViewTransform(camTrans.m_position, camTrans.m_orientation);

		m_screenCamera[0].SetOrthoView(g_theRenderer->GetViewport().m_mins, g_theRenderer->GetViewport().m_maxs);

		m_worldCamera[0].SetViewport(AABB2::ZERO_TO_ONE);
		m_screenCamera[0].SetViewport(AABB2::ZERO_TO_ONE);
	}
	else
	{
		m_worldCamera[0].SetViewport(AABB2(0.0f, 0.0f, 1.0f, 0.5f));
		m_worldCamera[1].SetViewport(AABB2(0.0f, 0.5f, 1.0f, 1.0f));
		m_screenCamera[0].SetViewport(AABB2(0.0f, 0.0f, 1.0f, 0.5f));
		m_screenCamera[1].SetViewport(AABB2(0.0f, 0.5f, 1.0f, 1.0f));

		for (int i : {0, 1})
		{
			g_theRenderer->BeginCamera(m_worldCamera[i]);
			m_worldCamera[i].SetPerspectiveView(g_theRenderer->GetViewport().GetDimensions().x / g_theRenderer->GetViewport().GetDimensions().y, m_cameraEntity[i]->GetCameraFOV(), 0.1f, 100.0f);
			m_worldCamera[i].SetRenderTransform(Vec3(0, -1, 0), Vec3(0, 0, 1), Vec3(1, 0, 0));
			Transformation camTrans = m_cameraEntity[i]->GetCameraTransform();
			m_worldCamera[i].SetViewTransform(camTrans.m_position, camTrans.m_orientation);

			m_screenCamera[i].SetOrthoView(g_theRenderer->GetViewport().m_mins, g_theRenderer->GetViewport().m_maxs);
			g_theRenderer->EndCamera(m_worldCamera[i]);
		}
	}

	Scene::UpdateCamera();
}

void ScenePlaying::RenderWorld() const
{
	g_theRenderer->SetFillMode(FillMode::SOLID);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetDepthMask(true);
	g_theRenderer->SetDepthTest(DepthTest::LESSEQUAL);
	g_theRenderer->SetCullMode(CullMode::BACK);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

	m_map->Render();
}

void ScenePlaying::RenderUI() const
{
	g_theRenderer->SetDepthMask(false);
	g_theRenderer->SetDepthTest(DepthTest::ALWAYS);
	m_map->RenderPost();

	g_theRenderer->SetFillMode(FillMode::SOLID);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMask(false);
	g_theRenderer->SetDepthTest(DepthTest::ALWAYS);
	g_theRenderer->SetCullMode(CullMode::NONE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	g_theRenderer->SetTintColor(Rgba8::WHITE);
	g_theRenderer->SetModelMatrix(Mat4x4::IDENTITY);
	g_theRenderer->BindTexture(nullptr);
	RenderHUD();

	DebugRenderScreen(GetScreenCamera());
}

void ScenePlaying::RenderHUD() const
{
	static VertexList verts;
	verts.clear();

	AABB2 screenBox = AABB2(GetScreenCamera().GetOrthoBottomLeft(), GetScreenCamera().GetOrthoTopRight());
	Vec2 screenCenter = screenBox.GetCenter();

	// cross hair
	AABB2 crosshairBox;
	crosshairBox.SetCenter(screenCenter);
	crosshairBox.SetDimensions(Vec2(screenBox.GetDimensions().y * 0.05f, screenBox.GetDimensions().y * 0.05f));
	AddVertsForAABB2D(verts, crosshairBox, Rgba8::WHITE);
	g_theRenderer->BindTexture(g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Reticle.png"));
	g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());
	g_theRenderer->BindTexture(nullptr);
	verts.clear();
}

void ScenePlaying::HandleInput()
{
	auto& controller = g_theInput->GetFirstAvaliableXboxController();
	if (controller.IsConnected())
	{
		if (controller.WasButtonJustPressed(XboxButtonID::XBOXBTN_BACK))
		{
			m_game->LoadScene(new SceneAttract(m_game));
			return;
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{
		m_game->LoadScene(new SceneAttract(m_game));
		return;
	}

	HandleCheatInput();
	// HandleLightInput();

	g_theInput->SetMouseMode(true, false, true);
}

void ScenePlaying::HandleCheatInput()
{
	// handle slow mode
	m_isSlowMo = g_theInput->IsKeyDown('T');
	m_clock.SetTimeDilation(m_isSlowMo ? 0.1 : 1.0);

	// handle pause
	if (g_theInput->WasKeyJustPressed('P'))
	{
		m_clock.TogglePause();
	}

	// handle one frame step
	if (g_theInput->WasKeyJustPressed('O'))
	{
		m_clock.StepFrame();
	}

	// handle kill all actors
	if (g_theInput->WasKeyJustPressed(KEYCODE_K))
	{
		m_map->RemoveEntities(false, 1 << Faction::DEMON);
	}
}

void ScenePlaying::HandleLightInput()
{
	LightConstants& lightConsts = g_theRenderer->GetLightConstants();

	bool displayValue = false;
	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		displayValue = true;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
	{
		displayValue = true;
		lightConsts.AmbientIntensity = Clamp(lightConsts.AmbientIntensity - 0.05f, 0.0f, 1.0f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F3))
	{
		displayValue = true;
		lightConsts.AmbientIntensity = Clamp(lightConsts.AmbientIntensity + 0.05f, 0.0f, 1.0f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F4))
	{
		displayValue = true;
		lightConsts.SunIntensity = Clamp(lightConsts.SunIntensity - 0.05f, 0.0f, 1.0f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F5))
	{
		displayValue = true;
		lightConsts.SunIntensity = Clamp(lightConsts.SunIntensity + 0.05f, 0.0f, 1.0f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
	{
		displayValue = true;
		EulerAngles rotation = DirectionToRotation(lightConsts.SunDirection);
		rotation.m_pitchDegrees = rotation.m_pitchDegrees - 5.0f;
		lightConsts.SunDirection = rotation.GetVectorXForward();
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
	{
		displayValue = true;
		EulerAngles rotation = DirectionToRotation(lightConsts.SunDirection);
		rotation.m_pitchDegrees = rotation.m_pitchDegrees + 5.0f;
		lightConsts.SunDirection = rotation.GetVectorXForward();
	}

	if (displayValue)
	{
		const char* msgRaw = "Lighting: ambient=%f, directional=%f, direction=%f(%f,%f,%f)";
		std::string msg = Stringf(msgRaw, lightConsts.AmbientIntensity, lightConsts.SunIntensity, DirectionToRotation(lightConsts.SunDirection).m_pitchDegrees, lightConsts.SunDirection.x, lightConsts.SunDirection.y, lightConsts.SunDirection.z);
		DebugAddMessage(msg, 2.0f, Rgba8::WHITE, Rgba8::WHITE);
	}
}

