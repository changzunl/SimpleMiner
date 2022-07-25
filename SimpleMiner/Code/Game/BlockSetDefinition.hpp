#pragma once

#include "Game/GameCommon.hpp"
#include "Game/BlockDef.hpp"

#include "Engine/Core/Rgba8.hpp"

#include <string>
#include <vector>

namespace tinyxml2
{
	class XMLElement;
}
typedef tinyxml2::XMLElement XmlElement;

//------------------------------------------------------------------------------------------------
class BlockDef;
class BlockMaterialDef;
class BlockMaterialAtlas;


//------------------------------------------------------------------------------------------------
class BlockSetDefinition
{
public:
	bool LoadFromXmlElement( const XmlElement& element );

public:
	std::string m_name;

private:
	BlockMaterialAtlas* m_blockMatAtlas = nullptr;
	std::vector<BlockMaterialDef*> m_blockMatDefs;
	std::vector<BlockDef*> m_blockDefs;

public:
	const BlockMaterialAtlas* GetBlockMaterialAtlas() const;
	const BlockDef* GetBlockDefByName(const char* name) const;
	inline const BlockDef* GetBlockDefById(BlockId id) const;
	BlockMaterialDef* GetBlockMatDefByName(const char* name) const;

public:
	static void InitializeDefinition(const char* path);
	static inline const BlockSetDefinition* GetDefinition();

private:
	static BlockSetDefinition* s_definition;
};


// inline codes
const BlockDef* BlockSetDefinition::GetBlockDefById(BlockId id) const
{
	return (id < 0 || id >= m_blockDefs.size()) ? nullptr : m_blockDefs[id];
}

const BlockSetDefinition* BlockSetDefinition::GetDefinition()
{
	return s_definition;
}

