#pragma once

#include "Game/GameCommon.hpp"
#include "Game/Block.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/VertexFormat.hpp"

#include <atomic>

//------------------------------------------------------------------------------------------------
typedef unsigned char BlockId;
typedef IntVec2 ChunkCoords;
typedef IntVec3 LocalCoords;
typedef IntVec3 WorldCoords;

class World;
class VertexBuffer;
class IndexBuffer;
class ByteBuffer;

struct BlockState
{
	WorldCoords m_coords;
	LocalCoords m_localCoords;
	Block m_block;
};

enum class ChunkState
{
	UNLOAD,           // chunk is not in memory 
	QUEUED,           // chunk is queued to generate 
	GENERATING,		  // chunk is being generated
	GENERATED,		  // chunk is generated
	LOADED,			  // chunk is loaded and active
};

//------------------------------------------------------------------------------------------------
class Chunk
{
public:
	static inline ChunkCoords            GetChunkCoords(const WorldCoords& worldCoords);
	static inline LocalCoords            GetLocalCoords(const WorldCoords& worldCoords);
	static inline ChunkCoords            GetChunkCoords(const Vec3& worldPosition);
	static inline LocalCoords            GetLocalCoords(const Vec3& worldPosition);
	static inline WorldCoords            GetWorldCoords(const Vec3& worldPosition);
	static inline WorldCoords            GetOriginInWorld(const ChunkCoords& chunkCoords);
	static inline WorldCoords            GetWorldCoords(const ChunkCoords& chunkCoords, const LocalCoords& localCoords);
	static inline int                    GetIndex(const LocalCoords& localCoords);
	static inline LocalCoords            GetLocalCoords(int blockIndex);

	Chunk(World* world, const ChunkCoords& chunkCoords);
	~Chunk();

	void Update();
	void UpdateRandomTick(size_t rndSize, const float* rndSource, size_t offset);
	void Render(int pass) const;

	void RebuildMesh();
	void MarkDirty();
	void PopulateSkyLight();

	WorldCoords     GetChunkOrigin() const;
	WorldCoords     GetWorldCoords(const LocalCoords& localCoords) const;
	BlockId         GetBlockId(const LocalCoords& localCoords) const;
	const Block&    GetBlock(const LocalCoords& localCoords) const;
	Block&          GetBlock(const LocalCoords& localCoords);
	void            SetBlockId(const LocalCoords& localCoords, BlockId block);
	const Block&    FindBlockOnFace(const LocalCoords& coords, BlockFace face) const;

	void            OnNeighborLoad(Chunk& neighbor);
	void            OnNeighborUnload(const Chunk& neighbor);
	void            OnNeighborBlockUpdate(const Chunk& neighbor, const LocalCoords& coords);

	void            WriteBytes(ByteBuffer* buffer) const;
	void            ReadBytes(ByteBuffer* buffer);

private:
	void RebuildOpaqueMesh();
	void RebuildTranslucentMesh();

public:
	World* m_world;
	ChunkCoords m_chunkCoords;
	std::atomic<ChunkState> m_state = ChunkState::UNLOAD;
	Chunk* m_neighbors[4] = {}; // NORTH(+X), SOUTH(-X), WEST(+Y), EAST(-Y)
	Block* m_blockArray = nullptr;

	bool m_meshDirty = true;
	bool m_blocksDirty = false;

private:
	VertexBufferBuilder m_opaqueMesh;
	VertexBuffer* m_opaqueBuffer = nullptr;
	IndexBuffer* m_opaqueBufferIdx = nullptr;
	VertexBufferBuilder m_fluidMesh;
	VertexBuffer* m_fluidBuffer = nullptr;
	IndexBuffer* m_fluidBufferIdx = nullptr;
};


ChunkCoords Chunk::GetChunkCoords(const WorldCoords& worldCoords)
{
	ChunkCoords chunkCoords;
	chunkCoords.x = worldCoords.x >> CHUNK_SIZE_BITWIDTH_XY;
	chunkCoords.y = worldCoords.y >> CHUNK_SIZE_BITWIDTH_XY;
	return chunkCoords;
}

ChunkCoords Chunk::GetChunkCoords(const Vec3& worldPosition)
{
	ChunkCoords chunkCoords;
	chunkCoords.x = int(floorf(worldPosition.x)) >> CHUNK_SIZE_BITWIDTH_XY;
	chunkCoords.y = int(floorf(worldPosition.y)) >> CHUNK_SIZE_BITWIDTH_XY;
	return chunkCoords;
}

LocalCoords Chunk::GetLocalCoords(const WorldCoords& worldCoords)
{
	LocalCoords localCoords;
	localCoords.x = worldCoords.x & CHUNK_BLOCKMASK_X;
	localCoords.y = worldCoords.y & (CHUNK_BLOCKMASK_Y >> CHUNK_BITSHIFT_Y);
	localCoords.z = worldCoords.z;
	return localCoords;
}

LocalCoords Chunk::GetLocalCoords(const Vec3& worldPosition)
{
	return GetLocalCoords(GetWorldCoords(worldPosition));
}

LocalCoords Chunk::GetLocalCoords(int blockIndex)
{
	LocalCoords coords = IntVec3();
	coords.x = (blockIndex & CHUNK_BLOCKMASK_X) >> CHUNK_BITSHIFT_X;
	coords.y = (blockIndex & CHUNK_BLOCKMASK_Y) >> CHUNK_BITSHIFT_Y;
	coords.z = blockIndex >> CHUNK_BITSHIFT_Z;

	return coords;
}

WorldCoords Chunk::GetWorldCoords(const Vec3& worldPosition)
{
	return IntVec3(int(floorf(worldPosition.x)), int(floorf(worldPosition.y)), int(floorf(worldPosition.z)));
}

WorldCoords Chunk::GetWorldCoords(const ChunkCoords& chunkCoords, const LocalCoords& localCoords)
{
	return Chunk::GetOriginInWorld(chunkCoords) + localCoords;
}

WorldCoords Chunk::GetOriginInWorld(const ChunkCoords& chunkCoords)
{
	WorldCoords origin = IntVec3(chunkCoords);
	origin.x <<= CHUNK_SIZE_BITWIDTH_XY;
	origin.y <<= CHUNK_SIZE_BITWIDTH_XY;
	return origin;
}

int Chunk::GetIndex(const LocalCoords& localCoords)
{
	int i = 0;
	i |= localCoords.z << (CHUNK_SIZE_BITWIDTH_XY + CHUNK_SIZE_BITWIDTH_XY);
	i |= (localCoords.y & CHUNK_MAX_Y) << CHUNK_SIZE_BITWIDTH_XY;
	i |= (localCoords.x & CHUNK_MAX_X);

	return i;
}

