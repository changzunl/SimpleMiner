#pragma once

#include "Game/GameCommon.hpp"
#include "Game/Block.hpp"

#include <string>
#include <vector>

namespace tinyxml2
{
	class XMLElement;
}
typedef tinyxml2::XMLElement XmlElement;
typedef unsigned char BlockId;
typedef unsigned char LightLevel;

//------------------------------------------------------------------------------------------------
class BlockMaterialDef;

//------------------------------------------------------------------------------------------------
class BlockDef
{
public:
	bool LoadFromXmlElement( const XmlElement& element );

	const BlockMaterialDef* GetBlockMaterial(BlockFace face) const;

public:
	std::string m_name;
	BlockId m_blockId = 0;
	bool m_solid = false;
	bool m_opaque = false;
	LightLevel m_lightLevel = 0;

	const BlockMaterialDef* m_matDef[BLOCK_FACE_SIZE] = {};
};

class Blocks
{
public:
	static unsigned char BLOCK_AIR;
	static unsigned char BLOCK_BEDROCK;
	static unsigned char BLOCK_STONE;
	static unsigned char BLOCK_DIRT;
	static unsigned char BLOCK_GRASS;
	static unsigned char BLOCK_COAL_ORE;
	static unsigned char BLOCK_IRON_ORE;
	static unsigned char BLOCK_GOLD_ORE;
	static unsigned char BLOCK_DIAMOND_ORE;
	static unsigned char BLOCK_WATER;
	static unsigned char BLOCK_COBBLESTONE;
	static unsigned char BLOCK_RED_BRICK;
	static unsigned char BLOCK_LOG;
	static unsigned char BLOCK_GLOWSTONE;
	static unsigned char BLOCK_SAND;
	static unsigned char BLOCK_LEAVES;
	static unsigned char BLOCK_ICE;

	static void Initialize();
};

