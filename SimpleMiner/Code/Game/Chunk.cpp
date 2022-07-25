#include "Chunk.hpp"

#include "Game/BlockDef.hpp"
#include "Game/BlockMaterialDef.hpp"
#include "Game/BlockSetDefinition.hpp"
#include "Game/World.hpp"
#include "Game/ChunkProvider.hpp"

#include "Engine/Core/ByteBuffer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/DebugRender.hpp"

Chunk::Chunk(World* world, const ChunkCoords& chunkCoords)
	: m_world(world)
	, m_chunkCoords(chunkCoords)
{
	m_blockArray = new Block[CHUNK_SIZE_BLOCKS];
}

Chunk::~Chunk()
{
	delete m_opaqueBuffer;
	delete m_opaqueBufferIdx;
	delete m_fluidBuffer;
	delete m_fluidBufferIdx;

	delete m_blockArray;
}

void Chunk::Update()
{
	if (m_meshDirty)
	{
		for (auto& neighbor : m_neighbors)
			if (!neighbor)
				return;
		if (m_world->GetChunkManager()->GetRebuildMeshTicket())
			RebuildMesh();
	}
}

void Chunk::UpdateRandomTick(size_t rndSize, const float* rndSource, size_t offset)
{
	if (rndSource[offset] > 0.25f)
		return;

	size_t rndIdx = offset;

	for (int i = 0; i < CHUNK_SIZE_BLOCKS; i++)
	{
		if (m_blockArray[i].GetBlockId() == Blocks::BLOCK_GRASS && rndSource[rndIdx++ % rndSize] < 0.01f)
		{
			BlockIterator ite(this, i);
			Block* up = ite.GetBlockNeighborUp().GetBlock();
			if (up->IsValid() && up->GetBlockId() == Blocks::BLOCK_AIR)
			{
				BlockFace face;
				float randNum = rndSource[rndIdx++ % rndSize];
				if (randNum < 0.25f)
					face = BLOCK_FACE_NORTH;
				else if (randNum < 0.5f)
					face = BLOCK_FACE_SOUTH;
				else if (randNum < 0.75f)
					face = BLOCK_FACE_WEST;
				else
					face = BLOCK_FACE_EAST;

				BlockIterator sideIte = ite.GetBlockNeighbor(face);
				Block* side = sideIte.GetBlock();
				if (side->IsValid() && side->GetBlockId() == Blocks::BLOCK_DIRT)
				{
					up = sideIte.GetBlockNeighborUp().GetBlock();
					if (up->IsValid() && up->GetBlockId() == Blocks::BLOCK_AIR)
					{
						if (up->GetOutdoorLightInfluence() >= 6 || up->GetIndoorLightInfluence() >= 6)
							sideIte.GetChunk()->SetBlockId(sideIte.GetLocalCoords(), Blocks::BLOCK_GRASS);
					}
				}
			}
		}
	}
}

void Chunk::Render(int pass) const
{

	if (pass == RENDER_PASS_OPAQUE && m_opaqueBuffer)
	{
		size_t meshIndexCount = m_opaqueMesh.Count() / 4 * 6;
		g_theRenderer->DrawIndexedVertexBuffer(m_opaqueBufferIdx, m_opaqueBuffer, (int)meshIndexCount);

		WorldCoords coords = GetChunkOrigin();
		// DebugAddWorldLine(Vec3(coords.x                , coords.y                , 0), Vec3(coords.x                , coords.y                , CHUNK_SIZE_Z), 0.01f, 0.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::XRAY);
		// DebugAddWorldLine(Vec3(coords.x + CHUNK_SIZE_XY, coords.y                , 0), Vec3(coords.x + CHUNK_SIZE_XY, coords.y                , CHUNK_SIZE_Z), 0.01f, 0.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::XRAY);
		// DebugAddWorldLine(Vec3(coords.x + CHUNK_SIZE_XY, coords.y + CHUNK_SIZE_XY, 0), Vec3(coords.x + CHUNK_SIZE_XY, coords.y + CHUNK_SIZE_XY, CHUNK_SIZE_Z), 0.01f, 0.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::XRAY);
		// DebugAddWorldLine(Vec3(coords.x                , coords.y + CHUNK_SIZE_XY, 0), Vec3(coords.x                , coords.y + CHUNK_SIZE_XY, CHUNK_SIZE_Z), 0.01f, 0.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::XRAY);
	}

	if (pass == RENDER_PASS_FLUID && m_fluidBuffer)
	{
		size_t meshIndexCount = m_fluidMesh.Count() / 4 * 6;
		g_theRenderer->DrawIndexedVertexBuffer(m_fluidBufferIdx, m_fluidBuffer, (int)meshIndexCount);
	}
}

void Chunk::RebuildMesh()
{
	RebuildOpaqueMesh();
	RebuildTranslucentMesh();
	m_meshDirty = false;
}

void Chunk::MarkDirty()
{
	m_meshDirty = true;
	m_blocksDirty = true;
}

void Chunk::PopulateSkyLight()
{
	LocalCoords coords = IntVec3::ZERO;
	BlockIterator ite = BlockIterator(this, CHUNK_SIZE_BLOCKS);
	for (coords.z = CHUNK_MAX_Z; coords.z >= 0; coords.z--)
		for (coords.y = CHUNK_MAX_Y; coords.y >= 0; coords.y--)
			for (coords.x = CHUNK_MAX_X; coords.x >= 0; coords.x--)
			{
				ite--;
				Block* blk = ite.GetBlock();
				if (blk->IsSolid())
					continue;
				
				if (coords.z == CHUNK_MAX_Z || ite.GetBlockNeighbor(BLOCK_FACE_UP).GetBlock()->IsSky())
				{
					blk->SetSky(true);
					blk->SetOutdoorLightInfluence(15);
				}
			}

	ite = BlockIterator(this, CHUNK_SIZE_BLOCKS);
	for (coords.z = CHUNK_MAX_Z; coords.z >= 0; coords.z--)
		for (coords.y = CHUNK_MAX_Y; coords.y >= 0; coords.y--)
			for (coords.x = CHUNK_MAX_X; coords.x >= 0; coords.x--)
			{
				ite--;
				Block* blk = ite.GetBlock();
				if (blk->GetGlowLight())
				{
					m_world->GetChunkManager()->MarkLightingDirty(ite);
					continue;
				}

				if (!blk->IsSky())
					continue;

				for (BlockFace face : CHUNK_NEIGHBORS)
				{
					BlockIterator iteNbr = ite.GetBlockNeighbor(face);

					Block* block = iteNbr.GetBlock();
					if (block->IsValid() && !block->IsSky() && !block->IsSolid())
					{
						m_world->GetChunkManager()->MarkLightingDirty(iteNbr);
					}
				}
			}
}

WorldCoords Chunk::GetChunkOrigin() const
{
	return Chunk::GetOriginInWorld(m_chunkCoords);
}

WorldCoords Chunk::GetWorldCoords(const LocalCoords& localCoords) const
{
	return GetWorldCoords(m_chunkCoords, localCoords);
}

BlockId Chunk::GetBlockId(const LocalCoords& localCoords) const
{
	return GetBlock(localCoords).GetBlockId();
}

const Block& Chunk::GetBlock(const LocalCoords& localCoords) const
{
	if (localCoords.x < 0 || localCoords.x >= CHUNK_SIZE_XY)
		return Block::INVALID;
	if (localCoords.y < 0 || localCoords.y >= CHUNK_SIZE_XY)
		return Block::INVALID;
	if (localCoords.z < 0 || localCoords.z >= CHUNK_SIZE_Z)
		return Block::INVALID;

	int index = GetIndex(localCoords);
	return m_blockArray[index];
}

Block& Chunk::GetBlock(const LocalCoords& localCoords)
{
	if (localCoords.x < 0 || localCoords.x >= CHUNK_SIZE_XY)
	{
		return Block::INVALID;
	}
	if (localCoords.y < 0 || localCoords.y >= CHUNK_SIZE_XY)
	{
		return Block::INVALID;
	}
	if (localCoords.z < 0 || localCoords.z >= CHUNK_SIZE_Z)
	{
		return Block::INVALID;
	}

	int index = GetIndex(localCoords);
	return m_blockArray[index];
}

void Chunk::SetBlockId(const LocalCoords& localCoords, BlockId block)
{
	if (localCoords.x < 0 || localCoords.x >= CHUNK_SIZE_XY)
	{
		ERROR_RECOVERABLE("Invalid local coords");
		return;
	}
	if (localCoords.y < 0 || localCoords.y >= CHUNK_SIZE_XY)
	{
		ERROR_RECOVERABLE("Invalid local coords");
		return;
	}
	if (localCoords.z < 0 || localCoords.z >= CHUNK_SIZE_Z)
	{
		ERROR_RECOVERABLE("Invalid local coords");
		return;
	}

	int index = GetIndex(localCoords);

	bool wasOpaque = m_blockArray[index].IsOpaque();

	m_blockArray[index].SetBlockId(block);

	MarkDirty();

	if (wasOpaque != m_blockArray[index].IsOpaque() && wasOpaque) // change from opaque to transparent, neighbor might need to build a face
	{
		BlockIterator ite(this, index);
		for (BlockFace face : CHUNK_NEIGHBORS)
		{
			BlockIterator nbr = ite.GetBlockNeighbor(face);
			if (nbr.IsValid() && nbr.GetBlock()->IsOpaque()) // build only if a face of neighbor is exposed
				nbr.GetChunk()->m_meshDirty = true;
		}
	}

	m_world->GetChunkManager()->MarkLightingDirty(BlockIterator(this, index));
	if (!m_blockArray[index].IsOpaque())
	{
		// transparent (air)
		BlockIterator ite = BlockIterator(this, index);
		BlockIterator iteUp = ite.GetBlockNeighbor(BLOCK_FACE_UP);
		Block* blk = ite.GetBlock();
		Block* blkUp = iteUp.GetBlock();
		if (blkUp->IsValid() && blkUp->IsSky())
		{
			blk->SetSky(true);
			BlockIterator iteDown = ite.GetBlockNeighbor(BLOCK_FACE_DOWN);
			Block* blkDown = iteDown.GetBlock();
			while (blkDown->IsValid() && !blkDown->IsOpaque())
			{
				blkDown->SetSky(true);
				m_world->GetChunkManager()->MarkLightingDirty(iteDown);
				iteDown = iteDown.GetBlockNeighbor(BLOCK_FACE_DOWN);
				blkDown = iteDown.GetBlock();
			}
		}
	}
	else
	{
		// opaque (block)
		BlockIterator ite = BlockIterator(this, index);
		Block* blk = ite.GetBlock();
		if (blk->IsSky())
		{
			blk->SetSky(false);
			BlockIterator iteDown = ite.GetBlockNeighbor(BLOCK_FACE_DOWN);
			Block* blkDown = iteDown.GetBlock();
			while (blkDown->IsValid() && blkDown->IsSky())
			{
				blkDown->SetSky(false);
				m_world->GetChunkManager()->MarkLightingDirty(iteDown);
				iteDown = iteDown.GetBlockNeighbor(BLOCK_FACE_DOWN);
			}
		}
	}

	if (localCoords.x == CHUNK_MAX_X)
	{
		Chunk* neighbor = m_neighbors[(int)BLOCK_FACE_NORTH];
		if (neighbor)
			neighbor->m_meshDirty = true;
	}
	if (localCoords.x == 0)
	{
		Chunk* neighbor = m_neighbors[(int)BLOCK_FACE_SOUTH];
		if (neighbor)
			neighbor->m_meshDirty = true;
	}
	if (localCoords.y == CHUNK_MAX_Y)
	{
		Chunk* neighbor = m_neighbors[(int)BLOCK_FACE_WEST];
		if (neighbor)
			neighbor->m_meshDirty = true;
	}
	if (localCoords.y == 0)
	{
		Chunk* neighbor = m_neighbors[(int)BLOCK_FACE_EAST];
		if (neighbor)
			neighbor->m_meshDirty = true;
	}
}

const Block& Chunk::FindBlockOnFace(const LocalCoords& coordsRef, BlockFace face) const
{
	LocalCoords coords = coordsRef;

	switch (face)
	{
	case BLOCK_FACE_UP:
	{
		if (coords.z == CHUNK_MAX_Z)
			return Block::INVALID;
		break;
	}
	case BLOCK_FACE_DOWN:
	{
		if (coords.z == 0)
			return Block::INVALID;
		break;
	}
	case BLOCK_FACE_NORTH:
	{
		if (coords.x == CHUNK_MAX_X)
		{
			Chunk* neighbor = m_neighbors[face];
			if (!neighbor)
				return Block::INVALID;
			coords.x = 0;
			return neighbor->GetBlock(coords);
		}
		break;
	}
	case BLOCK_FACE_SOUTH:
	{
		if (coords.x == 0)
		{
			Chunk* neighbor = m_neighbors[face];
			if (!neighbor)
				return Block::INVALID;
			coords.x = CHUNK_MAX_X;
			return neighbor->GetBlock(coords);
		}
		break;
	}
	case BLOCK_FACE_WEST:
	{
		if (coords.y == CHUNK_MAX_Y)
		{
			Chunk* neighbor = m_neighbors[face];
			if (!neighbor)
				return Block::INVALID;
			coords.y = 0;
			return neighbor->GetBlock(coords);
		}
		break;
	}
	case BLOCK_FACE_EAST:
	{
		if (coords.y == 0)
		{
			Chunk* neighbor = m_neighbors[face];
			if (!neighbor)
				return Block::INVALID;
			coords.y = CHUNK_MAX_Y;
			return neighbor->GetBlock(coords);
		}
		break;
	}
	}

	return GetBlock(coords + Block::GetOffsetByFace(face));
}

void Chunk::OnNeighborLoad(Chunk& neighbor)
{
	for (BlockFace face : CHUNK_NEIGHBORS)
	{
		if (m_chunkCoords + Block::GetOffset2ByFace(face) == neighbor.m_chunkCoords)
		{
			m_neighbors[face] = &neighbor;
			m_meshDirty = true;
			break;
		}
	}

	for (auto& nbr : m_neighbors)
		if (!nbr)
			return;

	// all neighbors are loaded, do some work
	PopulateSkyLight();
}

void Chunk::OnNeighborUnload(const Chunk& neighbor)
{
	for (BlockFace face : CHUNK_NEIGHBORS)
	{
		if (m_chunkCoords + Block::GetOffset2ByFace(face) == neighbor.m_chunkCoords)
		{
			m_neighbors[face] = nullptr;
			m_meshDirty = true;
			return;
		}
	}
	// not a neighbor, ignore
}

void Chunk::OnNeighborBlockUpdate(const Chunk& neighbor, const LocalCoords& coords)
{
	UNUSED(neighbor);
	UNUSED(coords);
	m_meshDirty = true;
// 	for (auto* pChunk : m_neighbors)
// 	{
// 		if (&neighbor == pChunk)
// 		{
// 			return;
// 		}
// 	}
// 	// not a neighbor, ignore
}



void Chunk::WriteBytes(ByteBuffer* buffer) const
{
	unsigned char rle_len = 1;
	unsigned char rle_last_char = m_blockArray[0].GetBlockId();
	for (size_t idx = 1; idx < CHUNK_SIZE_BLOCKS; idx++)
	{
		unsigned char rle_char = m_blockArray[idx].GetBlockId();
		if (rle_char != rle_last_char || rle_len == 255)
		{
			buffer->Write(rle_last_char);
			buffer->Write(rle_len);
			rle_len = 1;
			rle_last_char = rle_char;
		}
		else
		{
			rle_len++;
		}
	}
	buffer->Write(rle_last_char);
	buffer->Write(rle_len);
}

void Chunk::ReadBytes(ByteBuffer* buffer)
{
	size_t idx = 0;
	unsigned char rle_len;
	unsigned char rle_last_char;
	while (idx < CHUNK_SIZE_BLOCKS)
	{
		buffer->Read(rle_last_char);
		buffer->Read(rle_len);
		for (int i = 0; i < rle_len; i++)
			m_blockArray[idx + i].SetBlockId(rle_last_char);
		idx += rle_len;
	}
}

void Chunk::RebuildOpaqueMesh()
{
	Shader* shader = BlockSetDefinition::GetDefinition()->GetBlockMaterialAtlas()->m_shader;
	m_opaqueMesh.Start(shader->GetInputFormat(0), 65535);

	LocalCoords coords = IntVec3::ZERO;
	for (coords.z = 0; coords.z < CHUNK_SIZE_Z; coords.z++)
		for (coords.y = 0; coords.y < CHUNK_SIZE_XY; coords.y++)
			for (coords.x = 0; coords.x < CHUNK_SIZE_XY; coords.x++)
			{
				WorldCoords worldCoords = GetWorldCoords(coords);
				Vec3 worldPos;
				worldPos.x = (float)worldCoords.x;
				worldPos.y = (float)worldCoords.y;
				worldPos.z = (float)worldCoords.z;

				const Block& block = GetBlock(coords);
				const BlockDef* blockDef = block.GetBlockDef();
				if (!blockDef->m_opaque)
					continue;

				for (BlockFace face : BLOCK_NEIGHBORS)
				{
					const Block* neighborBlock = BlockIterator(this, coords).GetBlockNeighbor(face).GetBlock();
					if (neighborBlock->IsValid())
					{
						if (neighborBlock->GetBlockId() == block.GetBlockId())
							continue; // cull same block face
						if (neighborBlock->IsOpaque())
							continue; // cull opaque neighbor face
					}

					const BlockMaterialDef* material = blockDef->GetBlockMaterial(face);

					if (!material->m_visible)
						continue;

					unsigned char iLight /* indoor  */ = neighborBlock ? neighborBlock->GetIndoorLightInfluenceNormalized() : 15;
					unsigned char oLight /* outdoor */ = neighborBlock ? neighborBlock->GetOutdoorLightInfluenceNormalized() : 0;

					switch (face)
					{
					case BLOCK_FACE_NORTH:
					{
						unsigned char fLight /* face */ = 0xCD;
						m_opaqueMesh.begin()->pos(worldPos + Vec3(1.0f, 0.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_mins.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(1.0f, 1.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_mins.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(1.0f, 1.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_maxs.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(1.0f, 0.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_maxs.y)->end();
					}
					break;
					case BLOCK_FACE_SOUTH:
					{
						unsigned char fLight /* face */ = 0xCD;
						m_opaqueMesh.begin()->pos(worldPos + Vec3(0.0f, 1.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_mins.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(0.0f, 0.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_mins.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(0.0f, 0.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_maxs.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(0.0f, 1.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_maxs.y)->end();
					}
					break;
					case BLOCK_FACE_WEST:
					{
						unsigned char fLight /* face */ = 0xE6;
						m_opaqueMesh.begin()->pos(worldPos + Vec3(1.0f, 1.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_mins.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(0.0f, 1.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_mins.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(0.0f, 1.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_maxs.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(1.0f, 1.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_maxs.y)->end();
					}
					break;
					case BLOCK_FACE_EAST:
					{
						unsigned char fLight /* face */ = 0xE6;
						m_opaqueMesh.begin()->pos(worldPos + Vec3(0.0f, 0.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_mins.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(1.0f, 0.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_mins.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(1.0f, 0.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_maxs.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(0.0f, 0.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_maxs.y)->end();
					}
					break;
					case BLOCK_FACE_UP:
					{
						unsigned char fLight /* face */ = 0xFF;
						m_opaqueMesh.begin()->pos(worldPos + Vec3(0.0f, 1.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_mins.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(0.0f, 0.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_mins.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(1.0f, 0.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_maxs.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(1.0f, 1.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_maxs.y)->end();
					}
					break;
					case BLOCK_FACE_DOWN:
					{
						unsigned char fLight /* face */ = 0xFF;
						m_opaqueMesh.begin()->pos(worldPos + Vec3(1.0f, 1.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_mins.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(1.0f, 0.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_mins.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(0.0f, 0.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_maxs.y)->end();
						m_opaqueMesh.begin()->pos(worldPos + Vec3(0.0f, 1.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_maxs.y)->end();
					}
					break;
					}
				}
			}

	delete m_opaqueBuffer;
	delete m_opaqueBufferIdx;

	if (m_opaqueMesh.Count())
	{
		m_opaqueBuffer = g_theRenderer->CreateVertexBuffer(m_opaqueMesh.GetBufferSize(), &shader->GetInputFormat(0));
		m_opaqueMesh.Upload(g_theRenderer, m_opaqueBuffer);

		size_t meshIndexCount = m_opaqueMesh.Count() / 4 * 6;
		int* indexBuffer = new int[meshIndexCount];
		size_t indexIdx = 0;
		for (int index = 0; index < m_opaqueMesh.Count(); index += 4)
		{
			indexBuffer[indexIdx + 0] = index + 0;
			indexBuffer[indexIdx + 1] = index + 1;
			indexBuffer[indexIdx + 2] = index + 2;
			indexBuffer[indexIdx + 3] = index + 0;
			indexBuffer[indexIdx + 4] = index + 2;
			indexBuffer[indexIdx + 5] = index + 3;
			indexIdx += 6;
		}
		m_opaqueBufferIdx = g_theRenderer->CreateIndexBuffer(sizeof(int) * meshIndexCount);
		g_theRenderer->CopyCPUToGPU(indexBuffer, sizeof(int) * meshIndexCount, m_opaqueBufferIdx);
		delete[] indexBuffer;
	}
	else
	{
		m_opaqueBuffer = nullptr;
		m_opaqueBufferIdx = nullptr;
	}
}

void Chunk::RebuildTranslucentMesh()
{
	Shader* shader = BlockSetDefinition::GetDefinition()->GetBlockMaterialAtlas()->m_shader;
	m_fluidMesh.Start(shader->GetInputFormat(0), 65535);

	LocalCoords coords = IntVec3::ZERO;
	for (coords.z = 0; coords.z < CHUNK_SIZE_Z; coords.z++)
		for (coords.y = 0; coords.y < CHUNK_SIZE_XY; coords.y++)
			for (coords.x = 0; coords.x < CHUNK_SIZE_XY; coords.x++)
			{
				WorldCoords worldCoords = GetWorldCoords(coords);
				Vec3 worldPos;
				worldPos.x = (float)worldCoords.x;
				worldPos.y = (float)worldCoords.y;
				worldPos.z = (float)worldCoords.z;

				const Block& block = GetBlock(coords);
				if (block.GetBlockId() == Blocks::BLOCK_AIR)
					continue; // do not render air
				if (block.IsOpaque())
					continue; // do not render opaque

				bool isUpAir = false;
				const Block* upBlock = BlockIterator(this, coords).GetBlockNeighborUp().GetBlock();
				isUpAir = !upBlock->IsValid() || upBlock->GetBlockId() == Blocks::BLOCK_AIR;

				for (BlockFace face : BLOCK_NEIGHBORS)
				{
					const Block* neighborBlock = BlockIterator(this, coords).GetBlockNeighbor(face).GetBlock();
					if (neighborBlock->IsValid())
					{
						if (neighborBlock->GetBlockId() == block.GetBlockId())
							continue; // cull same block face
						if (neighborBlock->IsOpaque())
							continue; // cull opaque neighbor face
					}

					const BlockMaterialDef* material = block.GetBlockDef()->GetBlockMaterial(face);

					if (!material->m_visible)
						continue;

					unsigned char iLight /* indoor  */ = neighborBlock ? neighborBlock->GetIndoorLightInfluenceNormalized() : 15;
					unsigned char oLight /* outdoor */ = neighborBlock ? neighborBlock->GetOutdoorLightInfluenceNormalized() : 0;

					switch (face)
					{
					case BLOCK_FACE_NORTH:
					{
						unsigned char fLight /* face */ = 0xCD;
						m_fluidMesh.begin()->pos(worldPos + Vec3(1.0f, 0.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_mins.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(1.0f, 1.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_mins.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(1.0f, 1.0f, 1.0f))->color(iLight, oLight, isUpAir ? 0xff : fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_maxs.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(1.0f, 0.0f, 1.0f))->color(iLight, oLight, isUpAir ? 0xff : fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_maxs.y)->end();
					}
					break;
					case BLOCK_FACE_SOUTH:
					{
						unsigned char fLight /* face */ = 0xCD;
						m_fluidMesh.begin()->pos(worldPos + Vec3(0.0f, 1.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_mins.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(0.0f, 0.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_mins.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(0.0f, 0.0f, 1.0f))->color(iLight, oLight, isUpAir ? 0xff : fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_maxs.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(0.0f, 1.0f, 1.0f))->color(iLight, oLight, isUpAir ? 0xff : fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_maxs.y)->end();
					}
					break;
					case BLOCK_FACE_WEST:
					{
						unsigned char fLight /* face */ = 0xE6;
						m_fluidMesh.begin()->pos(worldPos + Vec3(1.0f, 1.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_mins.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(0.0f, 1.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_mins.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(0.0f, 1.0f, 1.0f))->color(iLight, oLight, isUpAir ? 0xff : fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_maxs.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(1.0f, 1.0f, 1.0f))->color(iLight, oLight, isUpAir ? 0xff : fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_maxs.y)->end();
					}
					break;
					case BLOCK_FACE_EAST:
					{
						unsigned char fLight /* face */ = 0xE6;
						m_fluidMesh.begin()->pos(worldPos + Vec3(0.0f, 0.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_mins.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(1.0f, 0.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_mins.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(1.0f, 0.0f, 1.0f))->color(iLight, oLight, isUpAir ? 0xff : fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_maxs.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(0.0f, 0.0f, 1.0f))->color(iLight, oLight, isUpAir ? 0xff : fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_maxs.y)->end();
					}
					break;
					case BLOCK_FACE_UP:
					{
						unsigned char fLight /* face */ = isUpAir ? 0xff : 0xF9;
						m_fluidMesh.begin()->pos(worldPos + Vec3(0.0f, 1.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_mins.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(0.0f, 0.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_mins.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(1.0f, 0.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_maxs.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(1.0f, 1.0f, 1.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_maxs.y)->end();
					}
					break;
					case BLOCK_FACE_DOWN:
					{
						unsigned char fLight /* face */ = 0xF9;
						m_fluidMesh.begin()->pos(worldPos + Vec3(1.0f, 1.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_mins.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(1.0f, 0.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_mins.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(0.0f, 0.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_maxs.x, material->m_uv.m_maxs.y)->end();
						m_fluidMesh.begin()->pos(worldPos + Vec3(0.0f, 1.0f, 0.0f))->color(iLight, oLight, fLight)->tex(material->m_uv.m_mins.x, material->m_uv.m_maxs.y)->end();
					}
					break;
					}
				}
			}

	delete m_fluidBuffer;
	delete m_fluidBufferIdx;

	if (m_fluidMesh.Count())
	{
		m_fluidBuffer = g_theRenderer->CreateVertexBuffer(m_fluidMesh.GetBufferSize(), &shader->GetInputFormat(0));
		m_fluidMesh.Upload(g_theRenderer, m_fluidBuffer);

		size_t meshIndexCount = m_fluidMesh.Count() / 4 * 6;
		int* indexBuffer = new int[meshIndexCount];
		size_t indexIdx = 0;
		for (int index = 0; index < m_fluidMesh.Count(); index += 4)
		{
			indexBuffer[indexIdx + 0] = index + 0;
			indexBuffer[indexIdx + 1] = index + 1;
			indexBuffer[indexIdx + 2] = index + 2;
			indexBuffer[indexIdx + 3] = index + 0;
			indexBuffer[indexIdx + 4] = index + 2;
			indexBuffer[indexIdx + 5] = index + 3;
			indexIdx += 6;
		}
		m_fluidBufferIdx = g_theRenderer->CreateIndexBuffer(sizeof(int) * meshIndexCount);
		g_theRenderer->CopyCPUToGPU(indexBuffer, sizeof(int) * meshIndexCount, m_fluidBufferIdx);
		delete[] indexBuffer;
	}
	else
	{
		m_fluidBuffer = nullptr;
		m_fluidBufferIdx = nullptr;
	}
}
