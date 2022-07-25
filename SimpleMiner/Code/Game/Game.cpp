#include "Game.hpp"

#include "App.hpp"
#include "ScenePlaying.hpp"
#include "SceneAttract.hpp"
#include "Scene.hpp"
#include "World.hpp"
#include "ActorDefinition.hpp"
#include "BlockDef.hpp"
#include "BlockMaterialDef.hpp"
#include "BlockSetDefinition.hpp"
#include "SoundClip.hpp"

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Networking.hpp"

NetworkManagerClient* NET_CLIENT;
NetworkManagerServer* NET_SERVER;

int CLIENT_ID = -1;

enum PacketType
{
	REGISTER,
	MESSAGE,
	START_GAME,
	PLAY_SPAWN_ACTOR,
	PLAY_ACTOR_STATE,
	PLAY_ACTOR_DESTROY,
	PLAY_INPUT,
	PLAY_PLAYER_STATE,
	QUIT_GAME,
};

bool Command_Connect(EventArgs& args)
{
	std::string host = args.GetValue("host", "127.0.0.1");
	std::string port = args.GetValue("port", "25564");

	g_theConsole->AddLine(DevConsole::LOG_INFO, Stringf("Connecting to %s:%s...", host.c_str(), port.c_str()));

	NetErrResult result;
	result = NET_CLIENT->CreateClient(host.c_str(), port.c_str());

	g_theConsole->AddLine(DevConsole::LOG_INFO, Stringf("Connection state: %s(%d)", result.m_errMsg.c_str(), result.m_errCode));
	return true;
}

bool Command_Server(EventArgs& args)
{
	std::string port = args.GetValue("port", "25564");

	g_theConsole->AddLine(DevConsole::LOG_INFO, Stringf("Starting server on %s...", port.c_str()));

	NetErrResult result;
	result = NET_SERVER->CreateServer(port.c_str());

	g_theConsole->AddLine(DevConsole::LOG_INFO, Stringf("Connection state: %s(%d)", result.m_errMsg.c_str(), result.m_errCode));
	return true;
}

bool Command_SendMessage(EventArgs& args)
{
	std::string message = args.GetValue("message", "(empty)");

	g_theConsole->AddLine(DevConsole::LOG_INFO, Stringf("> %s", message.c_str()));

	if (CLIENT_ID == -1)
	{
		int count = int(Clock::GetSystemClock().GetTotalTime() * 10.0f) % 10;

		for (int i = 0; i < count; i++)
			CLIENT_ID = g_theGame->m_rng->RollRandomIntInRange(0, 0xFF);
	}

	NetErrResult result;

	Packet packet;
	packet.m_type = PacketType::MESSAGE;
	packet.WriteString(Stringf("doom%d", CLIENT_ID));
	packet.WriteString(message);

	NET_CLIENT->m_queue.push_back(packet);
	return true;
}


bool Command_Disconnect(EventArgs& args)
{
	UNUSED(args);

	NET_CLIENT->ReleaseClient();
	g_theConsole->AddLine(DevConsole::LOG_INFO, "Disconnected!");
	return true;
}


bool Command_Stop(EventArgs& args)
{
	UNUSED(args);

	NET_SERVER->ReleaseServer();
	g_theConsole->AddLine(DevConsole::LOG_INFO, "Server stopped!");
	return true;
}


bool Command_Controls(EventArgs& args)
{
	UNUSED(args);

	g_theConsole->AddLine(DevConsole::LOG_INFO, "Controls:");
	g_theConsole->AddLine(DevConsole::LOG_INFO, "WASD / LEFT JOYSTICK - move");
	g_theConsole->AddLine(DevConsole::LOG_INFO, "LEFT MOUSE / RIGHT TRIGGER - fire");
	g_theConsole->AddLine(DevConsole::LOG_INFO, "MOUSE WHEEL / LEFT / RIGHT - switch weapon");
	return true;
}


bool Command_RaycastDebugToggle(EventArgs& args)
{
	UNUSED(args);

	World* map = g_theGame->GetCurrentMap();
	if (map)
	{
		map->m_debugRayVisible = !map->m_debugRayVisible;
		g_theConsole->AddLine(DevConsole::LOG_INFO, Stringf("Debug ray rendering is turned %s", map->m_debugRayVisible ? "on" : "off").c_str());
	}
	else
	{
		g_theConsole->AddLine(DevConsole::LOG_WARN, "No map is loaded!");
	}
	return true;
}

bool InitializeDebugCommands()
{
	g_theEventSystem->SubscribeEventCallbackFunction("Controls", Command_Controls);
	g_theEventSystem->SubscribeEventCallbackFunction("Connect", Command_Connect);
	g_theEventSystem->SubscribeEventCallbackFunction("Server", Command_Server);
	g_theEventSystem->SubscribeEventCallbackFunction("Send", Command_SendMessage);
	g_theEventSystem->SubscribeEventCallbackFunction("Disconnect", Command_Disconnect);
	g_theEventSystem->SubscribeEventCallbackFunction("Stop", Command_Stop);
	g_theEventSystem->SubscribeEventCallbackFunction("RaycastDebugToggle", Command_RaycastDebugToggle);
	DebugAddMessage("", -5.0f, Rgba8(255, 0, 0), Rgba8(0, 255, 0));

	NET_CLIENT->RegisterHandler(PacketType::MESSAGE, [](Packet& pkt) {
		std::string msg = pkt.ReadString();
		DebugAddMessage(msg, 2.0f, Rgba8::WHITE, Rgba8::WHITE);
		g_theConsole->AddLine(Rgba8::WHITE, Stringf("[%s] %s", "CHAT", msg.c_str()));
	});

	NET_SERVER->RegisterHandler(PacketType::MESSAGE, [](int connection, Packet& pkt) {
		UNUSED(connection);

		std::string name = pkt.ReadString();
		std::string msg = pkt.ReadString();

		g_theConsole->AddLine(Rgba8::WHITE, Stringf("[LOG] (CLIENT CHAT) %s: %s", name.c_str(), msg.c_str()));

		Packet packet;
		packet.m_type = PacketType::MESSAGE;
		packet.WriteString(Stringf("%s: %s", name.c_str(), msg.c_str()));
		NET_SERVER->Broadcast(packet);
	});

	return true;
}

Game::Game()
	: m_rng(new RandomNumberGenerator())
{
}

Game::~Game()
{
}

void Game::Initialize()
{
	WORLD_DEBUG_NO_TEXTURE    = g_gameConfigBlackboard.GetValue("debugWorldNoTexture", false);
	WORLD_DEBUG_NO_SHADER     = g_gameConfigBlackboard.GetValue("debugWorldNoShader", false);
	WORLD_DEBUG_STEP_LIGHTING = g_gameConfigBlackboard.GetValue("debugWorldStepLighting", false);

	m_soundVolume = 0.5f;

	StartupNetworking();
	NET_CLIENT = new NetworkManagerClient();
	NET_SERVER = new NetworkManagerServer();

	static bool init = InitializeDebugCommands();

	InitializeAudio();

	ActorDefinition::ClearDefinitions();
	BlockSetDefinition::InitializeDefinition("Data/Definitions/BlockDefinitions.xml");
	Blocks::Initialize();
// 	ActorDefinition::InitializeDefinitions("Data/Definitions/ProjectileActorDefinitions.xml");
 	ActorDefinition::InitializeDefinitions("Data/Definitions/ActorDefinitions.xml");

	m_currentScene = new SceneAttract(this);
	m_currentScene->Initialize();
}

void Game::Shutdown()
{
	if (m_newScene != nullptr)
	{
		delete m_newScene;
		m_newScene = nullptr;
	}
	m_currentScene->Shutdown();
	delete m_currentScene;
	m_currentScene = nullptr;

	ShutdownAudio();

	NET_CLIENT->ReleaseClient();
	NET_SERVER->ReleaseServer();
	delete NET_CLIENT;
	delete NET_SERVER;
	NET_CLIENT = nullptr;
	NET_SERVER = nullptr;
	StopNetworking();
}

void Game::Update()
{
	NET_CLIENT->NetworkTickClient();
	NET_SERVER->NetworkTickServer();

	if (m_newScene != nullptr)
	{
		m_currentScene->Shutdown();
		delete m_currentScene;
		m_currentScene = m_newScene;
		m_newScene = nullptr;
		m_currentScene->Initialize();
	}

	m_currentScene->Update();
	m_currentScene->UpdateCamera();
}

void Game::InitializeAudio()
{
	m_soundTestSound           = new SoundClip("Data/Audio/TestSound.mp3");
	m_bgmAttract               = new SoundClip("Data/Audio/Music/Beginning.mp3");
	m_bgmPlaying               = new SoundClip("Data/Audio/Music/Living Mice.mp3");
	m_clickSound               = new SoundClip("Data/Audio/Click.mp3");
}

void Game::ShutdownAudio()
{
	delete m_soundTestSound;
	delete m_bgmAttract;
	delete m_bgmPlaying;
	delete m_clickSound;
}

void Game::Render() const
{
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindTexture(nullptr);

	m_currentScene->Render();
}

void Game::LoadScene(Scene* scene)
{
	if (m_newScene != nullptr)
	{
		delete m_newScene;
		m_newScene = nullptr;
	}
	m_newScene = scene;
}

Scene* Game::GetCurrentScene()
{
	return m_currentScene;
}

World* Game::GetCurrentMap()
{
	return m_currentMap;
}

void Game::SetCurrentMap(World* map)
{
	m_currentMap = map;
}

void Game::PlayBGM(const SoundClip* sound)
{
	m_bgmInst.Stop();
	m_bgmInst = sound->PlayMusic(1.0f);
}

void Game::StopBGM()
{
	m_bgmInst.Stop();
}

