#include "ActorDefinition.hpp"

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/XmlUtils.hpp"

extern const char* g_factionNames[];

std::vector<ActorDefinition> ActorDefinition::s_definitions;

bool ActorDefinition::LoadFromXmlElement(const XmlElement& element)
{
	m_name           = ParseXmlAttribute(element, "name", m_name);
	m_health         = ParseXmlAttribute(element, "health", m_health);
	m_canBePossessed = ParseXmlAttribute(element, "canBePossessed", m_canBePossessed);
	m_corpseLifetime = ParseXmlAttribute(element, "corpseLifetime", m_corpseLifetime);
	m_lifetime       = ParseXmlAttribute(element, "lifetime", m_lifetime);
	m_attractAI      = ParseXmlAttribute(element, "attractAI", m_attractAI);
	m_eyeHeight      = ParseXmlAttribute(element, "eyeHeight", m_eyeHeight);

    std::string factionName = ParseXmlAttribute(element, "faction", g_factionNames[m_faction]);
	for (int faction = 0; faction < Faction::COUNT; faction++)
	{
		if (factionName == g_factionNames[faction])
		{
 			m_faction = (Faction)faction;
			break;
		}
	}

	const XmlElement* collision = element.FirstChildElement("Collision");
	if (collision)
	{
		m_physicsRadius = ParseXmlAttribute(*collision, "radius", m_physicsRadius);
		m_physicsHeight = ParseXmlAttribute(*collision, "height", m_physicsHeight);
		m_collidesWithWorld = ParseXmlAttribute(*collision, "collidesWithWorld", m_collidesWithWorld);
		m_collidesWithActors = ParseXmlAttribute(*collision, "collidesWithActors", m_collidesWithActors);
		m_damageOnCollide = ParseXmlAttribute(*collision, "damageOnCollide", m_damageOnCollide);
		m_impulseOnCollide = ParseXmlAttribute(*collision, "impulseOnCollide", m_impulseOnCollide);
		m_dieOnCollide = ParseXmlAttribute(*collision, "dieOnCollide", m_dieOnCollide);
	}

	const XmlElement* physics = element.FirstChildElement("Physics");
	if (physics)
	{
		m_simulated = ParseXmlAttribute(*physics, "simulated", m_simulated);
		m_walkSpeed = ParseXmlAttribute(*physics, "walkSpeed", m_walkSpeed);
		m_runSpeed = ParseXmlAttribute(*physics, "runSpeed", m_runSpeed);
		m_turnSpeed = ParseXmlAttribute(*physics, "turnSpeed", m_turnSpeed);
		m_flying = ParseXmlAttribute(*physics, "flying", m_flying);
		m_drag = ParseXmlAttribute(*physics, "drag", m_drag);
	}

	const XmlElement* camera = element.FirstChildElement("Camera");
	if (camera)
	{
		m_cameraFOVDegrees = ParseXmlAttribute(*camera, "cameraFOV", m_cameraFOVDegrees);
	}

	const XmlElement* ai = element.FirstChildElement("AI");
	if (ai)
	{
		m_aiEnabled   = true;
		m_aiType      = ParseXmlAttribute(*ai, "aiType", m_aiType);
		m_sightRadius = ParseXmlAttribute(*ai, "sightRadius", m_sightRadius);
		m_sightAngle  = ParseXmlAttribute(*ai, "sightAngle", m_sightAngle);
		m_meleeDamage = ParseXmlAttribute(*ai, "meleeDamage", m_meleeDamage);
		m_meleeDelay  = ParseXmlAttribute(*ai, "meleeDelay", m_meleeDelay);
		m_meleeRange  = ParseXmlAttribute(*ai, "meleeRange", m_meleeRange);
		m_meleeActor  = ParseXmlAttribute(*ai, "meleeActor", m_meleeActor);
	}

	const XmlElement* appearance = element.FirstChildElement("Appearance");
	if (appearance)
	{
		m_visible = ParseXmlAttribute(*appearance, "visible", true);
	}
	else
	{
		m_visible = false;
	}

	const XmlElement* sounds = element.FirstChildElement("Sounds");
	if (sounds)
	{
		m_sounds.LoadFromXmlElement(*sounds);
	}

	return true;
}

void ActorDefinition::ClearDefinitions()
{
	s_definitions.clear();
}

void ActorDefinition::InitializeDefinitions(const char* path)
{
	XmlDocument document;
	document.LoadFile(path);

	const XmlElement* pElement = document.RootElement()->FirstChildElement("ActorDefinition");
	while (pElement)
	{
		s_definitions.emplace_back();
		bool load = (*(s_definitions.end() - 1)).LoadFromXmlElement(*pElement);
		if (!load)
		{
			DebuggerPrintf(Stringf("Failed to load tile definition: %s", pElement->GetText()).c_str());
			s_definitions.pop_back();
		}

		pElement = pElement->NextSiblingElement("ActorDefinition");
	}
}

const ActorDefinition* ActorDefinition::GetByName(const std::string& name)
{
	for (const auto& def : s_definitions)
	{
		if (def.m_name == name)
			return &def;
	}
	ERROR_AND_DIE((std::string("Actor definition not found: ") + name).c_str());
}

