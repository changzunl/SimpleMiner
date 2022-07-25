#pragma once

#include "ActorUID.hpp"

class Actor;

class Controller
{
	friend class Actor;

public:
	Controller();
	virtual ~Controller();

	virtual void Update(float deltaSeconds);
	
	virtual void Possess(Actor* actor);
	Actor* GetActor() const;

protected:
	ActorUID    m_actorUID      = ActorUID::INVALID;
	bool        m_skipFrame     = false;

public:
	float       m_killCount     = 0;
	float       m_flagCount     = 0;
	float       m_waveWarn      = 0.0f;
};

