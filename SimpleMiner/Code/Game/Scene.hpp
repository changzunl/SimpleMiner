#pragma once

#include "GameCommon.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/Camera.hpp"

class RandomNumberGenerator;

class Game;
class Player;

class Scene {

public:
	Scene(Game* game);
	virtual ~Scene();
	virtual void     Initialize() = 0;
	virtual void     Shutdown() = 0;
	virtual void     Update();
	virtual void     UpdateCamera();
	virtual void     Render();
	virtual void     RenderWorld() const = 0;
	virtual void     RenderUI() const = 0;
				     
	virtual void     ShakeScreen(float maxDisplacement, float decay = 0.1f);
				     
	float            GetLifeTime() const;
	const Camera&    GetWorldCamera() const;
	const Camera&    GetScreenCamera() const;
	void             SetWorldCameraEntity(Player* actor, Player* secondActor = nullptr);
	Player*          GetWorldCameraEntity() const;
	const Clock*     GetClock() const;
	Clock*           GetClock();

protected:
	Game*                       m_game = nullptr;
	Clock                       m_clock;
	Camera                      m_worldCamera[2];
	Camera                      m_screenCamera[2];
	float                       m_worldCameraShakeDisp = 0.0f;
	float                       m_worldCameraShakeDecay = 0.0f;
	Player*                     m_cameraEntity[2] = {};
	unsigned int                m_cameraIndex = 0;
};

