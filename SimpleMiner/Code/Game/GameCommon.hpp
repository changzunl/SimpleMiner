#pragma once

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/IntVec2.hpp"

#define UNUSED(x) (void)(x);

struct Rgba8;
struct Vec2;
struct Vec3;
struct Vertex_PCU;

constexpr int DIFFICULTY_EASY      = 1;
constexpr int DIFFICULTY_NORMAL    = 2;
constexpr int DIFFICULTY_HARD      = 3;

constexpr unsigned int CHUNK_SIZE_BITWIDTH_XY = 4;
constexpr unsigned int CHUNK_SIZE_BITWIDTH_Z  = 7;
constexpr unsigned int CHUNK_SIZE_XY          = 1 << CHUNK_SIZE_BITWIDTH_XY;
constexpr unsigned int CHUNK_SIZE_Z           = 1 << CHUNK_SIZE_BITWIDTH_Z;
constexpr unsigned int CHUNK_SIZE_BLOCKS      = CHUNK_SIZE_XY * CHUNK_SIZE_XY * CHUNK_SIZE_Z;
constexpr unsigned int CHUNK_MAX_X            = CHUNK_SIZE_XY - 1;
constexpr unsigned int CHUNK_MAX_Y            = CHUNK_SIZE_XY - 1;
constexpr unsigned int CHUNK_MAX_Z            = CHUNK_SIZE_Z  - 1;

constexpr unsigned int CHUNK_BITSHIFT_X  = 0;
constexpr unsigned int CHUNK_BITSHIFT_Y  = CHUNK_SIZE_BITWIDTH_XY;
constexpr unsigned int CHUNK_BITSHIFT_Z  = CHUNK_SIZE_BITWIDTH_XY + CHUNK_SIZE_BITWIDTH_XY;
constexpr unsigned int CHUNK_BLOCKMASK_X = CHUNK_MAX_X << CHUNK_BITSHIFT_X;
constexpr unsigned int CHUNK_BLOCKMASK_Y = CHUNK_MAX_Y << CHUNK_BITSHIFT_Y;
constexpr unsigned int CHUNK_BLOCKMASK_Z = CHUNK_MAX_Z << CHUNK_BITSHIFT_Z;

class App;
class InputSystem;
class JobSystem;
class Window;
class Renderer;
class AudioSystem;
class Game;
class BitmapFont;
class NetworkManagerClient;
class NetworkManagerServer;

extern App* g_theApp;
extern JobSystem* g_theJobSystem;
extern InputSystem* g_theInput;
extern Window* g_theWindow;
extern Renderer* g_theRenderer;
extern AudioSystem* g_theAudio;
extern Game* g_theGame;
extern BitmapFont* g_testFont;

extern NetworkManagerClient* NET_CLIENT;
extern NetworkManagerServer* NET_SERVER;

extern bool WORLD_DEBUG_NO_TEXTURE;
extern bool WORLD_DEBUG_NO_SHADER;
extern bool WORLD_DEBUG_STEP_LIGHTING;
extern Vec3 CAMERA_WORLD_POSITION;

enum class BillboardType
{
	NONE,
	FACING,
	ALIGNED,
};

const char* GetNameFromType(BillboardType type);
BillboardType GetTypeByName(const char* name, BillboardType defaultType);

bool operator<(const IntVec2& a, const IntVec2& b);

class XboxController;

struct PlayerJoin
{
public:
	PlayerJoin() {};
	PlayerJoin(int order) : m_order(order) {};

public:
	int  m_order = 0;
	bool m_joined = false;
	bool m_isKeyboard = false;
	const XboxController* m_controller = nullptr;
};

