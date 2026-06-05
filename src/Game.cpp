#include "Game.hpp"
#include <iostream> // Para imprimir en consola

Game::Game() : m_window(sf::VideoMode(1280, 720), "Fruit Pool - Fase 2") {
    m_window.setFramerateLimit(60);
    initPhysics();
}

Game::~Game() {
    b2DestroyWorld(m_worldId); // Liberamos la memoria física al cerrar
}

void Game::initPhysics() {
    // 1. Crear el mundo
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 0.0f}; // Sin gravedad porque es vista superior
    m_worldId = b2CreateWorld(&worldDef);

    // 2. Definir el Coco (Bola blanca dinámica)
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {400.0f / SCALE, 360.0f / SCALE}; 
    bodyDef.linearDamping = 1.2f; // Fricción del tapete
    bodyDef.angularDamping = 1.0f;

    m_cueBallId = b2CreateBody(m_worldId, &bodyDef);

    // 3. Darle forma y peso
    b2Circle dynamicCircle;
    dynamicCircle.center = {0.0f, 0.0f};
    dynamicCircle.radius = 15.0f / SCALE; 

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f; 
    shapeDef.friction = 0.2f;
    shapeDef.restitution = 0.8f; // Rebote

    b2CreateCircleShape(m_cueBallId, &shapeDef, &dynamicCircle);
}

void Game::run() {
    while (m_window.isOpen()) {
        processEvents();
        update();
        render();
    }
}

void Game::processEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_window.close();
        }
    }
}

void Game::update() {
    // Calculamos el siguiente frame físico
    b2World_Step(m_worldId, 1.0f / 60.0f, 4);
}

void Game::render() {
    // Limpiamos la ventana (color azul de mesa)
    m_window.clear(sf::Color(20, 80, 150));
    
    // Obtenemos la posición física
    b2Vec2 pos = b2Body_GetPosition(m_cueBallId);
    
    // Convertimos a píxeles
    float pixelX = pos.x * SCALE;
    float pixelY = pos.y * SCALE;

    // Imprimimos coordenadas en la terminal
    std::cout << "Posicion del Coco: X=" << pixelX << ", Y=" << pixelY << std::endl;
    
    // Dibujamos el círculo en la pantalla
    sf::CircleShape visualBall(15.0f);
    visualBall.setFillColor(sf::Color::White);
    visualBall.setOrigin(15.0f, 15.0f); 
    visualBall.setPosition(pixelX, pixelY);
    
    m_window.draw(visualBall);
    m_window.display();
}