#include "Game/ChunkProvider.hpp"

#include "Engine/Core/ByteBuffer.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Game/World.hpp"
#include "Game/BlockDef.hpp"
#include "Game/WorldGenerator.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

#include <filesystem>

int g_nbrReqCounter = 0;
int g_dirtyCounter = 0;
int g_acceptedCounter = 0;
int g_rejectedCounter = 0;
int g_changedCounter = 0;
int g_notchangedCounter = 0;
int g_newReqCounter = 0;

extern RandomNumberGenerator rng;

ChunkProvider::ChunkProvider(World* world, const char* folderPath, WorldGenerator* generator)
	: m_world(world)
	, m_path(folderPath)
	, m_generator(generator)
{
	std::filesystem::create_directories(std::filesystem::path(folderPath));

	m_chunkActivationRange = g_gameConfigBlackboard.GetValue("chunkActivationRange", m_chunkActivationRange);
	m_worldSeed = (unsigned int)g_gameConfigBlackboard.GetValue("worldSeed", (int)m_worldSeed);
	m_generator->m_seed = m_worldSeed;

	m_rndTickWatch.Start(1.0 / 20.0);
}

ChunkProvider::~ChunkProvider()
{
	delete m_generator;
}

void ChunkProvider::Update()
{
	BeginFrame();

	for (auto& chunkEntry : GetLoadedChunks())
		chunkEntry.second->Update();

	UpdateRandomTick();

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		UnloadAllChunks();
	}

	EndFrame();
}

std::map<ChunkCoords, Chunk*>& ChunkProvider::GetLoadedChunks()
{
	return m_chunksLoaded;
}

const std::map<ChunkCoords, Chunk*>& ChunkProvider::GetLoadedChunks() const
{
	return m_chunksLoaded;
}

bool ChunkProvider::LoadChunkWithTicket(const ChunkCoords& coords)
{
	if (m_chunkIOTicket <= 0)
		return false;
	if (LoadChunk(coords) == ChunkLoadStatus::LOADED)
		GetChunkIOTicket();
	return true;
}

void ChunkProvider::PopulateChunk(Chunk* chunk)
{
	m_generator->GenerateChunk(chunk);
	chunk->m_meshDirty = true;
	chunk->m_blocksDirty = true;
}

Chunk* ChunkProvider::FindLoadedChunk(const ChunkCoords& coords) const
{
	auto ite = m_chunksLoaded.find(coords);
	if (ite != m_chunksLoaded.end())
		return ite->second;
	return nullptr;
}

std::string ChunkProvider::GetFileName(const ChunkCoords& coords) const
{
	return Stringf("%s/Chunk(%d,%d).chunk", m_path.c_str(), coords.x, coords.y);
}

void ChunkProvider::UnloadAllChunks()
{
	while (!m_chunksGenerating.empty())
	{
		g_theJobSystem->FinishUpJobsOfType(JOB_TYPE_GEN_CHUNK);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	for (auto& entry : m_chunksLoaded)
	{
		SaveChunkToDisk(entry.second);
		delete entry.second;
	}
	m_chunksLoaded.clear();
}

const Block& ChunkProvider::GetBlock(const WorldCoords& coords) const
{
	if (coords.z < 0 || coords.z >= CHUNK_SIZE_Z)
		return Block::INVALID;

	auto ite = m_chunksLoaded.find(Chunk::GetChunkCoords(coords));
	if (ite == m_chunksLoaded.end())
	{
		return Block::INVALID;
	}

	return ite->second->GetBlock(Chunk::GetLocalCoords(coords));
}

BlockId ChunkProvider::GetBlockId(const WorldCoords& coords) const
{
	if (coords.z < 0 || coords.z >= CHUNK_SIZE_Z)
		return Blocks::BLOCK_AIR;

	auto ite = m_chunksLoaded.find(Chunk::GetChunkCoords(coords));
	if (ite == m_chunksLoaded.end())
	{
		return Blocks::BLOCK_AIR;
	}

	return ite->second->GetBlockId(Chunk::GetLocalCoords(coords));
}

void ChunkProvider::SetBlockId(const WorldCoords& coords, BlockId block)
{
	if (coords.z < 0 || coords.z >= CHUNK_SIZE_Z)
	{
		ERROR_RECOVERABLE("Setting block out of world");
		return;
	}

	auto ite = m_chunksLoaded.find(Chunk::GetChunkCoords(coords));
	if (ite == m_chunksLoaded.end())
	{
		ERROR_RECOVERABLE("Chunk not loaded");
		return;
	}

	ite->second->SetBlockId(Chunk::GetLocalCoords(coords), block);
}

#include "Engine/Renderer/DebugRender.hpp"

void ChunkProvider::ProcessDirtyLighting()
{
	g_nbrReqCounter = 0;
	g_dirtyCounter = 0;
	g_acceptedCounter = 0;
	g_rejectedCounter = 0;
	g_changedCounter = 0;
	g_notchangedCounter = 0;
	g_newReqCounter = 0;

	if (WORLD_DEBUG_STEP_LIGHTING)
		if (!g_theInput->WasKeyJustPressed(KEYCODE_G))
			return;

	if (WORLD_DEBUG_STEP_LIGHTING)
		m_dirtyLighting.swap(m_debugLighting);
	size_t initial = m_dirtyLighting.size();
	while (m_dirtyLighting.size())
	{
		g_dirtyCounter++;
		ProcessNextDirtyLightBlock();
	}

	if (initial == 0)
		return;

// 	const char* info = "LightInfo: Init=%d, Dirty=%d, New=%d, Acc=%d, Rej=%d, Chg=%d, NoChg=%d, Nbr=%d";
// 	std::string msg = Stringf(info, initial, g_dirtyCounter, g_newReqCounter, g_acceptedCounter, g_rejectedCounter, g_changedCounter, g_notchangedCounter, g_nbrReqCounter);
// 	DebugAddMessage(msg, 10.f, Rgba8::WHITE, Rgba8::WHITE);
}

void ChunkProvider::ProcessNextDirtyLightBlock()
{
	BlockIterator ite = m_dirtyLighting.front();
	m_dirtyLighting.pop_front();
	Block* block = ite.GetBlock();

	block->SetLightDirty(false);

	if (g_theInput->IsKeyDown(KEYCODE_H))
	{
		IntVec3 worldCoords = ite.GetWorldCoords();
		Vec3 world = Vec3((float)worldCoords.x + 0.5f, (float)worldCoords.y + 0.5f, (float)worldCoords.z + 0.5f);
		DebugAddWorldPoint(world, 0.2f, 5.0f, Rgba8::RED, Rgba8::WHITE, DebugRenderMode::XRAY);
	}

	LightLevel iLightPrev = block->GetIndoorLightInfluence();
	LightLevel oLightPrev = block->GetOutdoorLightInfluence();

	LightLevel iLight = block->GetGlowLight();
	LightLevel oLight = block->GetSkyLight();

	BlockIterator iteNbrs[BlockFace::BLOCK_FACE_SIZE] = {};
	Block*        nbrs[BlockFace::BLOCK_FACE_SIZE]    = {};

	for (BlockFace face : BLOCK_NEIGHBORS)
	{
		iteNbrs[face] = ite.GetBlockNeighbor(face);
		g_nbrReqCounter++;
		nbrs[face] = iteNbrs[face].GetBlock();
	}

	if (!block->IsOpaque())
	{
		// transparent (air)
		for (BlockFace face : BLOCK_NEIGHBORS)
		{
			Block* nbr = nbrs[face];
			if (nbr->IsValid())
			{
				LightLevel iLightNbr = nbr->GetIndoorLightInfluence();
				LightLevel oLightNbr = nbr->GetOutdoorLightInfluence();
				if (iLightNbr > 0)
					iLightNbr--;
				if (oLightNbr > 0)
					oLightNbr--;
				iLight = Max(iLight, iLightNbr);
				oLight = Max(oLight, oLightNbr);
			}
		}
	}

	if (iLight != iLightPrev || oLight != oLightPrev)
	{
		g_changedCounter++;

		block->SetIndoorLightInfluence(iLight);
		block->SetOutdoorLightInfluence(oLight);
		ite.GetChunk()->m_meshDirty = true;
		for (BlockFace face : BLOCK_NEIGHBORS)
		{
			const BlockIterator& iteNbr = iteNbrs[face];
			Block* nbr = nbrs[face];
			if (nbr->IsValid())
			{
				if (nbr->IsOpaque()) // a face of neighbor's lighting has changed
				{
					iteNbr.GetChunk()->m_meshDirty = true;
				}
				else // a block of neighbor needs to update light
				{
					g_newReqCounter++;
					MarkLightingDirty(iteNbr);
				}
			}
		}
	}
	else
	{
		g_notchangedCounter++;
	}
}

void ChunkProvider::MarkLightingDirty(const BlockIterator& blockIte)
{
	Block* blk = blockIte.GetBlock();
	if (!blk->IsValid() || blk->IsLightDirty())
	{
		g_rejectedCounter++;
		return;
	}
	g_acceptedCounter++;
	blk->SetLightDirty(true);
	(WORLD_DEBUG_STEP_LIGHTING ? m_debugLighting : m_dirtyLighting).push_back(blockIte);
}

void ChunkProvider::MarkLightingDirty(const WorldCoords& worldCoords)
{
	MarkLightingDirty(BlockIterator(this, worldCoords));
}

void ChunkProvider::UndirtyAllBlocksInChunk(const ChunkCoords& chunkCoords)
{
	Chunk* chunk = FindLoadedChunk(chunkCoords);
	if (!chunk)
		return;

	auto ite = m_dirtyLighting.begin();
	while (ite != m_dirtyLighting.end())
	{
		auto& blockIte = *ite;
		if (blockIte.GetChunk() == chunk)
		{
			blockIte.GetBlock()->SetLightDirty(false);
			ite = m_dirtyLighting.erase(ite);
			continue;
		}
		ite++;
	}
}

bool ChunkProvider::GetChunkIOTicket()
{
	if (m_chunkIOTicket > 0)
	{
		m_chunkIOTicket--;
		return true;
	}
	return false;
}

bool ChunkProvider::GetRebuildMeshTicket()
{
	if (m_rebuildMeshTicket > 0)
	{
		m_rebuildMeshTicket--;
		return true;
	}
	return false;
}

void ChunkProvider::SetHotspotSize(int size)
{
	m_hotspots.resize(size);
}

void ChunkProvider::SetHotspot(int index, const Vec3& worldPos)
{
	ChunkCoords newCoords = Chunk::GetChunkCoords(worldPos);
	if (m_hotspots[index] != newCoords) 
	{
		m_hotspots[index] = newCoords;
		if (m_chunksLoaded.find(newCoords) == m_chunksLoaded.end())
			LoadChunk(newCoords); // make sure chunk is loaded otherwise player will fall into ground
	}
}

void ChunkProvider::BeginFrame()
{
	m_rebuildMeshTicket = 2;
	m_chunkIOTicket = 2;

	DoChunkActivation();
	DoChunkDeactivation();
}

void ChunkProvider::UpdateRandomTick()
{
	if (m_rndTickWatch.CheckDurationElapsedAndDecrement())
	{
		float rndSource[1024];
		for (int i = 0; i < 1024; i++)
			rndSource[i] = rng.RollRandomFloatZeroToOne();

		int rndTickChunksRadius = 8;

		for (auto& hotspot : m_hotspots)
		{
			Chunk* origin = FindLoadedChunk(hotspot);
			if (origin)
				origin->UpdateRandomTick(1024, rndSource, rng.RollRandomIntInRange(0, 1023));

			for (int size = 1; size < rndTickChunksRadius; size++)
			{
				ChunkCoords coords = hotspot + IntVec2(size, size);
				for (BlockFace face : { BLOCK_FACE_EAST, BLOCK_FACE_SOUTH, BLOCK_FACE_WEST, BLOCK_FACE_NORTH })
				{
					for (int i = 0; i < size * 2; i++)
					{
						if ((coords - hotspot).GetLengthSquared() < rndTickChunksRadius * rndTickChunksRadius)
						{
							Chunk* chunk = FindLoadedChunk(coords);
							if (chunk)
								chunk->UpdateRandomTick(1024, rndSource, rng.RollRandomIntInRange(0, 1023));
						}
						coords += Block::GetOffset2ByFace(face);
					}
				}
			}
		}
	}
}

void ChunkProvider::DoChunkDeactivation()
{
	int chunkDeactivationRange = m_chunkActivationRange + CHUNK_SIZE_XY + CHUNK_SIZE_XY;
	int unloadChunksRadius = 1 + chunkDeactivationRange / CHUNK_SIZE_XY;

	std::vector<ChunkCoords> unloadingChunks;
	for (const auto& entry : m_chunksLoaded)
	{
		bool unload = true;
		for (const auto& hotspot : m_hotspots)
		{
			if ((entry.first - hotspot).GetLengthSquared() < unloadChunksRadius * unloadChunksRadius)
			{
				unload = false;
				break;
			}
		}
		if (unload)
			unloadingChunks.push_back(entry.first);
	}

	ChunkCoords coordsMax;
	int lenSqMax = -1;
	for (const auto& coords : unloadingChunks)
	{
		int lenSqTemp = 0;
		for (const auto& hotspot : m_hotspots)
		{
			lenSqTemp += (coords - hotspot).GetLengthSquared();
		}
		if (lenSqTemp > lenSqMax)
		{
			lenSqMax = lenSqTemp;
			coordsMax = coords;
		}
	}

	if (lenSqMax >= 0 && GetChunkIOTicket())
		UnloadChunk(coordsMax);
}

void ChunkProvider::DoChunkActivation()
{
	int loadChunksRadius = 1 + m_chunkActivationRange / CHUNK_SIZE_XY;
	int maxChunks = (2 * loadChunksRadius) * (2 * loadChunksRadius);

	if (m_chunksLoaded.size() < maxChunks)
	{
		for (auto& hotspot : m_hotspots)
		{
			if (!LoadChunkWithTicket(hotspot))
				return;
			for (int size = 1; size < loadChunksRadius; size++)
			{
				ChunkCoords coords = hotspot + IntVec2(size, size);
				for (BlockFace face : { BLOCK_FACE_EAST, BLOCK_FACE_SOUTH, BLOCK_FACE_WEST, BLOCK_FACE_NORTH })
				{
					for (int i = 0; i < size * 2; i++)
					{
						if ((coords - hotspot).GetLengthSquared() < loadChunksRadius * loadChunksRadius)
						{
							if (!LoadChunkWithTicket(coords))
								return;
						}
						coords += Block::GetOffset2ByFace(face);
					}
				}
			}
		}
	}
}

void ChunkProvider::EndFrame()
{
	ProcessDirtyLighting();

	g_theJobSystem->FinishUpJobsOfType(JOB_TYPE_GEN_CHUNK, 2);
}

ChunkLoadStatus ChunkProvider::LoadChunk(const ChunkCoords& coords)
{
	auto ite = m_chunksLoaded.find(coords);
	if (ite != m_chunksLoaded.end())
		return ChunkLoadStatus::PRESENT;
	auto iteGen = m_chunksGenerating.find(coords);
	if (iteGen != m_chunksGenerating.end())
		return ChunkLoadStatus::QUEUED;

	Chunk* chunk = new Chunk(m_world, coords);
	if (!LoadChunkFromDisk(chunk))
	{
		chunk->m_state = ChunkState::QUEUED;
		m_chunksGenerating[coords] = chunk; // insert into m_chunksLoaded
		g_theJobSystem->QueueJob(new ChunkPopulateJob(this, chunk));
		return ChunkLoadStatus::LOADED;
	}
	else
	{
		FinishUpChunkLoading(chunk);
	}
	return ChunkLoadStatus::LOADED;
}

void ChunkProvider::UnloadChunk(const ChunkCoords& coords)
{
	auto ite = m_chunksLoaded.find(coords);
	if (ite == m_chunksLoaded.end())
		return;

	Chunk* chunk = ite->second;
	for (BlockFace face : CHUNK_NEIGHBORS)
		if (chunk->m_neighbors[(int)face])
			chunk->m_neighbors[(int)face]->OnNeighborUnload(*ite->second);
	m_chunksLoaded.erase(ite);
	SaveChunkToDisk(chunk);
	delete chunk;
}

// ============ IO ================ //

constexpr const char*   CHUNK_FILE_HEADER = "GCHK";
constexpr unsigned char CHUNK_FILE_HEADER_SIZE = 4;
constexpr unsigned char CHUNK_FILE_VERSION = 2;
constexpr unsigned char CHUNK_FILE_BITS_X = (unsigned char)CHUNK_SIZE_BITWIDTH_XY;
constexpr unsigned char CHUNK_FILE_BITS_Y = (unsigned char)CHUNK_SIZE_BITWIDTH_XY;
constexpr unsigned char CHUNK_FILE_BITS_Z = (unsigned char)CHUNK_SIZE_BITWIDTH_Z;


bool ChunkProvider::LoadChunkFromDisk(Chunk* chunk) const
{
	if (m_disableLoadFromDisk)
		return false; // Disabled load from disk.

	ByteBuffer buffer;
	int len = FileReadToBuffer(buffer, GetFileName(chunk->m_chunkCoords));
	if (len == -1)
		return false; // Read file failed.

	unsigned char chunk_FILE_HEADER[CHUNK_FILE_HEADER_SIZE];
	unsigned char chunk_FILE_VERSION;
	unsigned int  worldSeed;
	unsigned char chunk_FILE_BITS_X;
	unsigned char chunk_FILE_BITS_Y;
	unsigned char chunk_FILE_BITS_Z;

	buffer.Read(CHUNK_FILE_HEADER_SIZE, &chunk_FILE_HEADER[0]);
	buffer.Read(chunk_FILE_VERSION);
	buffer.Read(worldSeed);
	buffer.Read(chunk_FILE_BITS_X);
	buffer.Read(chunk_FILE_BITS_Y);
	buffer.Read(chunk_FILE_BITS_Z);

	for (size_t i = 0; i < CHUNK_FILE_HEADER_SIZE; i++)
		if (chunk_FILE_HEADER[i] != CHUNK_FILE_HEADER[i])
			return false; // Corrupt chunk file.
	if (chunk_FILE_VERSION != CHUNK_FILE_VERSION)
		return false; // Incompatible chunk file version.
	if (worldSeed != m_worldSeed)
		return false; // Incompatible world seed.
	if (chunk_FILE_BITS_X != CHUNK_FILE_BITS_X || chunk_FILE_BITS_Y != CHUNK_FILE_BITS_Y || chunk_FILE_BITS_Z != CHUNK_FILE_BITS_Z)
		return false; // Incompatible chunk array bit length.

	chunk->ReadBytes(&buffer);
	return true;
}

bool ChunkProvider::SaveChunkToDisk(const Chunk* chunk) const
{
	if (!chunk->m_blocksDirty)
		return true;

	ByteBuffer buffer;

	// write info
	buffer.Write(CHUNK_FILE_HEADER_SIZE, &CHUNK_FILE_HEADER[0]);
	buffer.Write(CHUNK_FILE_VERSION);
	buffer.Write(m_worldSeed);
	buffer.Write(CHUNK_FILE_BITS_X);
	buffer.Write(CHUNK_FILE_BITS_Y);
	buffer.Write(CHUNK_FILE_BITS_Z);
	
	chunk->WriteBytes(&buffer);
	int len = FileWriteFromBuffer(buffer, GetFileName(chunk->m_chunkCoords));
	return len != -1;
}

void ChunkProvider::FinishUpChunkLoading(Chunk* chunk)
{
	m_chunksLoaded[chunk->m_chunkCoords] = chunk;

	for (BlockFace face : CHUNK_NEIGHBORS)
	{
		auto iteNeighbor = m_chunksLoaded.find(chunk->m_chunkCoords + Block::GetOffset2ByFace(face));
		if (iteNeighbor != m_chunksLoaded.end())
		{
			iteNeighbor->second->OnNeighborLoad(*chunk);
			chunk->OnNeighborLoad(*iteNeighbor->second);
		}
	}

	chunk->m_state = ChunkState::LOADED;
}

ChunkPopulateJob::ChunkPopulateJob(ChunkProvider* provider, Chunk* chunk) : Job(JOB_TYPE_GEN_CHUNK)
	, m_chunk(chunk)
	, m_chunkProvider(provider)
{
}

void ChunkPopulateJob::Execute()
{
	m_chunk->m_state = ChunkState::GENERATING;
	m_chunkProvider->PopulateChunk(m_chunk);
	m_chunk->m_state = ChunkState::GENERATED;
}

void ChunkPopulateJob::OnFinished()
{
	m_chunkProvider->m_chunksGenerating.erase(m_chunk->m_chunkCoords);
	m_chunkProvider->FinishUpChunkLoading(m_chunk);
}
