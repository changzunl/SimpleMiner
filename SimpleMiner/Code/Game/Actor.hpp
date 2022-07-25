#pragma once

#include "GameCommon.hpp"
#include "ActorUID.hpp"
#include "Components.hpp"
#include "Faction.hpp"
#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include <vector>

typedef std::vector<EntityComponent*> ComponentList;

class World;
class SpawnInfo;
class ActorDefinition;
class Controller;

class Actor
{
	friend class World;

public:

	Actor(World* map, const SpawnInfo* definition, const ActorUID& uid);
	~Actor();

	// lifecycle functions
	virtual void     OnCollideWithEntity(Actor* entityToCollide);
	virtual void     Update(float deltaSeconds);
	virtual void     Die();
	virtual void     Render() const;

	// constant functions
	int              GetType() const;
	const ActorUID&  GetUID() const;
	bool             IsDead() const                                        { return m_dead; }
	bool             IsGarbage() const                                     { return m_garbage; }
	bool             IsOutOfWorld() const;
	bool             IsOfTypeMask(int entityTypeMask) const;
	Vec3             GetPosition() const;
	EulerAngles      GetOrientation() const;
	Vec3             GetEyePosition() const;
	Vec3             GetForward() const;
	bool             IsProjectile() const;

	// utilities
	template<typename T>
	T*               GetComponent()                                        { return reinterpret_cast<T*>(GetComponent(T::TYPE)); }
	void             OnPossessed(Controller* controller);
	void             OnUnpossessed(Controller* controller);
	void             MoveInDirection(Vec3 direction, float speed);
	void             TurnInDirection(EulerAngles direction);

	// event
	void             CallEvent(const char* type, const char* name);

private:
	void             HandleEventEffect(const char* name);

protected:
	void             AddComponent(EntityComponent* component);
	EntityComponent* GetComponent(EntityComponentType type);
	ComponentList    GetComponents(EntityComponentType type) const;

public:
	World* const                        m_world                = nullptr;
	const ActorDefinition* const        m_definition           = nullptr;
	const int                           m_type;
	Faction                             m_faction              = Faction::NEUTRAL;
							            
	Stopwatch                           m_lifetimeStopwatch;   
	bool                                m_dead                 = false;
	bool                                m_garbage              = false;
					                    
	bool                                m_dieOnSpawn           = false;
	bool                                m_visible              = true;
					                    
	Transformation                      m_transform;
	Transformation                      m_prevTransform;
					                    
	Controller*                         m_controller           = nullptr;
	Controller*                         m_defaultController    = nullptr;
					                    
	Physics*                            m_physics              = nullptr;
	StaticMesh*                         m_debugMesh            = nullptr;
					                    
	// fx			                    
    SoundSource*                        m_sound                = nullptr;
					                    
	ActorUID                            m_projectileOwner      = ActorUID::INVALID;
					                    
private:			                    
	ActorUID                            m_uid                  = ActorUID::INVALID;
	ComponentList                       m_comps;
	ComponentList                       m_tickableComps;
};

