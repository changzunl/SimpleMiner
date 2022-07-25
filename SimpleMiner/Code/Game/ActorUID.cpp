#include "ActorUID.hpp"

#include "GameCommon.hpp"
#include "Game.hpp"
#include "World.hpp"

const ActorUID ActorUID::INVALID;

ActorUID::ActorUID()
	: m_data(0)
{
}

ActorUID::ActorUID(int index, int salt)
	: m_data((salt << 16) | (index & 0xFFFF))
{
}

void ActorUID::Invalidate()
{
	m_data = 0;
}

bool ActorUID::IsValid() const
{
	return GetMap()->IsEntityUIDValid(*this);
}

int ActorUID::GetIndex() const
{
	return m_data & 0xFFFF;
}

Actor* ActorUID::operator*() const
{
	return GetMap()->GetEntityFromUID(*this);
}

World* ActorUID::GetMap()
{
	return g_theGame->GetCurrentMap();
}

bool ActorUID::operator!=(const ActorUID& other) const
{
	return !operator==(other);
}

bool ActorUID::operator==(const ActorUID& other) const
{
	return m_data == other.m_data;
}

