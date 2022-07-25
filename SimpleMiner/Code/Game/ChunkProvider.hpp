#include "Game/BlockIterator.hpp"
#include "Game/Chunk.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/Stopwatch.hpp"

#include <map>
#include <deque>

class World;
class WorldGenerator;

constexpr int JOB_TYPE_GEN_CHUNK = 999;

enum class ChunkLoadStatus
{
	PRESENT,
	LOADED,
	QUEUED,
	FAILED,
};

class ChunkProvider;

class ChunkPopulateJob : public Job
{
public:
	ChunkPopulateJob(ChunkProvider* provider, Chunk* chunk);

private:
	virtual void Execute() override;
	virtual void OnFinished() override;

private:
	Chunk* const                    m_chunk;
	ChunkProvider* const            m_chunkProvider;
};

class ChunkProvider
{
	friend class ChunkPopulateJob;

public:
	ChunkProvider(World* world, const char* folderPath, WorldGenerator* generator);
	~ChunkProvider();

	void Update();

	// chunk management
	std::map<ChunkCoords, Chunk*>& GetLoadedChunks();
	const std::map<ChunkCoords, Chunk*>& GetLoadedChunks() const;
	bool LoadChunkWithTicket(const ChunkCoords& coords);
	ChunkLoadStatus LoadChunk(const ChunkCoords& coords);
	void UnloadChunk(const ChunkCoords& coords);
	Chunk* FindLoadedChunk(const ChunkCoords& coords) const;
	void UnloadAllChunks();

	// block access
	const Block& GetBlock(const WorldCoords& worldCoords) const;
	BlockId GetBlockId(const WorldCoords& worldCoords) const;
	void    SetBlockId(const WorldCoords& worldCoords, BlockId block);

	// lighting
	void ProcessDirtyLighting();
	void ProcessNextDirtyLightBlock();
	void MarkLightingDirty(const BlockIterator& blockIte);
	void MarkLightingDirty(const WorldCoords& worldCoords);
	void UndirtyAllBlocksInChunk(const ChunkCoords& chunkCoords);

	// utils
	int  GetChunkActiveRange() const { return m_chunkActivationRange; }
	bool GetChunkIOTicket();
	bool GetRebuildMeshTicket();
	void SetHotspotSize(int size);
	void SetHotspot(int index, const Vec3& worldPos);

	// tick
	void BeginFrame();
	void EndFrame();

	// file utils
	std::string GetFileName(const ChunkCoords& coords) const;


private:
	void UpdateRandomTick();

	void DoChunkDeactivation();
	void DoChunkActivation();

	bool LoadChunkFromDisk(Chunk* chunk) const;
	void PopulateChunk(Chunk* chunk);
	bool SaveChunkToDisk(const Chunk* chunk) const;

	void FinishUpChunkLoading(Chunk* chunk);

private:
	World* m_world = nullptr;
	std::string m_path;
	bool m_disableLoadFromDisk = false;
	unsigned int m_worldSeed = 781031139;
	int m_chunkActivationRange = 250;
	WorldGenerator* m_generator = nullptr;
	std::map<ChunkCoords, Chunk*> m_chunksLoaded;
	std::map<ChunkCoords, Chunk*> m_chunksGenerating;
	int m_rebuildMeshTicket = 0;
	int m_chunkIOTicket = 0;
	std::vector<ChunkCoords> m_hotspots;
	std::deque<BlockIterator> m_dirtyLighting;
	std::deque<BlockIterator> m_debugLighting;
	Stopwatch m_rndTickWatch;
};

