#include "Controller.hpp"

#include "GameCommon.hpp"
#include "Game.hpp"
#include "World.hpp"
#include "Actor.hpp"

Controller::Controller()
{
}

Controller::~Controller()
{

}

void Controller::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

}

void Controller::Possess(Actor* actor)
{
	Actor* previous = GetActor();
	if (previous)
		previous->OnUnpossessed(this);

	if (actor)
	{
		m_actorUID = actor->GetUID();
		actor->OnPossessed(this);
	}
	else
	{
		m_actorUID = ActorUID::INVALID;
	}
	m_skipFrame = true;
}

Actor* Controller::GetActor() const
{
	return *m_actorUID;
}

