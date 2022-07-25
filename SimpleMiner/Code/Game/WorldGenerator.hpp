
class Chunk;

class WorldGenerator
{
public:
	WorldGenerator() {};
	virtual ~WorldGenerator() {};

	virtual void GenerateChunk(Chunk* chunk) = 0;

public:
	unsigned int m_seed = 0;
};

class PlainWorldGenerator : public WorldGenerator
{
public:
	void GenerateChunk(Chunk* chunk) override;
};

class PerlinWorldGenerator : public WorldGenerator
{
public:
	void GenerateChunk(Chunk* chunk) override;
};

class OverworldWorldGenerator : public WorldGenerator
{
public:
	OverworldWorldGenerator();

	void GenerateChunk(Chunk* chunk) override;

};

class SkyBlockWorldGenerator : public WorldGenerator
{
public:
	SkyBlockWorldGenerator();

	void GenerateChunk(Chunk* chunk) override;

};


#include "Engine/Math/IntVec3.hpp"
#include "Game/BlockDef.hpp"
#include "Game/Chunk.hpp"
#include <vector>

class BlockTemplate
{
public:
	struct Entry
	{
		Entry();
		Entry(int x, int y, int z, BlockId block);
		Entry(const IntVec3& coords, BlockId block);

		IntVec3 m_offset;
		BlockId m_blockId;
	};

public:
	void PlaceOn(Chunk* chunk, const LocalCoords& coords) const;

public:
	std::vector<Entry> m_blocks;
};

