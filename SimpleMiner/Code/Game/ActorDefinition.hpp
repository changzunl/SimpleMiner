#pragma once

#include "GameCommon.hpp"
#include "Faction.hpp"
#include "SoundClip.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/Vec2.hpp"
#include <vector>
#include <string>

namespace tinyxml2
{
	class XMLElement;
}
typedef tinyxml2::XMLElement XmlElement;

typedef size_t SoundID;

//------------------------------------------------------------------------------------------------
class ActorDefinition
{
public:
	bool LoadFromXmlElement( const XmlElement& element );

	std::string              m_name;
				             
	float                    m_physicsRadius                          = -1.0f;
	float                    m_physicsHeight                          = -1.0f;
	float                    m_walkSpeed                              = 0.0f;
	float                    m_runSpeed                               = 0.0f;
	float                    m_drag                                   = 0.0f;
	float                    m_turnSpeed                              = 0.0f;
	bool                     m_flying                                 = false;
	bool                     m_simulated                              = false;
	bool                     m_collidesWithWorld                      = false;
	bool                     m_collidesWithActors                     = false;
	float                    m_lifetime                               = -1.0f;
		                     
	bool                     m_canBePossessed                         = false;
	float                    m_eyeHeight                              = 0.0f;
	float                    m_cameraFOVDegrees                       = 60.0f;
		                     
	bool                     m_aiEnabled                              = false;
	std::string              m_aiType                                 = "simple";
	float                    m_sightRadius                            = 64.0f;
	float                    m_sightAngle                             = 180.0f;
	FloatRange               m_meleeDamage                            = FloatRange( 1.0f, 2.0f );
	std::string              m_meleeActor;
	float                    m_meleeDelay                             = 1.0f;
	float                    m_meleeRange                             = 0.5f;
	bool                     m_attractAI                              = false;
		                     
	float                    m_health                                 = -1.0f;
	Faction                  m_faction                                = Faction::NEUTRAL;
	float                    m_corpseLifetime                         = 0.0f;
	bool                     m_dieOnCollide                           = false;
	FloatRange               m_damageOnCollide                        = FloatRange( 0.0f, 0.0f );
	float                    m_impulseOnCollide                       = 0.0f;
				             
	bool                     m_visible                                = true;

	SoundCollection          m_sounds;

	static void ClearDefinitions();
	static void InitializeDefinitions( const char* path );
	static const ActorDefinition* GetByName( const std::string& name );
	static std::vector<ActorDefinition> s_definitions;
};

