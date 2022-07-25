#include "Game/WorldGenerator.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/Curves.hpp"
#include "Game/Chunk.hpp"
#include "Game/BlockDef.hpp"
#include "ThirdParty/squirrel/SmoothNoise.hpp"

extern RandomNumberGenerator rng;

void PlainWorldGenerator::GenerateChunk(Chunk* chunk)
{
	LocalCoords coords = IntVec3::ZERO;
	int index = Chunk::GetIndex(coords);
	for (; coords.z < CHUNK_SIZE_Z; coords.z++)
		for (coords.y = 0; coords.y < CHUNK_SIZE_XY; coords.y++)
			for (coords.x = 0; coords.x < CHUNK_SIZE_XY; coords.x++)
			{
				chunk->m_blockArray[index].SetBlockId(Blocks::BLOCK_AIR);
				index += 1;
			}

	index = 0;
	for (coords.z = 0; coords.z < 1; coords.z++)
		for (coords.y = 0; coords.y < CHUNK_SIZE_XY; coords.y++)
			for (coords.x = 0; coords.x < CHUNK_SIZE_XY; coords.x++)
			{
				chunk->m_blockArray[index].SetBlockId(Blocks::BLOCK_BEDROCK);
				index += 1;
			}

	coords - IntVec3(coords.z, 0, 0);
	index = Chunk::GetIndex(coords);
	for (; coords.z < 4; coords.z++)
		for (coords.y = 0; coords.y < CHUNK_SIZE_XY; coords.y++)
			for (coords.x = 0; coords.x < CHUNK_SIZE_XY; coords.x++)
			{
				chunk->m_blockArray[index].SetBlockId(Blocks::BLOCK_DIRT);
				index += 1;
			}

	coords - IntVec3(coords.z, 0, 0);
	index = Chunk::GetIndex(coords);
	for (; coords.z < 5; coords.z++)
		for (coords.y = 0; coords.y < CHUNK_SIZE_XY; coords.y++)
			for (coords.x = 0; coords.x < CHUNK_SIZE_XY; coords.x++)
			{
				chunk->m_blockArray[index].SetBlockId(Blocks::BLOCK_GRASS);
				index += 1;
			}
}

void PerlinWorldGenerator::GenerateChunk(Chunk* chunk)
{
	WorldCoords origin = chunk->GetChunkOrigin();

	auto heightMapFunc = [](const WorldCoords& coords, unsigned int seed) {
		return 64 + int(30.f * Compute2dPerlinNoise(float(coords.x), float(coords.y), 200.f, 5, 0.5f, 2.0f, false, seed));
	};

	int heightMap[CHUNK_SIZE_XY * CHUNK_SIZE_XY] = {};

	LocalCoords coords = IntVec3::ZERO;
	int index = 0;
	for (coords.y = 0; coords.y < CHUNK_SIZE_XY; coords.y++)
		for (coords.x = 0; coords.x < CHUNK_SIZE_XY; coords.x++)
		{
			heightMap[index] = heightMapFunc(origin + coords, m_seed);
			index += 1;
		}

	index = 0;
	for (coords.z = 0; coords.z < CHUNK_SIZE_Z; coords.z++)
		for (coords.y = 0; coords.y < CHUNK_SIZE_XY; coords.y++)
			for (coords.x = 0; coords.x < CHUNK_SIZE_XY; coords.x++)
			{
				int height = heightMap[Chunk::GetIndex(IntVec3(coords.x, coords.y, 0))];

				unsigned char blockId = Blocks::BLOCK_AIR;
				if (coords.z <= height)
				{
					if (height - coords.z == 0)
					{
						blockId = Blocks::BLOCK_GRASS;
					}
					else if (height - coords.z < 5)
					{
						blockId = Blocks::BLOCK_DIRT;
					}
					else
					{
						blockId = Blocks::BLOCK_STONE;

						float oreSeed = rng.RollRandomFloatZeroToOne();

						if (oreSeed < 0.01f * 0.05f)
							blockId = Blocks::BLOCK_DIAMOND_ORE;
						else if (oreSeed < 0.01f * 0.25f)
							blockId = Blocks::BLOCK_GOLD_ORE;
						else if (oreSeed < 0.01f * 1.0f)
							blockId = Blocks::BLOCK_IRON_ORE;
						else if (oreSeed < 0.01f * 4.0f)
							blockId = Blocks::BLOCK_COAL_ORE;
					}
				}
				if (blockId == Blocks::BLOCK_AIR && coords.z < 64)
					blockId = Blocks::BLOCK_WATER;

				chunk->m_blockArray[index].SetBlockId(blockId);
				index++;
			}
}

// =======================================================================================================
// ==================================   ADVANCED MAP GENERATION   ========================================
// =======================================================================================================

constexpr int CHUNK_WATER_LEVEL = 64;
constexpr int CHUNK_OCEAN_LEVEL = 35;
constexpr int CHUNK_BIOME_EXTEND_TREE_SIZE = 2; // trees are maximum of 3x3 blocks wide
constexpr int CHUNK_BIOME_EXTEND_TREE_FUNC = 2; // trees function compare width is 5x5
constexpr int CHUNK_BIOME_EXTEND = CHUNK_BIOME_EXTEND_TREE_SIZE + CHUNK_BIOME_EXTEND_TREE_FUNC;
constexpr int CHUNK_BIOME_EXTENDED_SIZE_XY = CHUNK_SIZE_XY + CHUNK_BIOME_EXTEND * 2;

OverworldWorldGenerator::OverworldWorldGenerator()
{
}

float HeightFunc(const WorldCoords& coords, unsigned int seed, float heightScale = 30.0f, float heightBase = 60.0f)
{
	constexpr float          scale = 200.0f;
	constexpr unsigned int   octaves = 5;
	constexpr float          octResist = 0.5f;
	constexpr float          octScale = 2.0f;

	return heightBase + heightScale * Hesitate3(abs(Compute2dPerlinNoise(float(coords.x), float(coords.y), scale, octaves, octResist, octScale, true, seed)));
}

float HeightFunc2(const WorldCoords& coords, unsigned int seed, float heightScale = 30.0f, float heightBase = 60.0f)
{
	constexpr float          scale = 200.0f;
	constexpr unsigned int   octaves = 5;
	constexpr float          octResist = 0.5f;
	constexpr float          octScale = 2.0f;

	return heightBase + heightScale * Compute2dPerlinNoise(float(coords.x), float(coords.y), scale, octaves, octResist, octScale, true, seed);
}

float HumidityFunc(const WorldCoords& coords, unsigned int seed)
{
	constexpr float          scale = 500.0f;
	constexpr unsigned int   octaves = 5;
	constexpr float          octResist = 0.5f;
	constexpr float          octScale = 2.0f;

	return Compute2dPerlinNoise(float(coords.x), float(coords.y), scale, octaves, octResist, octScale, true, seed);
}

float TemperatureFunc(const WorldCoords& coords, unsigned int seed)
{
	constexpr float          scale = 500.0f;
	constexpr unsigned int   octaves = 5;
	constexpr float          octResist = 0.5f;
	constexpr float          octScale = 2.0f;

	return Compute2dPerlinNoise(float(coords.x), float(coords.y), scale, octaves, octResist, octScale, true, seed);
}

float HillinessFunc(const WorldCoords& coords, unsigned int seed)
{
	constexpr float          scale = 50.0f;
	constexpr unsigned int   octaves = 2;
	constexpr float          octResist = 0.5f;
	constexpr float          octScale = 2.0f;

	return 5.0f * SmoothStart5(0.5f * (1.0f + Compute2dPerlinNoise(float(coords.x), float(coords.y), scale, octaves, octResist, octScale, true, seed)));
}

float OceannessFunc(const WorldCoords& coords, unsigned int seed)
{
	constexpr float          scale = 500.0f;
	constexpr unsigned int   octaves = 5;
	constexpr float          octResist = 0.5f;
	constexpr float          octScale = 2.0f;

	return Compute2dPerlinNoise(float(coords.x), float(coords.y), scale, octaves, octResist, octScale, true, seed);
}

float TreenessFunc(const WorldCoords& coords, unsigned int seed)
{
	constexpr float          scale = 800.0f;
	constexpr unsigned int   octaves = 13;
	constexpr float          octResist = 0.5f;
	constexpr float          octScale = 2.0f;

	return Compute2dPerlinNoise(float(coords.x), float(coords.y), scale, octaves, octResist, octScale, true, seed);
}

// GetRef: Get reference for extended 2d heat map for biome generation
float& GetRef(float* extendedMap, const LocalCoords& coords)
{
	int index = coords.x + CHUNK_BIOME_EXTEND + CHUNK_BIOME_EXTENDED_SIZE_XY * (coords.y + CHUNK_BIOME_EXTEND);
	// ASSERT_OR_DIE(index >= 0 && index < CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY, "Access violation");
	return extendedMap[index];
}

bool IsTree(float* treenessMap, const LocalCoords& coords)
{
	int treeSelf = (int)GetRef(treenessMap, coords);

	for (int offsetY = -CHUNK_BIOME_EXTEND_TREE_FUNC; offsetY <= CHUNK_BIOME_EXTEND_TREE_FUNC; offsetY++)
		for (int offsetX = -CHUNK_BIOME_EXTEND_TREE_FUNC; offsetX <= CHUNK_BIOME_EXTEND_TREE_FUNC; offsetX++)
		{
			if (offsetX == 0 && offsetY == 0) // ignore self
				continue;
			float value = GetRef(treenessMap, coords + IntVec3(offsetX, offsetY, 0));
			if ((int) value >= treeSelf) // find higher than self
				return false;
		}
	return true;
}

BlockTemplate TEMPLATE_TREE;

bool InitTree()
{
	TEMPLATE_TREE.m_blocks.emplace_back(0, 0, 0, Blocks::BLOCK_LOG);
	TEMPLATE_TREE.m_blocks.emplace_back(0, 0, 1, Blocks::BLOCK_LOG);
	TEMPLATE_TREE.m_blocks.emplace_back(0, 0, 2, Blocks::BLOCK_LOG);
	TEMPLATE_TREE.m_blocks.emplace_back(1, 0, 1, Blocks::BLOCK_LEAVES);
	TEMPLATE_TREE.m_blocks.emplace_back(1, 0, 2, Blocks::BLOCK_LEAVES);
	TEMPLATE_TREE.m_blocks.emplace_back(-1, 0, 1, Blocks::BLOCK_LEAVES);
	TEMPLATE_TREE.m_blocks.emplace_back(-1, 0, 2, Blocks::BLOCK_LEAVES);
	TEMPLATE_TREE.m_blocks.emplace_back(0, 1, 1, Blocks::BLOCK_LEAVES);
	TEMPLATE_TREE.m_blocks.emplace_back(0, 1, 2, Blocks::BLOCK_LEAVES);
	TEMPLATE_TREE.m_blocks.emplace_back(0, -1, 1, Blocks::BLOCK_LEAVES);
	TEMPLATE_TREE.m_blocks.emplace_back(0, -1, 2, Blocks::BLOCK_LEAVES);

	return true;
}

void OverworldWorldGenerator::GenerateChunk(Chunk* chunk)
{
	WorldCoords origin = chunk->GetChunkOrigin();

	unsigned int seed_Height      = m_seed + 0;
	unsigned int seed_Humidity    = m_seed + 1;
	unsigned int seed_Temperature = m_seed + 2;
	unsigned int seed_Hilliness   = m_seed + 3;
	unsigned int seed_Treeness    = m_seed + 4;
	unsigned int seed_Oceanness   = m_seed + 5;
	unsigned int seed_Lampness    = m_seed + 6;

	float heightMap      [CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];
	float humidityMap    [CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];
	float temperatureMap [CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];
	float hillinessMap   [CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];
	float treenessMap    [CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];
	float lampnessMap    [CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];
	float oceannessMap   [CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];

	LocalCoords coords = IntVec3::ZERO;
	int index = 0;
	for (coords.y = -CHUNK_BIOME_EXTEND; coords.y < (int)CHUNK_SIZE_XY + CHUNK_BIOME_EXTEND; coords.y++)
		for (coords.x = -CHUNK_BIOME_EXTEND; coords.x < (int)CHUNK_SIZE_XY + CHUNK_BIOME_EXTEND; coords.x++)
		{
			float height      = HeightFunc(origin + coords, seed_Height);
			float humidity    = HumidityFunc(origin + coords, seed_Humidity);
			float temperature = TemperatureFunc(origin + coords, seed_Temperature);
			float hilliness   = HillinessFunc(origin + coords, seed_Hilliness);
			float oceanness   = OceannessFunc(origin + coords, seed_Oceanness);

			// process height with hilliness
			height = (height > 64.0f) ? ((height - 64.0f) * hilliness + 64.0f) : height;

			// process height with oceanness
			float oceanBlend = RangeMapClamped(oceanness, 0.0f, 0.5f, 0.0f, 1.0f);
			height = Lerp(height, (float)CHUNK_OCEAN_LEVEL, oceanBlend);
			
			GetRef(heightMap, coords) = height;
			GetRef(hillinessMap, coords) = hilliness;
			GetRef(oceannessMap, coords) = oceanness;
			GetRef(humidityMap, coords) = humidity;
			GetRef(temperatureMap, coords) = temperature;
			GetRef(treenessMap, coords) = TreenessFunc(origin + coords, seed_Treeness) * 1000.0f;
			GetRef(lampnessMap, coords) = TreenessFunc(origin + coords, seed_Lampness) * 1000.0f;
		}

	index = 0;
	for (coords.z = 0; coords.z < CHUNK_SIZE_Z; coords.z++)
		for (coords.y = 0; coords.y < CHUNK_SIZE_XY; coords.y++)
			for (coords.x = 0; coords.x < CHUNK_SIZE_XY; coords.x++)
			{
				int height = (int)GetRef(heightMap, coords);

				unsigned char blockId = Blocks::BLOCK_AIR;
				if (coords.z <= height)
				{
					// generate grass
					if (height - coords.z == 0)
					{
						blockId = Blocks::BLOCK_GRASS;
					}
					// generate dirt
					else if (height - coords.z < 5)
					{
						blockId = Blocks::BLOCK_DIRT;
					}
					// generate stone
					else
					{
						blockId = Blocks::BLOCK_STONE;

						// generate ores (stone -> ores)
						float oreSeed = rng.RollRandomFloatZeroToOne();

						if (oreSeed < 0.01f * 0.05f)
							blockId = Blocks::BLOCK_DIAMOND_ORE;
						else if (oreSeed < 0.01f * 0.25f)
							blockId = Blocks::BLOCK_GOLD_ORE;
						else if (oreSeed < 0.01f * 1.0f)
							blockId = Blocks::BLOCK_IRON_ORE;
						else if (oreSeed < 0.01f * 4.0f)
							blockId = Blocks::BLOCK_COAL_ORE;
					}
				}


				// generate beach
				if (GetRef(humidityMap, coords) < 0.1f)
				{
					if (GetRef(oceannessMap, coords) > -0.05f)
					{
						if (coords.z == CHUNK_WATER_LEVEL && blockId == Blocks::BLOCK_GRASS)
							blockId = Blocks::BLOCK_SAND;
					}
				}

				// replace air to water below water level
				if (blockId == Blocks::BLOCK_AIR && coords.z <= CHUNK_WATER_LEVEL)
					blockId = Blocks::BLOCK_WATER; // GetRef(oceannessMap, coords) > 0 ? Blocks::BLOCK_RED_BRICK : Blocks::BLOCK_WATER;

				// sand & ice
				if (height <= CHUNK_WATER_LEVEL)
				{
					float sandLevel = RangeMap(GetRef(humidityMap, coords), -0.1f, -0.2f, 64.0f, 60.0f);
					float iceLevel  = RangeMap(GetRef(temperatureMap , coords), -0.1f, -0.2f, 64.0f, 60.0f); ;

					// water ->ice
					if (coords.z > iceLevel && blockId == Blocks::BLOCK_WATER)
						blockId = Blocks::BLOCK_ICE;

					// water -> sand
					if (sandLevel <= CHUNK_WATER_LEVEL && blockId == Blocks::BLOCK_WATER && GetRef(oceannessMap, coords) <= 0)
						blockId = Blocks::BLOCK_SAND;

					// grass & dirt -> sand
					if (coords.z > sandLevel && (blockId == Blocks::BLOCK_DIRT || blockId == Blocks::BLOCK_GRASS))
						blockId = Blocks::BLOCK_SAND;
				}

				chunk->m_blockArray[index].SetBlockId(blockId);
				index++;
			}


	// generate trees
	static const bool initTree = InitTree(); // thread safe

	for (coords.y = -CHUNK_BIOME_EXTEND_TREE_SIZE; coords.y < (int)CHUNK_SIZE_XY + CHUNK_BIOME_EXTEND_TREE_SIZE; coords.y++)
		for (coords.x = -CHUNK_BIOME_EXTEND_TREE_SIZE; coords.x < (int)CHUNK_SIZE_XY + CHUNK_BIOME_EXTEND_TREE_SIZE; coords.x++)
		{
			if (IsTree(treenessMap, coords) && GetRef(humidityMap, coords) > 0.0f && GetRef(temperatureMap, coords) > 0.0f)
			{
				coords.z = (int)GetRef(heightMap, coords);
				if (coords.z >= CHUNK_WATER_LEVEL)
				{
					coords.z++;
					TEMPLATE_TREE.PlaceOn(chunk, coords);
				}
			}
		}

	for (coords.y = 0; coords.y < (int)CHUNK_SIZE_XY; coords.y++)
		for (coords.x = 0; coords.x < (int)CHUNK_SIZE_XY; coords.x++)
		{
			if (IsTree(lampnessMap, coords) && GetRef(humidityMap, coords) < 0.0f && GetRef(oceannessMap, coords) < -0.05f)
			{
				coords.z = (int)GetRef(heightMap, coords);
				if (chunk->GetBlockId(coords) == Blocks::BLOCK_GRASS && coords.z >= CHUNK_WATER_LEVEL)
				{
					coords.z++;
					chunk->m_blockArray[Chunk::GetIndex(coords)].SetBlockId(Blocks::BLOCK_GLOWSTONE);
				}
			}
		}
}


void BlockTemplate::PlaceOn(Chunk* chunk, const LocalCoords& coords) const
{
	for (auto& entry : m_blocks)
	{
		auto offset = coords + entry.m_offset;

		if (offset.x >= 0 && offset.x < CHUNK_SIZE_XY)
			if (offset.y >= 0 && offset.y < CHUNK_SIZE_XY)
				if (offset.z >= 0 && offset.z < CHUNK_SIZE_Z)
				{
					chunk->m_blockArray[Chunk::GetIndex(offset)].SetBlockId(entry.m_blockId);
				}
	}
}

BlockTemplate::Entry::Entry()
	: m_blockId(Blocks::BLOCK_AIR)
{
}

BlockTemplate::Entry::Entry(int x, int y, int z, BlockId block)
	: m_offset(IntVec3(x, y, z))
	, m_blockId(block)
{

}

BlockTemplate::Entry::Entry(const IntVec3& coords, BlockId block)
	: m_offset(coords)
	, m_blockId(block)
{

}

SkyBlockWorldGenerator::SkyBlockWorldGenerator()
{

}

void SkyBlockWorldGenerator::GenerateChunk(Chunk* chunk)
{
	WorldCoords origin = chunk->GetChunkOrigin();

	unsigned int seed_Height1 = m_seed + 0;
	unsigned int seed_Height2 = m_seed + 20;
	unsigned int seed_Humidity = m_seed + 1;
	unsigned int seed_Temperature = m_seed + 2;
	unsigned int seed_Hilliness = m_seed + 3;
	unsigned int seed_Treeness = m_seed + 4;
	unsigned int seed_Lampness = m_seed + 6;

	float height1Map[CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];
	float height2Map[CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];
	float humidityMap[CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];
	float temperatureMap[CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];
	float hillinessMap[CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];
	float treenessMap[CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];
	float lampnessMap[CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];
	float oceannessMap[CHUNK_BIOME_EXTENDED_SIZE_XY * CHUNK_BIOME_EXTENDED_SIZE_XY];

	LocalCoords coords = IntVec3::ZERO;
	int index = 0;
	for (coords.y = -CHUNK_BIOME_EXTEND; coords.y < (int)CHUNK_SIZE_XY + CHUNK_BIOME_EXTEND; coords.y++)
		for (coords.x = -CHUNK_BIOME_EXTEND; coords.x < (int)CHUNK_SIZE_XY + CHUNK_BIOME_EXTEND; coords.x++)
		{
			float height1 = HeightFunc(origin + coords, seed_Height1);
			float height2 = HeightFunc2(origin + coords, seed_Height2);
			float humidity = HumidityFunc(origin + coords, seed_Humidity);
			float temperature = TemperatureFunc(origin + coords, seed_Temperature);
			float hilliness = HillinessFunc(origin + coords, seed_Hilliness);

			// process height with hilliness
			height1 = (height1 > 64.0f) ? ((height1 - 64.0f) * hilliness + 64.0f) : height1;
			height2 = (height2 > 64.0f) ? ((height2 - 64.0f) * 5.0f + 64.0f) : height2;
			height2 = 64.0f + -(height2 - 64.0f);

			height2 += 0;

			height1 = 64.0f + Clamp(height1 - 64.0f, height1 - 64.0f, Clamp(64.0f - height2, 0.0f, 64.0f - height2));

			if (height2 >= 60.0f)
				height1 -= height2 - 60;

			if (height2 >= 64.0f)
				height2 += 50;


			GetRef(height1Map, coords) = height1;
			GetRef(height2Map, coords) = height2 - 0.0f;
			GetRef(hillinessMap, coords) = hilliness;
			GetRef(humidityMap, coords) = humidity;
			GetRef(temperatureMap, coords) = temperature;
			GetRef(treenessMap, coords) = TreenessFunc(origin + coords, seed_Treeness) * 1000.0f;
			GetRef(lampnessMap, coords) = TreenessFunc(origin + coords, seed_Lampness) * 1000.0f;
		}

	index = 0;
	for (coords.z = 0; coords.z < CHUNK_SIZE_Z; coords.z++)
		for (coords.y = 0; coords.y < CHUNK_SIZE_XY; coords.y++)
			for (coords.x = 0; coords.x < CHUNK_SIZE_XY; coords.x++)
			{
				int height1 = (int)GetRef(height1Map, coords);
				int height2 = (int)GetRef(height2Map, coords);

				unsigned char blockId = Blocks::BLOCK_AIR;

				if (coords.z <= height1 && coords.z > height2)
				{
					// generate grass
					if (height1 - coords.z == 0)
					{
						blockId = Blocks::BLOCK_GRASS;
					}
					// generate dirt
					else if (height1 - coords.z < 5)
					{
						blockId = Blocks::BLOCK_DIRT;
					}
					// generate stone
					else
					{
						blockId = Blocks::BLOCK_STONE;

						// generate ores (stone -> ores)
						float oreSeed = rng.RollRandomFloatZeroToOne();

						if (oreSeed < 0.01f * 0.05f)
							blockId = Blocks::BLOCK_DIAMOND_ORE;
						else if (oreSeed < 0.01f * 0.25f)
							blockId = Blocks::BLOCK_GOLD_ORE;
						else if (oreSeed < 0.01f * 1.0f)
							blockId = Blocks::BLOCK_IRON_ORE;
						else if (oreSeed < 0.01f * 4.0f)
							blockId = Blocks::BLOCK_COAL_ORE;
					}
				}


				// generate beach
				if (GetRef(humidityMap, coords) < 0.1f)
				{
					if (GetRef(oceannessMap, coords) > -0.05f)
					{
						if (coords.z == CHUNK_WATER_LEVEL && blockId == Blocks::BLOCK_GRASS)
							blockId = Blocks::BLOCK_SAND;
					}
				}

				// replace air to water below water level
				if (blockId == Blocks::BLOCK_AIR && coords.z <= CHUNK_WATER_LEVEL && coords.z > height2)
				{
					if (coords.z > height2 + 5)
						blockId = Blocks::BLOCK_WATER;
					else if (coords.z == CHUNK_WATER_LEVEL)
						blockId = Blocks::BLOCK_GRASS;
					else
						blockId = Blocks::BLOCK_DIRT;
				}

				// sand & ice
				if (height1 <= CHUNK_WATER_LEVEL && coords.z <= height1 && coords.z > height2)
				{
					float sandLevel = RangeMap(GetRef(humidityMap, coords), -0.1f, -0.2f, 64.0f, 60.0f);
					float iceLevel = RangeMap(GetRef(temperatureMap, coords), -0.1f, -0.2f, 64.0f, 60.0f); ;

					// water ->ice
					if (coords.z > iceLevel && blockId == Blocks::BLOCK_WATER)
						blockId = Blocks::BLOCK_ICE;

					// water -> sand
					if (sandLevel <= CHUNK_WATER_LEVEL && blockId == Blocks::BLOCK_WATER && GetRef(oceannessMap, coords) <= 0)
						blockId = Blocks::BLOCK_SAND;

					// grass & dirt -> sand
					if (coords.z > sandLevel && (blockId == Blocks::BLOCK_DIRT || blockId == Blocks::BLOCK_GRASS))
						blockId = Blocks::BLOCK_SAND;
				}

				chunk->m_blockArray[index].SetBlockId(blockId);
				index++;
			}


	index = 0;
	for (coords.y = 0; coords.y < CHUNK_SIZE_XY; coords.y++)
		for (coords.x = 0; coords.x < CHUNK_SIZE_XY; coords.x++)
		{
			BlockId blockId = Blocks::BLOCK_AIR;

			for (coords.z = CHUNK_MAX_Z; coords.z >= 0; coords.z--)
			{
				index = Chunk::GetIndex(coords);

				if (chunk->m_blockArray[index].GetBlockId() == Blocks::BLOCK_WATER)
					blockId = Blocks::BLOCK_WATER;
				if (chunk->m_blockArray[index].IsOpaque())
					blockId = Blocks::BLOCK_AIR;

				if (chunk->m_blockArray[index].GetBlockId() == Blocks::BLOCK_AIR)
					chunk->m_blockArray[index].SetBlockId(blockId);
			}
		}

	// generate trees
	static const bool initTree = InitTree(); // thread safe

	for (coords.y = -CHUNK_BIOME_EXTEND_TREE_SIZE; coords.y < (int)CHUNK_SIZE_XY + CHUNK_BIOME_EXTEND_TREE_SIZE; coords.y++)
		for (coords.x = -CHUNK_BIOME_EXTEND_TREE_SIZE; coords.x < (int)CHUNK_SIZE_XY + CHUNK_BIOME_EXTEND_TREE_SIZE; coords.x++)
		{
			int height2 = (int)GetRef(height2Map, coords);
			if (IsTree(treenessMap, coords) && GetRef(humidityMap, coords) > 0.0f && GetRef(temperatureMap, coords) > 0.0f && height2 <= 64.0f)
			{
				coords.z = (int)GetRef(height1Map, coords);
				if (coords.z >= CHUNK_WATER_LEVEL)
				{
					coords.z++;
					TEMPLATE_TREE.PlaceOn(chunk, coords);
				}
			}
		}

	for (coords.y = 0; coords.y < (int)CHUNK_SIZE_XY; coords.y++)
		for (coords.x = 0; coords.x < (int)CHUNK_SIZE_XY; coords.x++)
		{
			int height2 = (int)GetRef(height2Map, coords);
			if (IsTree(lampnessMap, coords) && GetRef(humidityMap, coords) < 0.0f && GetRef(oceannessMap, coords) < -0.05f && height2 <= 64.0f)
			{
				coords.z = (int)GetRef(height1Map, coords);
				if (chunk->GetBlockId(coords) == Blocks::BLOCK_GRASS && coords.z >= CHUNK_WATER_LEVEL)
				{
					coords.z++;
					chunk->m_blockArray[Chunk::GetIndex(coords)].SetBlockId(Blocks::BLOCK_GLOWSTONE);
				}
			}
		}
}
