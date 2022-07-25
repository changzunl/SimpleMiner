#pragma once

#include "GameCommon.hpp"
#include "SoundClip.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include <vector>

class RandomNumberGenerator;

class Scene;
class World;
class SoundClip;
class SoundInst;

class Game {

public:
	Game();
	~Game();
	
	// lifecycle handlers
	void Initialize();
	void Shutdown();
	void Update();
	void Render() const;

	// scene services
	void LoadScene(Scene* scene);
	Scene* GetCurrentScene();
	World*   GetCurrentMap();
	void   SetCurrentMap(World* map);

	void   PlayBGM(const SoundClip* sound);
	void   StopBGM();

private:
	void InitializeAudio();
	void ShutdownAudio();

public:
	RandomNumberGenerator* m_rng            = nullptr;
	float                  m_soundVolume    = 1.0f;
	int                    m_difficulty     = DIFFICULTY_NORMAL;
	SoundClip*             m_soundTestSound = nullptr;
	SoundClip*             m_bgmAttract     = nullptr;
	SoundClip*             m_bgmPlaying     = nullptr;
	SoundClip*             m_clickSound     = nullptr;
	SoundInst              m_bgmInst;

private:
	Scene*                      m_currentScene                   = nullptr;
	Scene*                      m_newScene                       = nullptr;

	World*                        m_currentMap                     = nullptr;
};

