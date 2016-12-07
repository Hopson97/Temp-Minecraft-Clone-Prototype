#include "Player.h"

#include <SFML/Graphics.hpp>

#include "../Maths/General_Maths.h"
#include "../World/Block/D_Blocks.h"
#include "../World/Chunk/Chunk_Map.h"

#include "../Util/Debug_Display.h"
#include "../Util/Display.h"

Player::Player()
:   m_p_heldBlock (&Block::grass)
{
    m_camera.position = {20000, 250, 20000};
    Debug_Display::addheldBlock (*m_p_heldBlock);
}


/**

*/
const Entity& Player::getCamera() const
{
    return m_camera;
}


/**

*/
void Player::setPosition(const Vector3& position)
{
    m_camera.position = position;
}


/**

*/
void Player::input (const sf::Event& e)
{
    if (e.type == sf::Event::KeyPressed)
    {
        if (e.key.code == sf::Keyboard::Left)
            changeBlock(-1);
        if (e.key.code == sf::Keyboard::Right)
            changeBlock(1);
    }
}


/**

*/
void Player::input()
{
    translationInput ();
    rotationInput();
}


/**

*/
void Player::update(float dt, Entity& camera, Chunk_Map& chunkMap)
{
    if (!m_isOnGround)
        m_velocity.y -= 10 * dt;
    //m_isOnGround = false;

    collision(chunkMap, dt);
    m_camera.position += m_velocity * dt;

    m_velocity *= 0.95;
    camera = m_camera;

    Debug_Display::addLookVector(camera.rotation);
    Debug_Display::addheldBlock (*m_p_heldBlock);
}


/**

*/
const Block_t& Player::getBlock() const
{
    return *m_p_heldBlock;
}


/**

*/
void Player::translationInput()
{
    Vector3 change;
    auto yaw = glm::radians (m_camera.rotation.y);

    walkingInput(change, yaw);
    upDownInput(change);

    m_velocity += change;
}


/**

*/
void Player::walkingInput(Vector3& change, float yaw)
{
    //Speed variables
    float acc;
    sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ?
        acc = SPEED * 10 :
        acc = SPEED;

    if  (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
    {
        change.x -= cos (yaw + Maths::PI / 2) * acc;
        change.z -= sin (yaw + Maths::PI / 2) * acc;
    }
    if  (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
    {
        change.x += cos (yaw + Maths::PI / 2) * acc;
        change.z += sin (yaw + Maths::PI / 2) * acc;
    }
    if  (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
    {
        change.x += cos (yaw) * acc;
        change.z += sin (yaw) * acc;
    }
    if  (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
    {
        change.x -= cos (yaw) * acc;
        change.z -= sin (yaw) * acc;
    }
}


/**

*/
void Player::upDownInput(Vector3& change)
{
    //Speed variables
    float acc;
    sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ?
        acc = SPEED * 10 :
        acc = SPEED;

    //Up/ Down
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
    {
        change.y -= acc;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && m_isOnGround)
    {
        change.y += acc;
        //m_isOnGround = false;
    }
}


/**

*/
void Player::rotationInput()
{
    static sf::Vector2i lastMousePos;
    auto mouseMove = lastMousePos - sf::Mouse::getPosition();

    m_camera.rotation.y -= (float)mouseMove.x / 1.5f;// / 0.9;
    m_camera.rotation.x -= (float)mouseMove.y / 1.5f;// / 0.9;

    if (m_camera.rotation.x > 80 )
        m_camera.rotation.x = 80;
    else if (m_camera.rotation.x < -80)
        m_camera.rotation.x = -80;

    if (m_camera.rotation.y < 0 )
        m_camera.rotation.y = 360;
    else if (m_camera.rotation.y > 360)
        m_camera.rotation.y = 0;

    auto windowSize = Display::get().getSize();
    sf::Mouse::setPosition(sf::Vector2i(windowSize.x / 2, windowSize.y / 2), Display::get());

    lastMousePos = sf::Mouse::getPosition();
}


/**

*/
void Player::changeBlock(int increment)
{
    constexpr static auto NUM_BLOCK_TYPES = static_cast<int>(Block::ID::NUM_BLOCK_TYPES);

    auto currId = static_cast<int>(m_p_heldBlock->getID());
    currId += increment;

    //Seeing as "0" is an air block, we just skip over it
    if (currId == 0) currId = NUM_BLOCK_TYPES - 1;
    else if (currId == NUM_BLOCK_TYPES) currId = 1;

    auto* newBlock = &Block::getBlockFromId(static_cast<Block::ID>(currId));

    //We don't want to place liquid and gas as blocks, so skip.
    if (newBlock->getPhysicalState() == Block::Physical_State::Liquid ||
        newBlock->getPhysicalState() == Block::Physical_State::Gas)
    {
        currId += increment;
    }

    m_p_heldBlock = &Block::getBlockFromId(static_cast<Block::ID>(currId));
}

double  width = 0.3;
double  height = 1.0;


/**

*/
void Player::collision(Chunk_Map& chunkMap, float dt)
{
    /*
    auto pos = m_camera.position + m_velocity * dt;

    for (double  x = pos.x - width ; x < pos.x + width ; x += width)
    {
        for (double  y = pos.y - height ; y < pos.y ; y += 0.4)
        {
            for (double  z = pos.z - width ; z < pos.z + width ; z += width)
            {
                if (chunkMap.isSolidBlockAt({x, y, z}))
                {
                    //if(m_velocity.x > 0)
                    {
                        m_camera.position.x = x - width;
                        m_velocity.x = 0;
                    }
                }
            }
        }
    }*/
}


/*
  void collision(float Dx,float Dy,float Dz)
  {
    for (int X=(x-w)/size;X<(x+w)/size;X++)
    for (int Y=(y-h)/size;Y<(y+h)/size;Y++)
    for (int Z=(z-d)/size;Z<(z+d)/size;Z++)
      if (check(X,Y,Z))  {
        if (Dx>0)  x = X*size-w;
        if (Dx<0)  x = X*size+size+w;
		if (Dy>0)  y = Y*size-h;
        if (Dy<0) {y = Y*size+size+h; onGround=true; dy=0;}
		if (Dz>0)  z = Z*size-d;
        if (Dz<0)  z = Z*size+size+d;
                          }
  }
*/











