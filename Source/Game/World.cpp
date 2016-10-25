#include "World.h"

#include <thread>

#include "Shader_Loader.h"

#include "Display.h"
#include "Loader.h"
#include "Application.h"
#include "Height_Generator.h"
#include "Random.h"
#include "Maths/Position_Converter_Maths.h"

#include <iostream>

World::World()
:   m_blockAtlas    ( 512, 16, "Blocks_det" )
,   m_player        ( m_chunks )
{
    sf::Clock clock;

    addChunk( { 0, 0 } );
    m_chunks.at( { 0, 0 } )->generateMesh();
    m_chunks.at( { 0, 0 } )->bufferMesh();

    std::thread ( &World::manageChunks, this ).detach();
}

World :: ~World ()
{
    m_isRunning = false;
    std::this_thread::sleep_for( std::chrono::seconds ( 2 ) );
}

sf::Clock c;
void World :: update ( float dt )
{
    /*
    struct Ray
    {
        float x = 0;
        float y = 0;
        float z = 0;
    } ray;

    for ( int distance = 0; distance < 5 ; distance++ )
    {

    }

*/

    deleteChunks();
    m_player.update( dt );
    updateChunks();
}

void World :: draw ()
{
    int calls = 0;
    for ( auto itr = m_chunks.begin() ; itr != m_chunks.end() ; )
    {
        if ( itr->second->hasBuffered() )
        {

            m_renderer.processChunk( *itr->second );
            itr++;
            calls++;
        }
        else if ( itr->second->hasVertexData())
        {
            itr->second->bufferMesh();
        }
        else
        {
            itr++;
        }

    }
    m_renderer.render( m_player.getCamera(), Maths::worldToChunkPosition( m_player.getPosition() ) );
}

void World :: deleteChunks   ()
{
    for ( auto itr = m_chunks.begin() ; itr != m_chunks.end() ; )
    {
        Chunk* currChunk = (*itr).second.get();
        if ( !currChunk ) continue;

        currChunk->shouldBeDeleted() ?
            itr = m_chunks.erase( itr ) :
            itr++;
    }
}

void World :: updateChunks ()
{
    for ( auto itr = m_updateChunks.begin() ; itr != m_updateChunks.end() ; )
    {
        (*itr)->generateMesh();
        itr = m_updateChunks.erase( itr );
    }
}



void World :: addChunk ( const Chunk_Location& location )
{
    if ( m_chunks.find( location) == m_chunks.end() )
    {
        m_chunks[ location ] = std::make_unique<Chunk> ( m_chunks, location, m_blockAtlas, *this );
    }
}


//This below is all ran on a seperate thread
struct RenderArea
{
    int minX;
    int minZ;

    int maxX;
    int maxZ;
};


void World :: manageChunks()
{
    while ( m_isRunning )
    {
        static RenderArea loadArea;
        static RenderArea deleteArea;

        Chunk_Location chunkLocation = Maths::worldToChunkPosition( m_player.getPosition() );

        loadArea.minX =  chunkLocation.x - m_loadDistance;
        loadArea.minZ =  chunkLocation.z - m_loadDistance;
        loadArea.maxX =  chunkLocation.x + m_loadDistance;
        loadArea.maxZ =  chunkLocation.z + m_loadDistance;

        deleteArea.minX =  chunkLocation.x - m_renderDistance;
        deleteArea.minZ =  chunkLocation.z - m_renderDistance;
        deleteArea.maxX =  chunkLocation.x + m_renderDistance;
        deleteArea.maxZ =  chunkLocation.z + m_renderDistance;

        generateChunks( loadArea );

        if ( m_loadDistance < m_renderDistance )
        {
            m_loadDistance++;
        }
        else if ( m_loadDistance > m_renderDistance )
        {
            m_loadDistance = m_renderDistance;
        }

        if ( !m_isRunning ) return;

        checkChunks( deleteArea );
    }
}

void World :: generateChunks ( const RenderArea& area )
{
    std::vector<std::unique_ptr<std::thread>> threads;

    for ( int x = area.minX ; x < area.maxX ; x++ )
    {
        for ( int z = area.minZ ; z < area.maxZ ; z++ )
        {
            if ( !m_isRunning ) return; //Safety
            if ( m_chunks.find( { x, z } ) == m_chunks.end() )
            {
                threads.emplace_back( std::make_unique<std::thread>(&World::addChunk, this, Chunk_Location(x, z) ) );
            }
        }
        for ( auto& thread : threads ) thread->join();
        threads.clear();
    }
}

void World::checkChunks( const RenderArea& area )
{
    for ( auto& chunkPair : m_chunks )
    {
        if ( !m_isRunning ) return; //Safety

        Chunk& chunk = *chunkPair.second;
        if ( chunk.shouldBeDeleted() ) continue;

        Chunk_Location loc = chunk.getLocation();
        if ( loc.x < area.minX ||
             loc.x > area.maxX ||
             loc.z < area.minZ ||
             loc.z > area.maxZ )
        {
            chunk.setToDelete();
        }
        else
        {
            if ( !chunk.hasVertexData() && chunk.hasBlockData() )
            {
                chunk.generateMesh();
            }
        }
    }
}

//Setting a block
/*
    if (c.getElapsedTime().asSeconds() > 1.0f )
    {
        Chunk_Location chunkLocation = Maths::worldToChunkLocation  ( m_player.getPosition() );
        Vector3  blockLocation = Maths::worldToBlockInChunkPos( m_player.getPosition() );

        if ( m_chunks.find( chunkLocation ) != m_chunks.end() )
        {
            m_chunks.at( chunkLocation )->setBlock (    blockLocation.x,
                                                        blockLocation.y,
                                                        blockLocation.z,
                                                        Block::ID::Sand,
                                                        true );

            m_updateChunks.push_back( m_chunks.at( chunkLocation ).get() );
        }
        c.restart();
    }
*/
