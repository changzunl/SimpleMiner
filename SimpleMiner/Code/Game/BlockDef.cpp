#include "BlockDef.hpp"

#include "Game/BlockMaterialDef.hpp"
#include "Game/BlockSetDefinition.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/XmlUtils.hpp"

enum BlockMaterialType
{
	SINGLE,
	TOP_SIDE_BOTTOM,
	SEPERATE,
};

bool BlockDef::LoadFromXmlElement(const XmlElement& element)
{
	m_name = ParseXmlAttribute(element, "name", m_name);
	ASSERT_OR_DIE(m_name.size() != 0, "Name not assigned");
	m_solid = ParseXmlAttribute(element, "solid", m_solid);
	m_opaque = ParseXmlAttribute(element, "opaque", m_opaque);
	m_lightLevel = (unsigned char)ParseXmlAttribute(element, "light", (int)m_lightLevel);
	std::string type = ParseXmlAttribute(element, "materialType", "single");

	if (_stricmp(type.c_str(), "single") == 0)
	{
		std::string material = ParseXmlAttribute(element, "material", "");

		BlockMaterialDef* materialDef = BlockSetDefinition::GetDefinition()->GetBlockMatDefByName(material.c_str());
		for (int i = 0; i < BLOCK_FACE_SIZE; i++)
			m_matDef[i] = materialDef;
	}
	else if (_stricmp(type.c_str(), "seperate3") == 0)
	{
		std::string material1 = ParseXmlAttribute(element, "materialTop", "");
		std::string material2 = ParseXmlAttribute(element, "materialSide", "");
		std::string material3 = ParseXmlAttribute(element, "materialBottom", "");

		BlockMaterialDef* materialDef1 = BlockSetDefinition::GetDefinition()->GetBlockMatDefByName(material1.c_str());
		BlockMaterialDef* materialDef2 = BlockSetDefinition::GetDefinition()->GetBlockMatDefByName(material2.c_str());
		BlockMaterialDef* materialDef3 = BlockSetDefinition::GetDefinition()->GetBlockMatDefByName(material3.c_str());

		m_matDef[BLOCK_FACE_NORTH] = materialDef2;
		m_matDef[BLOCK_FACE_SOUTH] = materialDef2;
		m_matDef[BLOCK_FACE_EAST]  = materialDef2;
		m_matDef[BLOCK_FACE_WEST]  = materialDef2;
		m_matDef[BLOCK_FACE_UP]    = materialDef1;
		m_matDef[BLOCK_FACE_DOWN]  = materialDef3;
	}
	else if (_stricmp(type.c_str(), "seperate6") == 0)
	{
		ERROR_AND_DIE("Not implemented: seperate each face");
	}
	else
	{
		ERROR_AND_DIE(("Unknown material type: " + type).c_str());
	}

	return true;
}

const BlockMaterialDef* BlockDef::GetBlockMaterial(BlockFace face) const
{
	return m_matDef[face];
}

unsigned char Blocks::BLOCK_AIR = 0;
unsigned char Blocks::BLOCK_BEDROCK = 0;
unsigned char Blocks::BLOCK_STONE = 0;
unsigned char Blocks::BLOCK_DIRT = 0;
unsigned char Blocks::BLOCK_GRASS = 0;
unsigned char Blocks::BLOCK_COAL_ORE = 0;
unsigned char Blocks::BLOCK_IRON_ORE = 0;
unsigned char Blocks::BLOCK_GOLD_ORE = 0;
unsigned char Blocks::BLOCK_DIAMOND_ORE = 0;
unsigned char Blocks::BLOCK_WATER = 0;
unsigned char Blocks::BLOCK_COBBLESTONE = 0;
unsigned char Blocks::BLOCK_RED_BRICK = 0;
unsigned char Blocks::BLOCK_LOG = 0;
unsigned char Blocks::BLOCK_GLOWSTONE = 0;
unsigned char Blocks::BLOCK_SAND = 0;
unsigned char Blocks::BLOCK_LEAVES = 0;
unsigned char Blocks::BLOCK_ICE = 0;

void Blocks::Initialize()
{
	auto* def = BlockSetDefinition::GetDefinition();

	BLOCK_AIR            = def->GetBlockDefByName("Air")->m_blockId;
	BLOCK_BEDROCK        = def->GetBlockDefByName("Bedrock")->m_blockId;
	BLOCK_STONE          = def->GetBlockDefByName("Stone")->m_blockId;
	BLOCK_DIRT           = def->GetBlockDefByName("Dirt")->m_blockId;
	BLOCK_GRASS          = def->GetBlockDefByName("Grass")->m_blockId;
	BLOCK_COAL_ORE       = def->GetBlockDefByName("CoalOre")->m_blockId;
	BLOCK_IRON_ORE       = def->GetBlockDefByName("IronOre")->m_blockId;
	BLOCK_GOLD_ORE       = def->GetBlockDefByName("GoldOre")->m_blockId;
	BLOCK_DIAMOND_ORE    = def->GetBlockDefByName("DiamondOre")->m_blockId;
	BLOCK_WATER          = def->GetBlockDefByName("Water")->m_blockId;
	BLOCK_COBBLESTONE    = def->GetBlockDefByName("CobbleStone")->m_blockId;
	BLOCK_RED_BRICK      = def->GetBlockDefByName("RedBrick")->m_blockId;
	BLOCK_LOG            = def->GetBlockDefByName("Log")->m_blockId;
	BLOCK_GLOWSTONE      = def->GetBlockDefByName("GlowStone")->m_blockId;
	BLOCK_SAND           = def->GetBlockDefByName("Sand")->m_blockId;
	BLOCK_LEAVES         = def->GetBlockDefByName("Leaves")->m_blockId;
	BLOCK_ICE            = def->GetBlockDefByName("Ice")->m_blockId;

	((BlockDef*)def->GetBlockDefByName("GlowStone"))->m_lightLevel = 15;
}
