#include "BlockMaterialDef.hpp"

#include "GameCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"


bool BlockMaterialDef::LoadFromXmlElement(const XmlElement& element, const BlockMaterialAtlas& atlas)
{
	m_name = ParseXmlAttribute(element, "name", m_name);
	m_visible = ParseXmlAttribute(element, "visible", m_visible);
	ASSERT_OR_DIE(m_name.size() != 0, "Name not assigned");

	if (m_visible)
	{
		IntVec2 cell = ParseXmlAttribute(element, "cell", IntVec2(0, 0));
		m_uv = atlas.m_spriteSheet->GetSpriteUVs(cell.x + cell.y * atlas.m_gridLayout.x);
	}

	return true;
}

BlockMaterialAtlas::BlockMaterialAtlas()
{
}

BlockMaterialAtlas::~BlockMaterialAtlas()
{
	delete m_spriteSheet;
}

bool BlockMaterialAtlas::LoadFromXmlElement(const XmlElement& element)
{
	std::string       shader    = ParseXmlAttribute(element, "shader", "");
	std::string       texture   = ParseXmlAttribute(element, "texture", "");
	IntVec2           cellCount = ParseXmlAttribute(element, "cellCount", IntVec2(8, 8));

	Texture* pTexture = g_theRenderer->CreateOrGetTextureFromFile(texture.c_str());
	ASSERT_OR_DIE(pTexture, "Texture not present");

	m_gridLayout = cellCount;
	m_spriteSheet = new SpriteSheet(*pTexture, cellCount);
	m_shader = g_theRenderer->CreateOrGetShader(shader.c_str());
	ASSERT_OR_DIE(m_shader, "Shader not present");

	return true;
}
