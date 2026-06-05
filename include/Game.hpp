#pragma once
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

class Game {
public:
    Game();
    ~Game(); // Importante: liberar la memoria del mundo físico
    void run();

private:
    void processEvents();
    void update();
    void render();
    void initPhysics();

    sf::RenderWindow m_window;
    
    // En Box2D v3, usamos "IDs" en lugar de punteros a clases
    b2WorldId m_worldId;
    b2BodyId m_cueBallId; 

    // Constante de conversión: 30 píxeles equivalen a 1 metro físico
    const float SCALE = 30.0f;
};