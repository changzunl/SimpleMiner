#pragma once

class Actor;
class World;

struct ActorUID
{
public:
	ActorUID();
	ActorUID( int index, int salt );

	void Invalidate();
	bool IsValid() const;
	int  GetIndex() const;
	bool operator==( const ActorUID& other ) const;
	bool operator!=( const ActorUID& other ) const;

	Actor* operator*() const;

	static const ActorUID INVALID;

private:
	static World* GetMap();

private:
	int m_data;
};

