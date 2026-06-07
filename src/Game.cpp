#include "Game.hpp"
#include <iostream> // Para imprimir en consola

Game::Game() : m_window(sf::VideoMode(1280, 720), "Fruit Pool - Fase 4") {
    m_window.setFramerateLimit(60);
    loadAssets();
    initPhysics();// 1. Inicializar el mundo físico de Box2D y sus bandas
    spawnTriangle(); // Llenamos la mesa con el triángulo de frutas al iniciar
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
    updateAnimation();
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
    
  m_window.clear();
m_window.draw(m_tableSprite); // Primero el fondo
m_window.draw(m_cocoSprite);  // Luego las frutas

    // Dibuja el resto de las frutas de la mesa
float SCALE = 30.0f;
for (b2BodyId id : m_fruitIds) {
    b2Vec2 pos = b2Body_GetPosition(id);
    m_cocoSprite.setPosition({pos.x * SCALE, pos.y * SCALE});
    m_cocoSprite.setColor(sf::Color(255, 100, 100)); // Teñirlas de rojo (Test visual)
    m_window.draw(m_cocoSprite);
}

// Resetear el color para que el Coco original (Bola blanca) se dibuje normal
m_cocoSprite.setColor(sf::Color::White);
   
// NUEVO: Apuntado avanzado con retroceso y láser grueso
if (m_isAiming) {
    // 1. Obtener la posición física central del Coco
    b2Vec2 pos = b2Body_GetPosition(m_cueBallId);
    float SCALE = 30.0f; 
    float pixelX = pos.x * SCALE;
    float pixelY = pos.y * SCALE;

    // 2. Calcular el vector de "jalón"
    sf::Vector2i currentMousePos = sf::Mouse::getPosition(m_window);
    float pullX = m_mouseStartPos.x - currentMousePos.x;
    float pullY = m_mouseStartPos.y - currentMousePos.y;

    // 3. Limitar el retroceso máximo de la caña (Clamp)
    float pullDist = std::sqrt(pullX * pullX + pullY * pullY);
    float maxPull = 120.0f; // Píxeles máximos que la caña se hará hacia atrás
    
    if (pullDist > maxPull) {
        pullX = (pullX / pullDist) * maxPull;
        pullY = (pullY / pullDist) * maxPull;
    }

    // 4. Dibujar la Guía de Tiro (Línea Gruesa Fija)
    // El ángulo de tiro va en dirección contraria al jalón del ratón
    float shotAngle = std::atan2(pullY, pullX) * 180.0f / 3.14159265f;

    sf::RectangleShape aimLine;
    aimLine.setSize({900.0f, 4.0f}); // 800px de largo fijo, 4px de grosor
    aimLine.setOrigin({0.0f, 2.0f}); // Centrar el grosor verticalmente
    aimLine.setPosition({pixelX, pixelY});
    aimLine.setRotation(shotAngle);
    aimLine.setFillColor(sf::Color(255, 255, 255, 120)); // Blanco semi-transparente
    
    m_window.draw(aimLine);

    // 5. Dibujar la Caña de Azúcar con Retroceso
    // El ángulo del taco está invertido respecto al tiro
    float stickAngle = std::atan2(-pullY, -pullX) * 180.0f / 3.14159265f;
    
    // Restamos pullX y pullY a la posición del Coco para mover el taco hacia atrás
    m_cueSprite.setPosition({pixelX - pullX, pixelY - pullY});
    m_cueSprite.setRotation(stickAngle);
    
    m_window.draw(m_cueSprite);
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

void Game::loadAssets() {
    // 1. Cargar el Mantel
    if (!m_tableTexture.loadFromFile("assets/images/mantel.jpg")) {
        // Manejo de error básico
    }
    m_tableSprite.setTexture(m_tableTexture);

    // 2. Cargar el Spritesheet del Coco
    if (!m_cocoTexture.loadFromFile("assets/images/coco.png")) {
        // Manejo de error básico
    }
    m_cocoSprite.setTexture(m_cocoTexture);

    // 3. Configurar el primer recorte (Frame 0)
    m_cocoSprite.setTextureRect(sf::IntRect({0, 0}, {FRAME_WIDTH, FRAME_HEIGHT}));
    
    // 4. Centrar el origen de la imagen en su centro exacto (50, 50)
    m_cocoSprite.setOrigin({FRAME_WIDTH / 2.0f, FRAME_HEIGHT / 2.0f});
    
    // 5. Escalar visualmente al tamaño del cuerpo físico de Box2D
    // Diámetro físico (30.0f) / Tamaño del frame en píxeles (100.0f) = Escala de 0.3f
    m_cocoSprite.setScale({30.0f / FRAME_WIDTH, 30.0f / FRAME_HEIGHT});

    // 6. Cargar el Taco de Billar
if (!m_cueTexture.loadFromFile("assets/images/taco.png")) {
    // Manejo de error básico
}
m_cueSprite.setTexture(m_cueTexture);

// Mover el punto de rotación al centro del borde izquierdo (la punta del taco)
float tacoHeight = static_cast<float>(m_cueTexture.getSize().y);
m_cueSprite.setOrigin({0.0f, tacoHeight / 2.0f});

// Opcional: Ajustar la escala si tu imagen del taco es muy grande
m_cueSprite.setScale({0.5f, 0.5f});


}

void Game::updateAnimation() {
    b2Vec2 velocity = b2Body_GetLinearVelocity(m_cueBallId);
    float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

    if (speed > 0.5f) {
        float deltaTime = m_animClock.restart().asSeconds();
        m_frameTime += deltaTime;

        // Cambiar de frame cada 0.05 segundos (ajústalo si la animación va muy rápido/lento)
        if (m_frameTime >= 0.05f) {
            m_currentFrame = (m_currentFrame + 1) % TOTAL_FRAMES;

            // Calcular en qué fila y columna está el frame actual
            int row = m_currentFrame / COLUMNS;
            int col = m_currentFrame % COLUMNS;

            // Aplicar el nuevo recorte
            m_cocoSprite.setTextureRect(sf::IntRect({col * FRAME_WIDTH, row * FRAME_HEIGHT}, {FRAME_WIDTH, FRAME_HEIGHT}));
            m_frameTime = 0.0f;
        }
    } else {
        m_animClock.restart();
    }

    // Sincronizar posición con Box2D
b2Vec2 pos = b2Body_GetPosition(m_cueBallId);
    float SCALE = 30.0f; // Nuestra constante de conversión
    
    m_cocoSprite.setPosition({pos.x * SCALE, pos.y * SCALE});
}

void Game::spawnTriangle() {
    float SCALE = 30.0f;
    float startX = 900.0f / SCALE; // Posición de la primera bola (Ápice del triángulo)
    float startY = 360.0f / SCALE; // Centro vertical exacto de la pantalla
    float radius = 15.0f / SCALE;  // Radio físico (Diámetro 30)
    
    float diameter = radius * 2.0f;
    float rowSpacing = diameter * 0.866f; // Ajuste horizontal hexagonal

    // Bucle anidado para crear las 5 filas (1 + 2 + 3 + 4 + 5 = 15 frutas)
    for (int row = 0; row < 5; ++row) {
        float firstY = startY - (row * radius); // Desplazamiento Y inicial de cada fila

        for (int col = 0; col <= row; ++col) {
            float x = startX + (row * rowSpacing);
            float y = firstY + (col * diameter);

            // 1. Crear el cuerpo físico
            b2BodyDef bodyDef = b2DefaultBodyDef();
            bodyDef.type = b2_dynamicBody;
            bodyDef.position = {x, y};
            
            // Fricción de rodadura para que se detengan solas
            bodyDef.linearDamping = 1.2f; 
            bodyDef.angularDamping = 1.0f;

            b2BodyId fruitId = b2CreateBody(m_worldId, &bodyDef);

            // 2. Crear la forma y el material de colisión
            b2Circle circle = {{0.0f, 0.0f}, radius};
            b2ShapeDef shapeDef = b2DefaultShapeDef();
            
            // Propiedades físicas de una bola de billar real
            shapeDef.restitution = 0.85f; // Mucho rebote
            shapeDef.friction = 0.2f;     // Poca fricción al raspar
            shapeDef.density = 1.0f;

            b2CreateCircleShape(fruitId, &shapeDef, &circle);

            // 3. Almacenar el ID en nuestra lista
            m_fruitIds.push_back(fruitId);
        }
    }
}