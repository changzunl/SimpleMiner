#pragma once

#include "Game/Chunk.hpp"

class ChunkProvider;

class BlockIterator
{
private:
	Chunk*         m_chunk = nullptr;
	int            m_blockIndex = 0;

public:
	inline BlockIterator();
	inline BlockIterator(Chunk* chunk, int blockIndex);
	inline BlockIterator(Chunk* chunk, const LocalCoords& localCoords);
	BlockIterator(const ChunkProvider* chunkProvider, const WorldCoords& worldCoords);

	inline bool IsValid() const;
	inline Chunk* GetChunk() const;
	inline Block* GetBlock() const;
	inline LocalCoords GetLocalCoords() const;
	inline WorldCoords GetWorldCoords() const;

	BlockIterator GetBlockNeighbor(BlockFace face) const;
	BlockIterator GetBlockNeighborNorth() const;
	BlockIterator GetBlockNeighborSouth() const;
	BlockIterator GetBlockNeighborWest() const;
	BlockIterator GetBlockNeighborEast() const;
	BlockIterator GetBlockNeighborUp() const;
	BlockIterator GetBlockNeighborDown() const;
	Chunk* GetChunkNeighbor(BlockFace face) const;


	BlockIterator operator+(const WorldCoords& coords) const;
	void operator+=(const WorldCoords& coords);
	BlockIterator& operator++();
	BlockIterator& operator--();
	BlockIterator& operator++(int);
	BlockIterator& operator--(int);
};


BlockIterator::BlockIterator()
{

}

BlockIterator::BlockIterator(Chunk* chunk, int blockIndex)
	: m_chunk(chunk)
	, m_blockIndex(blockIndex)
{

}

BlockIterator::BlockIterator(Chunk* chunk, const LocalCoords& localCoords)
	: m_chunk(chunk)
	, m_blockIndex(Chunk::GetIndex(localCoords))
{

}

bool BlockIterator::IsValid() const
{
	return m_chunk;
}

Chunk* BlockIterator::GetChunk() const
{
	return m_chunk;
}

Block* BlockIterator::GetBlock() const
{
	if (!IsValid())
		return &Block::INVALID;
	LocalCoords coords = GetLocalCoords();
	if (coords.z < 0 || coords.z > CHUNK_MAX_Z)
		return &Block::INVALID;
	return &m_chunk->GetBlock(coords);
}

LocalCoords BlockIterator::GetLocalCoords() const
{
	return Chunk::GetLocalCoords(m_blockIndex);
}

WorldCoords BlockIterator::GetWorldCoords() const
{
	return Chunk::GetWorldCoords(m_chunk->m_chunkCoords, GetLocalCoords());
}

