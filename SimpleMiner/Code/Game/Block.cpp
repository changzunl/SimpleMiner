#include "Block.hpp"

#include "Game/BlockDef.hpp"
#include "Game/BlockSetDefinition.hpp"


Block Block::INVALID = Block::GetInvalidBlock();

Block Block::GetInvalidBlock()
{
	Block block;
	block.SetFlag(BLOCK_FLAG_BIT_INVALID, true);
	return block;
}

Block::Block(const BlockDef* definition)
{
	SetBlockId(definition ? definition->m_blockId : Blocks::BLOCK_AIR);
}

Block::Block(const Block& copyFrom)
	: m_blockId(copyFrom.m_blockId)
	, m_lightBits(copyFrom.m_lightBits)
	, m_flagBits(copyFrom.m_flagBits)
{
}

Block::Block(BlockId blockId)
{
	SetBlockId(blockId);
}

Block::Block()
{
}

void Block::InitializeFlags()
{
	if (IsValid())
	{
		SetFlag(BLOCK_FLAG_BIT_IS_SOLID, GetBlockDef()->m_solid);
		SetFlag(BLOCK_FLAG_BIT_IS_OPAQUE, GetBlockDef()->m_opaque);
	}
}

const BlockDef* Block::GetBlockDef() const
{
	return BlockSetDefinition::GetDefinition()->GetBlockDefById(IsValid() ? m_blockId : Blocks::BLOCK_AIR);
}

BlockId Block::GetBlockId() const
{
	return m_blockId;
}

void Block::SetBlockId(BlockId blockId)
{
	m_blockId = blockId;
	InitializeFlags();
}

void Block::SetBlockId(const BlockDef* blockDef)
{
	SetBlockId(blockDef->m_blockId);
}

LightLevel Block::GetSkyLight() const
{
	return IsSky() ? 15 : 0;
}

LightLevel Block::GetGlowLight() const
{
	return GetBlockDef()->m_lightLevel;
}

LightLevel Block::GetIndoorLightInfluence() const
{
	return (m_lightBits & BLOCK_LIGHT_BITS_INDOOR) >> BLOCK_LIGHT_BITSHIFT_INDOOR;
}

LightLevel Block::GetOutdoorLightInfluence() const
{
	return (m_lightBits & BLOCK_LIGHT_BITS_OUTDOOR) >> BLOCK_LIGHT_BITSHIFT_OUTDOOR;
}

unsigned char Block::GetIndoorLightInfluenceNormalized() const
{
	return NormalizeLightInfluence(GetIndoorLightInfluence());
}

unsigned char Block::GetOutdoorLightInfluenceNormalized() const
{
	return NormalizeLightInfluence(GetOutdoorLightInfluence());
}

void Block::SetIndoorLightInfluence(LightLevel intensity)
{
	m_lightBits &= ~BLOCK_LIGHT_BITS_INDOOR;
	m_lightBits |= BLOCK_LIGHT_BITS_INDOOR & (intensity << BLOCK_LIGHT_BITSHIFT_INDOOR);
}

void Block::SetOutdoorLightInfluence(LightLevel intensity)
{
	m_lightBits &= ~BLOCK_LIGHT_BITS_OUTDOOR;
	m_lightBits |= BLOCK_LIGHT_BITS_OUTDOOR & (intensity << BLOCK_LIGHT_BITSHIFT_OUTDOOR);
}
