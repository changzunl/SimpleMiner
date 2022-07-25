#pragma once

#include "Game/GameCommon.hpp"

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"

//------------------------------------------------------------------------------------------------
typedef unsigned char BlockId;
typedef unsigned char LightLevel;

class BlockDef;

constexpr IntVec3 DIRECTION_N( 1,  0,  0);
constexpr IntVec3 DIRECTION_S(-1,  0,  0);
constexpr IntVec3 DIRECTION_W( 0,  1,  0);
constexpr IntVec3 DIRECTION_E( 0, -1,  0);
constexpr IntVec3 DIRECTION_U( 0,  0,  1);
constexpr IntVec3 DIRECTION_D( 0,  0, -1);

constexpr int BLOCK_LIGHT_BITS_INDOOR         = 0x0F;
constexpr int BLOCK_LIGHT_BITS_OUTDOOR        = 0xF0;
constexpr int BLOCK_LIGHT_BITSHIFT_INDOOR     = 0;
constexpr int BLOCK_LIGHT_BITSHIFT_OUTDOOR    = 4;
constexpr int BLOCK_FLAG_BIT_IS_SKY           = 1 << 0;
constexpr int BLOCK_FLAG_BIT_INVALID          = 1 << 1;
constexpr int BLOCK_FLAG_BIT_LIGHT_DIRTY      = 1 << 2;
constexpr int BLOCK_FLAG_BIT_IS_SOLID         = 1 << 3;
constexpr int BLOCK_FLAG_BIT_IS_OPAQUE        = 1 << 4;

enum BlockFace
{
	BLOCK_FACE_NORTH,
	BLOCK_FACE_SOUTH,
	BLOCK_FACE_WEST,
	BLOCK_FACE_EAST,
	BLOCK_FACE_UP,
	BLOCK_FACE_DOWN,
	BLOCK_FACE_SIZE,
};

constexpr BlockFace CHUNK_NEIGHBORS[4] = { BLOCK_FACE_NORTH, BLOCK_FACE_SOUTH, BLOCK_FACE_WEST, BLOCK_FACE_EAST };
constexpr BlockFace BLOCK_NEIGHBORS[6] = { BLOCK_FACE_NORTH, BLOCK_FACE_SOUTH, BLOCK_FACE_WEST, BLOCK_FACE_EAST, BLOCK_FACE_UP, BLOCK_FACE_DOWN };


//------------------------------------------------------------------------------------------------
struct Block
{
public:
	static Block INVALID;
	static Block GetInvalidBlock();
	static inline IntVec3 GetOffsetByFace(BlockFace face);
	static inline IntVec2 GetOffset2ByFace(BlockFace face);
	static inline unsigned char NormalizeLightInfluence(unsigned char influence); // 0 ~ 15 -> 0 ~ 255

public:
	Block();
	Block(const Block& copyFrom);
	explicit Block(BlockId blockId);
	explicit Block(const BlockDef* definition);

	void InitializeFlags();

	const BlockDef* GetBlockDef() const;
	BlockId GetBlockId() const;
	void SetBlockId(BlockId blockId);
	void SetBlockId(const BlockDef* blockDef);

	LightLevel GetSkyLight() const;
	LightLevel GetGlowLight() const;
	LightLevel GetIndoorLightInfluence() const;
	LightLevel GetOutdoorLightInfluence() const;
	unsigned char GetIndoorLightInfluenceNormalized() const;
	unsigned char GetOutdoorLightInfluenceNormalized() const;

	void SetIndoorLightInfluence(LightLevel intensity);
	void SetOutdoorLightInfluence(LightLevel intensity);

	inline bool GetFlag(int bit) const;
	inline void SetFlag(int bit, bool val);
	
	inline bool IsValid() const;
	inline bool IsOpaque() const;
	inline bool IsSolid() const;
	inline bool IsSky() const;
	inline bool IsLightDirty() const;

	inline void SetSky(bool val = true);
	inline void SetLightDirty(bool val = true);

private:
	unsigned char m_blockId = 0;
	unsigned char m_blockMeta = 0;
	unsigned char m_lightBits = 0;
	unsigned char m_flagBits = 0;
};


//------------------------------------------------------------------------------------------------
IntVec3 Block::GetOffsetByFace(BlockFace face)
{
	constexpr IntVec3 directions[BLOCK_FACE_SIZE] = { DIRECTION_N, DIRECTION_S, DIRECTION_W, DIRECTION_E, DIRECTION_U, DIRECTION_D };
	return directions[face];
}

IntVec2 Block::GetOffset2ByFace(BlockFace face)
{
	IntVec3 val = GetOffsetByFace(face);
	return IntVec2(val.x, val.y);
}

unsigned char Block::NormalizeLightInfluence(unsigned char influence)
{
	return (influence << 4) + influence; // accelerated?: influence * 17 
}

bool Block::GetFlag(int bit) const
{
	return (m_flagBits & bit) == bit;
}

void Block::SetFlag(int bit, bool val)
{
	if (val)
		m_flagBits |= bit;
	else
		m_flagBits &= ~bit;
}

bool Block::IsValid() const 
{
	return !GetFlag(BLOCK_FLAG_BIT_INVALID); 
}

bool Block::IsOpaque() const 
{
	return GetFlag(BLOCK_FLAG_BIT_IS_OPAQUE); 
}

bool Block::IsSolid() const 
{
	return GetFlag(BLOCK_FLAG_BIT_IS_SOLID); 
}

bool Block::IsSky() const 
{
	return GetFlag(BLOCK_FLAG_BIT_IS_SKY); 
}

bool Block::IsLightDirty() const 
{
	return GetFlag(BLOCK_FLAG_BIT_LIGHT_DIRTY); 
}

void Block::SetSky(bool val/* = true*/) 
{
	SetFlag(BLOCK_FLAG_BIT_IS_SKY, val); 
}

void Block::SetLightDirty(bool val/* = true*/) 
{
	SetFlag(BLOCK_FLAG_BIT_LIGHT_DIRTY, val); 
}

