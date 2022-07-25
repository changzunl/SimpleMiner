#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <string>
#include <vector>

namespace tinyxml2
{
	class XMLElement;
}
typedef tinyxml2::XMLElement XmlElement;

class Shader;
class SpriteSheet;

//------------------------------------------------------------------------------------------------
class BlockMaterialAtlas
{
public:
	BlockMaterialAtlas();
	~BlockMaterialAtlas();
	bool LoadFromXmlElement(const XmlElement& element);

public:
	IntVec2 m_gridLayout = IntVec2::ZERO;
	SpriteSheet* m_spriteSheet = nullptr;
	Shader* m_shader = nullptr;
};

//------------------------------------------------------------------------------------------------
class BlockMaterialDef
{
public:
	bool LoadFromXmlElement(const XmlElement& element, const BlockMaterialAtlas& atlas);

public:
	std::string m_name;
	bool m_visible = true;
	AABB2 m_uv = AABB2::ZERO_TO_ONE;
};

