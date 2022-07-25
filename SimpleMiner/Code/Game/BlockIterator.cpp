#include "Game/BlockIterator.hpp"

#include "Game/ChunkProvider.hpp"
#include "Game/World.hpp"


BlockIterator::BlockIterator(const ChunkProvider* chunkProvider, const WorldCoords& worldCoords)
	: m_chunk(chunkProvider->FindLoadedChunk(Chunk::GetChunkCoords(worldCoords)))
	, m_blockIndex(Chunk::GetIndex(Chunk::GetLocalCoords(worldCoords)))
{

}

Chunk* BlockIterator::GetChunkNeighbor(BlockFace face) const
{
	ChunkCoords coords = m_chunk->m_chunkCoords + Block::GetOffset2ByFace(face);
	return m_chunk->m_world->GetChunkManager()->FindLoadedChunk(coords);
}

BlockIterator BlockIterator::GetBlockNeighborNorth() const
{
	if (!IsValid())
		return *this;

	BlockIterator ite = *this;
	int x = (m_blockIndex & CHUNK_BLOCKMASK_X) >> CHUNK_BITSHIFT_X;
	if (x == CHUNK_MAX_X)
	{
		ite.m_chunk = m_chunk->m_neighbors[BLOCK_FACE_NORTH];
		ite.m_blockIndex -= CHUNK_MAX_X;
		return ite;
	}
	else
	{
		ite.m_blockIndex++;
		return ite;
	}
}

BlockIterator BlockIterator::GetBlockNeighborSouth() const
{
	if (!IsValid())
		return *this;

	BlockIterator ite = *this;
	int x = (m_blockIndex & CHUNK_BLOCKMASK_X) >> CHUNK_BITSHIFT_X;
	if (x == 0)
	{
		ite.m_chunk = m_chunk->m_neighbors[BLOCK_FACE_SOUTH];
		ite.m_blockIndex += CHUNK_MAX_X;
		return ite;
	}
	else
	{
		ite.m_blockIndex--;
		return ite;
	}
}

BlockIterator BlockIterator::GetBlockNeighborWest() const
{
	if (!IsValid())
		return *this;

	BlockIterator ite = *this;
	int y = (m_blockIndex & CHUNK_BLOCKMASK_Y) >> CHUNK_BITSHIFT_Y;
	if (y == CHUNK_MAX_Y)
	{
		ite.m_chunk = m_chunk->m_neighbors[BLOCK_FACE_WEST];
		ite.m_blockIndex -= CHUNK_MAX_Y * CHUNK_SIZE_XY;
		return ite;
	}
	else
	{
		ite.m_blockIndex += CHUNK_SIZE_XY;
		return ite;
	}
}

BlockIterator BlockIterator::GetBlockNeighborEast() const
{
	if (!IsValid())
		return *this;

	BlockIterator ite = *this;
	int y = (m_blockIndex & CHUNK_BLOCKMASK_Y) >> CHUNK_BITSHIFT_Y;
	if (y == 0)
	{
		ite.m_chunk = m_chunk->m_neighbors[BLOCK_FACE_EAST];
		ite.m_blockIndex += CHUNK_MAX_Y * CHUNK_SIZE_XY;
		return ite;
	}
	else
	{
		ite.m_blockIndex -= CHUNK_SIZE_XY;
		return ite;
	}
}

BlockIterator BlockIterator::GetBlockNeighborUp() const
{
	if (!IsValid())
		return *this;

	BlockIterator ite = *this;
	ite.m_blockIndex += CHUNK_SIZE_XY * CHUNK_SIZE_XY;
	return ite;
}

BlockIterator BlockIterator::GetBlockNeighborDown() const
{
	if (!IsValid())
		return *this;

	BlockIterator ite = *this;
	// int z = m_blockIndex >> CHUNK_BITSHIFT_Z;
	ite.m_blockIndex -= CHUNK_SIZE_XY * CHUNK_SIZE_XY;
	return ite;
}

BlockIterator BlockIterator::GetBlockNeighbor(BlockFace face) const
{
	if (!IsValid())
		return *this;

// 	IntVec3 localCoords = Chunk::GetLocalCoords(m_blockIndex);
// 	IntVec3 localCoordsNew = localCoords + Block::GetOffsetByFace(face);
// 	if (Chunk::GetChunkCoords(localCoords) == Chunk::GetChunkCoords(localCoordsNew))
// 	{
// 		return BlockIterator(m_chunk, GetLocalCoords() + Block::GetOffsetByFace(face));
// 	}
// 	WorldCoords wCoords = GetWorldCoords();
// 	WorldCoords newWCoords = wCoords + Block::GetOffsetByFace(face);
// 
// 	return BlockIterator(m_chunk->m_neighbors[face], Chunk::GetLocalCoords(newWCoords));

	switch (face)
	{
	case BLOCK_FACE_NORTH:             return GetBlockNeighborNorth();
	case BLOCK_FACE_SOUTH:             return GetBlockNeighborSouth();
	case BLOCK_FACE_WEST:              return GetBlockNeighborWest();
	case BLOCK_FACE_EAST:              return GetBlockNeighborEast();
	case BLOCK_FACE_UP:                return GetBlockNeighborUp();
	case BLOCK_FACE_DOWN:              return GetBlockNeighborDown();
	default:                           return *this;
	}
}

BlockIterator BlockIterator::operator+(const WorldCoords& offset) const
{
	if (!IsValid())
		return *this;

	if (offset.GetTaxicabLength() == 1)
	{
		if (offset.x == 1)
			return GetBlockNeighbor(BLOCK_FACE_NORTH);
		if (offset.x == -1)
			return GetBlockNeighbor(BLOCK_FACE_SOUTH);
		if (offset.y == 1)
			return GetBlockNeighbor(BLOCK_FACE_WEST);
		if (offset.y == -1)
			return GetBlockNeighbor(BLOCK_FACE_EAST);
		if (offset.z == 1)
			return GetBlockNeighbor(BLOCK_FACE_UP);
		if (offset.z == -1)
			return GetBlockNeighbor(BLOCK_FACE_DOWN);
	}

	WorldCoords wCoords = GetWorldCoords();
	WorldCoords newWCoords = wCoords + offset;
	ChunkCoords cCoords = Chunk::GetChunkCoords(wCoords);
	ChunkCoords newCCoords = Chunk::GetChunkCoords(newWCoords);
	if (newCCoords == cCoords)
		return BlockIterator(m_chunk, Chunk::GetLocalCoords(newWCoords));
	Chunk* pChunk = m_chunk;
	while (newCCoords.x > cCoords.x)
	{
		cCoords.x++;
		pChunk = pChunk->m_neighbors[BLOCK_FACE_NORTH];
	}
	while (newCCoords.x < cCoords.x)
	{
		cCoords.x--;
		pChunk = pChunk->m_neighbors[BLOCK_FACE_SOUTH];
	}
	while (newCCoords.y > cCoords.y)
	{
		cCoords.y++;
		pChunk = pChunk->m_neighbors[BLOCK_FACE_WEST];
	}
	while (newCCoords.y < cCoords.y)
	{
		cCoords.y--;
		pChunk = pChunk->m_neighbors[BLOCK_FACE_EAST];
	}

	return BlockIterator(pChunk, Chunk::GetLocalCoords(newWCoords));
}

void BlockIterator::operator+=(const WorldCoords& coords)
{
	*this = operator+(coords);
}

BlockIterator& BlockIterator::operator++()
{
	m_blockIndex++;
	return *this;
}

BlockIterator& BlockIterator::operator--()
{
	m_blockIndex--;
	return *this;
}

BlockIterator& BlockIterator::operator++(int)
{
	return ++(*this);
}

BlockIterator& BlockIterator::operator--(int)
{
	return --(*this);
}
