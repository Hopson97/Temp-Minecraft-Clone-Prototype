#include "World.h"

#include <iostream>
#include <SFML/Graphics.hpp>

#include "../Renderer/RMaster.h"
#include "../Player/Player.h"
#include "../Camera.h"
#include "../Maths/General_Maths.h"

World::World(const Camera& camera, int32_t worldSize)
:   m_p_camera      (&camera)
,   m_worldSize     (worldSize)
{
    for (int32_t x = 0 ; x < m_worldSize; x++)
    {
        for (int32_t z = 0; z < m_worldSize; z++)
        {
            m_chunks.addChunk({x, z}, *this);
        }
    }
}

void World::checkPlayerBounds(Player& player)
{
    if (player.position.x < 0.2)
    {
        player.position.x = 0.2;
    }
    if (player.position.z < 0.2)
    {
        player.position.z = 0.2;
    }

    if (player.position.x + 0.2 > m_worldSize * CHUNK_SIZE - 0.2 )
    {
        player.position.x = m_worldSize * CHUNK_SIZE - 0.3;
    }
    if (player.position.z + 0.2 > m_worldSize * CHUNK_SIZE - 0.2 )
    {
        player.position.z = m_worldSize * CHUNK_SIZE - 0.3;
    }
}

void World::setBlock(const Vector3& position, CBlock block)
{
    m_newBlocks.emplace_back(block, position);
}

CBlock World::getBlock(const Vector3& position)
{
    Chunk::Chunklet_Position    chunkPosition   = Maths::worldToChunkletPos(position);
    Block::Small_Position       blockPosition   = Maths::blockToSmallBlockPos(Maths::worldToBlockPos(position));

    Chunk::Section* chunk = m_chunks.get(chunkPosition);

    if (chunk)
        return chunk->qGetBlock(blockPosition);
    else
        return Block::ID::Air;
}


void World::regenerateChunks()
{
    std::unordered_map<Chunk::Chunklet_Position, Chunk::Section*> chunksToUpdate;

    //Duh, inserts a chunk into the map..
    auto insertChunk = [&](const Chunk::Chunklet_Position& chunkPosition,
                                 Chunk::Section* chunk)
    {
        Chunk::Full_Chunk* chunkFull = m_chunks.get({chunkPosition.x, chunkPosition.z});
        if(!chunkFull)
            return;

        while (!chunk)
        {
            chunkFull->addSection();
            chunk = chunkFull->getSection(chunkPosition.y);
        }
        chunksToUpdate.insert(std::make_pair(chunkPosition, chunk));
    };

    //Lambda checks if blocks are being set on chunk edges, and if so, adds the adjacent chunk to the update batch
    auto checkForBatchAdd = [&](int8_t position,
                                const Chunk::Chunklet_Position& chunkPosition,
                                const Vector3& direction)
    {
        if (position == 0)
        {
            Chunk::Chunklet_Position newChunkPosition(  chunkPosition.x - direction.x,
                                                        chunkPosition.y - direction.y,
                                                        chunkPosition.z - direction.z);

            insertChunk(newChunkPosition, m_chunks.get(newChunkPosition));
        }
        else if (position == CHUNK_SIZE - 1)
        {
            Chunk::Chunklet_Position newChunkPosition(  chunkPosition.x + direction.x,
                                                        chunkPosition.y + direction.y,
                                                        chunkPosition.z + direction.z);

            insertChunk(newChunkPosition, m_chunks.get(newChunkPosition));
        }
    };



    for (New_Block& newBlock : m_newBlocks)
    {
        Chunk::Chunklet_Position    chunkPosition   = Maths::worldToChunkletPos(newBlock.position);
        Block::Small_Position       blockPosition   = Maths::blockToSmallBlockPos(Maths::worldToBlockPos(newBlock.position));
        Chunk::Section*             chunk           = nullptr;
        Chunk::Full_Chunk*          chunkFull       = m_chunks.get({chunkPosition.x, chunkPosition.z});

        //Adds sections onto a chunk until it reaches the height of the block
        while (!chunk)
        {
            chunkFull->addSection();
            chunk = chunkFull->getSection(chunkPosition.y);
        }

        chunk->setBlock(blockPosition, newBlock.block);
        insertChunk(chunkPosition, chunk);
        checkForBatchAdd(blockPosition.x, chunkPosition, {1, 0, 0});
        checkForBatchAdd(blockPosition.y, chunkPosition, {0, 1, 0});
        checkForBatchAdd(blockPosition.z, chunkPosition, {0, 0, 1});
    }

    for (auto& chunk : chunksToUpdate)
    {
        Chunk::Section* sect = chunk.second;
        sect->makeMesh();
    }

    m_newBlocks.clear();
}

//Generates meshes for the chunks.
//It does this in a sort of radius starting from the middle of the world
void World::buffer(const Camera& camera)
{
    if (m_loadingDistance == ((m_worldSize / 2) + 1)) return;

    int minDis = m_worldSize / 2 - m_loadingDistance;
    int maxDis = m_worldSize / 2 + m_loadingDistance;

    bool isMeshMade = false;

    std::cout << m_loadingDistance << std::endl;

    for (int32_t x = minDis ; x < maxDis; x++)
    {
        for (int32_t z = minDis; z < maxDis; z++)
        {
            Chunk::Full_Chunk* chunk = m_chunks.get({x, z});
            if (chunk)
            {
                if(chunk->tryGen(*m_p_camera))
                {
                    isMeshMade = true;
                    break;
                }
            }
        }
        if (isMeshMade)
        {
            break;
        }
    }
    if (!isMeshMade)
    {
        m_loadingDistance++;
    }
}

void World::draw(Renderer::Master& renderer, const Camera& camera)
{
    m_facesDrawn = 0;
    for (auto& chunk : m_chunks.getChunks())
    {
        m_facesDrawn += chunk.second.draw(renderer, camera);
    }
}

void World::drawWorld(Renderer::Master& renderer, const Camera& camera)
{
    if (!m_newBlocks.empty())
        regenerateChunks();

    draw(renderer, camera);
    buffer(camera);
}
