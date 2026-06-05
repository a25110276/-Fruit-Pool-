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
    // NUEVO: Creamos las 4 bandas de la mesa de billar (Arriba, Abajo, Izquierda, Derecha)
    // Coordenadas: Centro X, Centro Y, Ancho total, Alto total
    createWall(640.0f, 0.0f, 1280.0f, 20.0f);   // Pared Superior
    createWall(640.0f, 720.0f, 1280.0f, 20.0f); // Pared Inferior
    createWall(0.0f, 360.0f, 20.0f, 720.0f);    // Pared Izquierda
    createWall(1280.0f, 360.0f, 20.0f, 720.0f); // Pared Derecha
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
        // Evento de cerrar la ventana
        if (event.type == sf::Event::Closed) {
            m_window.close();
        }

        // Evento: Jugador hace Click Izquierdo (Empieza a apuntar/cargar fuerza)
        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                m_isAiming = true;
                // Guardamos el punto exacto donde hizo click
                m_mouseStartPos = sf::Vector2f(event.mouseButton.x, event.mouseButton.y);
            }
        }

        // Evento: Jugador suelta el Click Izquierdo (Golpea el Coco)
        if (event.type == sf::Event::MouseButtonReleased) {
            if (event.mouseButton.button == sf::Mouse::Left && m_isAiming) {
                m_isAiming = false;
                sf::Vector2f mouseEndPos(event.mouseButton.x, event.mouseButton.y);

                // Calculamos la diferencia entre donde inició el click y donde lo soltó.
                // Al restar (Start - End) logramos que jalar hacia "atrás" dispare hacia "adelante".
                float deltaX = m_mouseStartPos.x - mouseEndPos.x;
                float deltaY = m_mouseStartPos.y - mouseEndPos.y;

                // Creamos un vector de fuerza. Multiplicamos por 0.05f para suavizar la magnitud.
                b2Vec2 impulse = {deltaX * 0.05f, deltaY * 0.05f};
                
                // Obtenemos la posición actual de la bola
                b2Vec2 pos = b2Body_GetPosition(m_cueBallId);
                
                // Aplicamos el impulso a la bola en Box2D (true es para "despertar" el cuerpo)
                b2Body_ApplyLinearImpulse(m_cueBallId, impulse, pos, true);
            }
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

    // NUEVO: Dibujar la línea de dirección si el jugador está apuntando
    if (m_isAiming) {
        // Obtenemos la posición actual del mouse en tiempo real
        sf::Vector2i currentMousePos = sf::Mouse::getPosition(m_window);
        
        // Calculamos el vector de proyección visual
        float projX = pixelX + (m_mouseStartPos.x - currentMousePos.x);
        float projY = pixelY + (m_mouseStartPos.y - currentMousePos.y);

        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(pixelX, pixelY), sf::Color::White), // Inicio (Centro del coco)
            sf::Vertex(sf::Vector2f(projX, projY), sf::Color::Red)      // Fin (Proyección del golpe)
        };
        m_window.draw(line, 2, sf::Lines);
     
    }
       m_window.display();
}

void Game::createWall(float x, float y, float width, float height) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody; // Estático: inamovible como una pared
    // Box2D ubica los rectángulos desde su centro, por eso dividimos entre SCALE
    bodyDef.position = {x / SCALE, y / SCALE}; 
    
    b2BodyId wallId = b2CreateBody(m_worldId, &bodyDef);

    // Box2D requiere "half-widths" (la mitad del ancho y alto) para crear cajas
    b2Polygon box = b2MakeBox((width / 2.0f) / SCALE, (height / 2.0f) / SCALE);
    
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.friction = 0.2f;
    shapeDef.restitution = 0.6f; // Esto simula el rebote elástico de las bandas de billar

    b2CreatePolygonShape(wallId, &shapeDef, &box);
}