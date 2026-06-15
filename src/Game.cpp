#include "Game.hpp"
#include <iostream> 
#include <cmath>
#include <limits>
#include <string>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <random>


static const unsigned int WINDOW_WIDTH = 1480;
static const unsigned int WINDOW_HEIGHT = 920;
static const float WINDOW_CENTER_X = WINDOW_WIDTH / 2.0f;
static const float WINDOW_CENTER_Y = WINDOW_HEIGHT / 2.0f;
static const float TABLE_OFFSET_Y = 60.0f;

static bool loadTexture(sf::Texture& texture, const std::string& path) {
    if (!texture.loadFromFile(path)) {
        std::cerr << "No se pudo cargar la imagen: " << path
                  << "\nEjecuta el juego desde la carpeta raiz del proyecto, donde existe assets/images/."
                  << std::endl;
        return false;
    }
    return true;
}

static bool loadOptionalTexture(sf::Texture& texture, const std::string& path) {
    if (!std::ifstream(path).good()) {
        return false;
    }

    return texture.loadFromFile(path);
}

static bool loadSoundBuffer(sf::SoundBuffer& buffer, const std::string& path) {
    if (!std::ifstream(path).good()) {
        std::cerr << "No se pudo cargar el audio: " << path << std::endl;
        return false;
    }

    if (!buffer.loadFromFile(path)) {
        std::cerr << "No se pudo cargar el audio: " << path << std::endl;
        return false;
    }

    return true;
}

static bool openOptionalMusic(sf::Music& music, const std::string& path) {
    if (!std::ifstream(path).good()) {
        return false;
    }

    return music.openFromFile(path);
}


// NOTA: Este código asume que tienes las imágenes "mantel.jpg", "coco.png" y "taco.png" en la carpeta "assets/images/" de tu proyecto.
Game::Game() : m_window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Fruit Pool - Fase 8") {
    m_window.setFramerateLimit(60);
    loadAssets();
    initPhysics();// 1. Inicializar el mundo físico de Box2D y sus bandas
    spawnTriangle(); // Llenamos la mesa con el triángulo de frutas al iniciar
    initPockets();   // Configuramos las posiciones de las troneras
    resetTurnTimer();
    updateWindowTitle();


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
    bodyDef.position = {485.0f / SCALE, (460.0f + TABLE_OFFSET_Y) / SCALE}; // Posición inicial del Coco
    bodyDef.linearDamping = 1.2f; // Fricción del tapete
    bodyDef.angularDamping = 0.8f;// Fricción de rotación para que no gire indefinidamente


    m_cueBallId = b2CreateBody(m_worldId, &bodyDef);


    // 3. Darle forma y peso 
    b2Circle dynamicCircle;
    dynamicCircle.center = {0.0f, 0.0f};// El centro del círculo se define en el sistema local del cuerpo
    dynamicCircle.radius = 15.0f / SCALE; // Radio del Coco (15 píxeles convertido a metros físicos)


    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;// Densidad del Coco (puedes ajustar para que se sienta más pesado o ligero)
    shapeDef.friction = 0.1f;// Fricción entre la bola y el tapete
    shapeDef.restitution = 1.0f; // Rebote


    b2CreateCircleShape(m_cueBallId, &shapeDef, &dynamicCircle);
    
// Grosor de las bandas
float wallThickness = 60.0f;


// Horizontales (superior e inferior, se separan para dejar espacio a las esquinas)
//x, y, width, height
createWall(485.0f, 173.0f, 423.0f, wallThickness);  // Superior izquierda
createWall(999.0f, 173.0f, 425.0f, wallThickness);  // Superior derecha
createWall(484.0f, 746.0f, 423.0f, wallThickness); // Inferior izquierda
createWall(998.0f, 746.0f, 423.0f, wallThickness); // Inferior derecha


// Verticales (izquierda y derecha, recortadas en los extremos para no afectar el radio de las troneras)
createWall(196.0f, 460.0f, wallThickness, 425.0f); // Izquierda
createWall(1285.0f, 458.0f, wallThickness, 427.0f); // Derecha


// Esquinas ahora con dos cubos iguales en 45 grados, tangencialmente unidos al radio de las troneras
// Esquinas ahora con bloques rectangulares que pasan por la esquina más cercana del muro.
float blockLength = 120.0f; // Largo del rectángulo
float blockThickness = 32.0f; // Grosor del rectángulo


// Top-left pocket: calcular centros para que los vértices pedidos toquen las esquinas de los muros
{
    float thetaDeg = 45.0f;
    float theta = thetaDeg * 3.14159265f / 180.0f;
    float halfW = blockLength * 0.5f;
    float halfH = blockThickness * 0.5f;


    // Coordenadas de los muros que delimitan la esquina (valores exactos según createWall anteriores)
    float topHorCenterX = 485.0f;
    float topHorWidth = 423.0f;
    float topHorCenterY = 173.0f;


    float vertLeftCenterX = 196.0f;
    float vertLeftCenterY = 460.0f;
    float vertLeftHeight = 425.0f;


    // Punto objetivo A: vértice inferior-izquierdo del muro horizontal (queremos que el BR del rectángulo lo toque)
    sf::Vector2f targetA;
    targetA.x = topHorCenterX - (topHorWidth * 0.5f); // left edge
    targetA.y = topHorCenterY + (wallThickness * 0.5f); // bottom edge


    // Punto objetivo B: vértice superior-derecho del muro vertical (queremos que el TR del rectángulo lo toque)
    sf::Vector2f targetB;
    targetB.x = vertLeftCenterX + (wallThickness * 0.5f); // right edge
    targetB.y = vertLeftCenterY - (vertLeftHeight * 0.5f); // top edge


    auto rot = [&](float x, float y) -> sf::Vector2f {
        return { x * std::cos(theta) - y * std::sin(theta), x * std::sin(theta) + y * std::cos(theta) };
    };


    // Para targetA queremos que el bottom-right local (halfW, halfH) coincida con targetA
    // Nota: rot aplica rotación desde el sistema local (center como origen)
    sf::Vector2f localBR = rot(halfW, halfH);
    sf::Vector2f centerA = targetA - localBR;


    // Para targetB queremos que el top-right local (halfW, -halfH) coincida con targetB
    sf::Vector2f localTR = rot(halfW, -halfH);
    sf::Vector2f centerB = targetB - localTR;


    createWall(centerA.x, centerA.y, blockLength, blockThickness, thetaDeg);
    createWall(centerB.x, centerB.y, blockLength, blockThickness, thetaDeg);
}
// Top-right pocket: calcular centros para que los vértices pedidos toquen las esquinas de los muros
{
    float thetaDeg = -45.0f;
    float theta = thetaDeg * 3.14159265f / 180.0f;
    float halfW = blockLength * 0.5f;
    float halfH = blockThickness * 0.5f;


    float topHor2CenterX = 999.0f;
    float topHor2Width = 425.0f;
    float topHorCenterY2 = 173.0f;


    float vertRightCenterX = 1285.0f;
    float vertRightCenterY = 458.0f;
    float vertRightHeight = 427.0f;


    sf::Vector2f targetA;
    targetA.x = topHor2CenterX + (topHor2Width * 0.5f); // right edge
    targetA.y = topHorCenterY2 + (wallThickness * 0.5f); // bottom edge


    sf::Vector2f targetB;
    targetB.x = vertRightCenterX - (wallThickness * 0.5f); // left edge of right vertical
    targetB.y = vertRightCenterY - (vertRightHeight * 0.5f); // top edge


    auto rot = [&](float x, float y) -> sf::Vector2f {
        return { x * std::cos(theta) - y * std::sin(theta), x * std::sin(theta) + y * std::cos(theta) };
    };


    sf::Vector2f localA = rot(-halfW, halfH); // bottom-left local
    sf::Vector2f localB = rot(-halfW, -halfH); // top-left local


    sf::Vector2f centerA = targetA - localA;
    sf::Vector2f centerB = targetB - localB;


    createWall(centerA.x, centerA.y, blockLength, blockThickness, thetaDeg);
    createWall(centerB.x, centerB.y, blockLength, blockThickness, thetaDeg);
}


// Bottom-left pocket: calcular centros (espejo vertical del top-left)
{
    float thetaDeg = -45.0f;
    float theta = thetaDeg * 3.14159265f / 180.0f;
    float halfW = blockLength * 0.5f;
    float halfH = blockThickness * 0.5f;


    float bottomHorCenterX = 484.0f;
    float bottomHorWidth = 423.0f;
    float bottomHorCenterY = 746.0f;


    float vertLeftCenterX = 196.0f;
    float vertLeftCenterY = 460.0f;
    float vertLeftHeight = 425.0f;


    sf::Vector2f targetA;
    targetA.x = bottomHorCenterX - (bottomHorWidth * 0.5f); // left edge
    targetA.y = bottomHorCenterY - (wallThickness * 0.5f); // top edge of bottom wall


    sf::Vector2f targetB;
    targetB.x = vertLeftCenterX + (wallThickness * 0.5f); // right edge of left vertical
    targetB.y = vertLeftCenterY + (vertLeftHeight * 0.5f); // bottom edge


    auto rot = [&](float x, float y) -> sf::Vector2f {
        return { x * std::cos(theta) - y * std::sin(theta), x * std::sin(theta) + y * std::cos(theta) };
    };


    sf::Vector2f localA = rot(halfW, -halfH); // top-right local
    sf::Vector2f localB = rot(halfW, halfH);  // bottom-right local


    sf::Vector2f centerA = targetA - localA;
    sf::Vector2f centerB = targetB - localB;


    createWall(centerA.x, centerA.y, blockLength, blockThickness, thetaDeg);
    createWall(centerB.x, centerB.y, blockLength, blockThickness, thetaDeg);
}


// Bottom-right pocket: calcular centros para la esquina inferior-derecha
{
    float thetaDeg = 45.0f;
    float theta = thetaDeg * 3.14159265f / 180.0f;
    float halfW = blockLength * 0.5f;
    float halfH = blockThickness * 0.5f;


    float bottomHor2CenterX = 998.0f;
    float bottomHor2Width = 423.0f;
    float bottomHor2CenterY = 746.0f;


    float vertRightCenterX = 1285.0f;
    float vertRightCenterY = 458.0f;
    float vertRightHeight = 427.0f;


    sf::Vector2f targetA;
    targetA.x = bottomHor2CenterX + (bottomHor2Width * 0.5f); // right edge
    targetA.y = bottomHor2CenterY - (wallThickness * 0.5f); // top edge of bottom wall


    sf::Vector2f targetB;
    targetB.x = vertRightCenterX - (wallThickness * 0.5f); // left edge of right vertical
    targetB.y = vertRightCenterY + (vertRightHeight * 0.5f); // bottom edge


    auto rot = [&](float x, float y) -> sf::Vector2f {
        return { x * std::cos(theta) - y * std::sin(theta), x * std::sin(theta) + y * std::cos(theta) };
    };


    sf::Vector2f localA = rot(-halfW, -halfH); // top-left local
    sf::Vector2f localB = rot(-halfW, halfH);  // bottom-left local


    sf::Vector2f centerA = targetA - localA;
    sf::Vector2f centerB = targetB - localB;


    createWall(centerA.x, centerA.y, blockLength, blockThickness, thetaDeg);
    createWall(centerB.x, centerB.y, blockLength, blockThickness, thetaDeg);
}


// Top-center pocket: par de cuadrados a 25 grados
{
    float thetaDeg = 25.0f;
    float theta = thetaDeg * 3.14159265f / 180.0f;
    float squareSide = blockThickness;
    float halfSide = squareSide * 0.5f;


    float leftGapX = 485.0f + (423.0f * 0.5f);
    float rightGapX = 999.0f - (425.0f * 0.5f);
    float topPocketY = 173.0f + (wallThickness * 0.5f);


    sf::Vector2f targetA = {leftGapX, topPocketY};
    sf::Vector2f targetB = {rightGapX, topPocketY};


    auto rotA = [&](float x, float y) -> sf::Vector2f {
        return { x * std::cos(theta) - y * std::sin(theta), x * std::sin(theta) + y * std::cos(theta) };
    };
    float thetaDegB = -25.0f;
    float thetaB = thetaDegB * 3.14159265f / 180.0f;
    auto rotB = [&](float x, float y) -> sf::Vector2f {
        return { x * std::cos(thetaB) - y * std::sin(thetaB), x * std::sin(thetaB) + y * std::cos(thetaB) };
    };


    sf::Vector2f centerA = targetA - rotA(halfSide, halfSide);
    sf::Vector2f centerB = targetB - rotB(-halfSide, halfSide);


    createWall(centerA.x, centerA.y, squareSide, squareSide, thetaDeg);
    createWall(centerB.x, centerB.y, squareSide, squareSide, thetaDegB);
}


// Bottom-center pocket: opuesto a los cuadrados superiores
{
    float thetaDegA = -25.0f;
    float thetaA = thetaDegA * 3.14159265f / 180.0f;
    float thetaDegB = 25.0f;
    float thetaB = thetaDegB * 3.14159265f / 180.0f;
    float squareSide = blockThickness;
    float halfSide = squareSide * 0.5f;


    float leftGapX = 484.0f + (423.0f * 0.5f);
    float rightGapX = 998.0f - (423.0f * 0.5f);
    float bottomPocketY = 746.0f - (wallThickness * 0.5f);


    sf::Vector2f targetA = {leftGapX, bottomPocketY};
    sf::Vector2f targetB = {rightGapX, bottomPocketY};


    auto rotA = [&](float x, float y) -> sf::Vector2f {
        return { x * std::cos(thetaA) - y * std::sin(thetaA), x * std::sin(thetaA) + y * std::cos(thetaA) };
    };
    auto rotB = [&](float x, float y) -> sf::Vector2f {
        return { x * std::cos(thetaB) - y * std::sin(thetaB), x * std::sin(thetaB) + y * std::cos(thetaB) };
    };


    sf::Vector2f centerA = targetA - rotA(halfSide, -halfSide);
    sf::Vector2f centerB = targetB - rotB(-halfSide, -halfSide);


    createWall(centerA.x, centerA.y, squareSide, squareSide, thetaDegA);
    createWall(centerB.x, centerB.y, squareSide, squareSide, thetaDegB);
}



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

        if (m_showMainMenu) {
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
                if (m_menuPanelOpen && m_musicSliderBounds.contains(mousePos)) {
                    m_draggingVolumeSlider = true;
                    setMusicVolumeFromMouse(mousePos.x);
                }
            }

            if (event.type == sf::Event::MouseMoved && m_draggingVolumeSlider) {
                setMusicVolumeFromMouse(static_cast<float>(event.mouseMove.x));
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
                m_draggingVolumeSlider = false;

                if (m_menuPanelOpen) {
                    if (m_closeMenuPanelBounds.contains(mousePos)) {
                        m_menuPanelOpen = false;
                    } else if (m_musicSliderBounds.contains(mousePos)) {
                        setMusicVolumeFromMouse(mousePos.x);
                    } else if (m_rulesButtonBounds.contains(mousePos)) {
                        m_menuPanelView = 1;
                    } else if (m_creditsButtonBounds.contains(mousePos)) {
                        m_menuPanelView = 2;
                    }
                } else if (m_menuButtonBounds.contains(mousePos)) {
                    m_menuPanelOpen = true;
                    m_menuPanelView = 0;
                } else if (m_playButtonBounds.contains(mousePos)) {
                    m_showMainMenu = false;
                    m_menuPanelOpen = false;
                    resetTurnTimer();
                    updateWindowTitle();
                }
            }

            continue;
        }

        if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
            if (m_restartButtonBounds.contains(mousePos)) {
                restartMatch();
                continue;
            }

            if (m_backToMenuButtonBounds.contains(mousePos)) {
                m_isAiming = false;
                m_showMainMenu = true;
                m_menuPanelOpen = false;
                m_menuPanelView = 0;
                updateWindowTitle();
                continue;
            }
        }


        // Evento: Jugador hace Click Izquierdo (Empieza a apuntar/cargar fuerza)
        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left && m_phase == GamePhase::AIMING && areBallsStopped()) {
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


                // Creamos un vector de fuerza. Multiplicamos por 0.40f para mucho más potencia.
                if (std::sqrt(deltaX * deltaX + deltaY * deltaY) < 4.0f) {
                    continue;
                }

                m_phase = GamePhase::BALLS_MOVING;
                m_cueBallPocketedThisShot = false;
                m_eightBallPocketedThisShot = false;
                m_shotPocketedGroups.clear();
                playCueHitSound();
                updateWindowTitle();

                b2Vec2 impulse = {deltaX * 0.40f, deltaY * 0.40f};
                
                // Obtenemos la posición actual de la bola
                b2Vec2 pos = b2Body_GetPosition(m_cueBallId);
                
                // Aplicamos el impulso a la bola en Box2D (true es para "despertar" el cuerpo)
                b2Body_ApplyLinearImpulse(m_cueBallId, impulse, pos, true);
            }
        }
    }
}


void Game::update() {
    if (m_showMainMenu) {
        return;
    }

    // Calculamos el siguiente frame físico
    updateAnimation();
    b2World_Step(m_worldId, 1.0f / 60.0f, 4);
    checkCollisionAudio();
    checkPockets();
    resolveShotIfReady();
    updateTurnTimer();
    
}


sf::Vector2f Game::normalize(const sf::Vector2f& vector) const {
    float length = std::sqrt(vector.x * vector.x + vector.y * vector.y);
    if (length <= 0.0001f) {
        return {0.0f, 0.0f};
    }
    return {vector.x / length, vector.y / length};
}


float Game::dot(const sf::Vector2f& a, const sf::Vector2f& b) const {
    return a.x * b.x + a.y * b.y;
}


bool Game::predictBallCollision(const sf::Vector2f& origin,
                                 const sf::Vector2f& direction,
                                 sf::Vector2f& collisionPoint,
                                 sf::Vector2f& fruitCenter,
                                 sf::Vector2f& reboundDir,
                                 b2BodyId& hitId) const {
    const float ballRadiusPixels = 15.0f;
    const float combinedRadius = ballRadiusPixels * 2.0f;
    const float combinedRadiusSq = combinedRadius * combinedRadius;
    float closestT = std::numeric_limits<float>::infinity();
    bool found = false;


    for (const Fruit& fruit : m_fruits) {
        b2Vec2 pos = b2Body_GetPosition(fruit.bodyId);
        sf::Vector2f center = {pos.x * SCALE, pos.y * SCALE};
        sf::Vector2f oc = center - origin;


        float tca = dot(oc, direction);
        if (tca < 0.0f) {
            continue;
        }


        float d2 = dot(oc, oc) - (tca * tca);
        if (d2 > combinedRadiusSq) {
            continue;
        }


        float thc = std::sqrt(std::max(0.0f, combinedRadiusSq - d2));
        float t0 = tca - thc;
        float t1 = tca + thc;
        float t = (t0 >= 0.0f) ? t0 : t1;
        if (t < 0.0f) {
            continue;
        }


        if (t < closestT) {
            closestT = t;
            found = true;
            hitId = fruit.bodyId;
            collisionPoint = origin + direction * t;
            fruitCenter = center;
            reboundDir = normalize(center - collisionPoint);
        }
    }


    return found;
}


bool Game::predictWallCollision(const sf::Vector2f& origin,
                                 const sf::Vector2f& direction,
                                 sf::Vector2f& collisionPoint) const {
    const float maxRayDistance = 2000.0f;
    float closestT = std::numeric_limits<float>::infinity();
    bool found = false;

    for (const WallRender& wall : m_wallRenders) {
        float angle = -wall.angle * 3.14159265f / 180.0f;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        sf::Vector2f relativeOrigin = origin - sf::Vector2f(wall.x, wall.y);
        sf::Vector2f localOrigin = {
            relativeOrigin.x * cosA - relativeOrigin.y * sinA,
            relativeOrigin.x * sinA + relativeOrigin.y * cosA
        };
        sf::Vector2f localDirection = {
            direction.x * cosA - direction.y * sinA,
            direction.x * sinA + direction.y * cosA
        };

        float boundsMin[2] = {
            -wall.w * 0.5f,
            -wall.h * 0.5f
        };
        float boundsMax[2] = {
            wall.w * 0.5f,
            wall.h * 0.5f
        };
        float rayStart[2] = {localOrigin.x, localOrigin.y};
        float rayDir[2] = {localDirection.x, localDirection.y};

        float tMin = 0.0f;
        float tMax = maxRayDistance;
        bool intersects = true;

        for (int axis = 0; axis < 2; ++axis) {
            if (std::abs(rayDir[axis]) < 0.0001f) {
                if (rayStart[axis] < boundsMin[axis] || rayStart[axis] > boundsMax[axis]) {
                    intersects = false;
                    break;
                }
            } else {
                float t1 = (boundsMin[axis] - rayStart[axis]) / rayDir[axis];
                float t2 = (boundsMax[axis] - rayStart[axis]) / rayDir[axis];
                if (t1 > t2) {
                    std::swap(t1, t2);
                }

                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);
                if (tMin > tMax) {
                    intersects = false;
                    break;
                }
            }
        }

        if (intersects && tMin > 0.1f && tMin < closestT) {
            closestT = tMin;
            found = true;
        }
    }

    if (found) {
        collisionPoint = origin + direction * closestT;
    }

    return found;
}


std::vector<std::pair<sf::Vector2f, sf::Vector2f>> Game::predictTrajectory(const sf::Vector2f& origin,
                                                                             const sf::Vector2f& direction,
                                                                             int maxBounces) const {
    std::vector<std::pair<sf::Vector2f, sf::Vector2f>> trajectory;
    
    sf::Vector2f currentPos = origin;
    sf::Vector2f currentDir = normalize(direction);
    const float maxRayDistance = 2000.0f;
    const float ballRadiusPixels = 15.0f;
    
    for (int bounce = 0; bounce < maxBounces; ++bounce) {
        float closestT = maxRayDistance;
        sf::Vector2f hitPoint = currentPos + currentDir * maxRayDistance;
        sf::Vector2f nextDir = currentDir;
        
        // Detectar colisiones con frutas
        for (const Fruit& fruit : m_fruits) {
            b2Vec2 pos = b2Body_GetPosition(fruit.bodyId);
            sf::Vector2f center = {pos.x * SCALE, pos.y * SCALE};
            sf::Vector2f oc = center - currentPos;
            
            float tca = dot(oc, currentDir);
            if (tca < 0.1f) continue;
            
            float d2 = dot(oc, oc) - (tca * tca);
            float combinedRadiusSq = (ballRadiusPixels * 2.0f) * (ballRadiusPixels * 2.0f);
            if (d2 > combinedRadiusSq) continue;
            
            float thc = std::sqrt(std::max(0.0f, combinedRadiusSq - d2));
            float t = tca - thc;
            if (t > 0.1f && t < closestT) {
                closestT = t;
                hitPoint = currentPos + currentDir * t;
                sf::Vector2f normal = normalize(hitPoint - center);
                nextDir = normal;
            }
        }
        
        // Detectar colisiones con paredes (simplificado: aproximación a raycast contra rectángulos)
        for (const WallRender& wall : m_wallRenders) {
            // Para simplificar, tratamos cada pared como un rectángulo sin rotación aproximado
            float halfW = wall.w * 0.5f;
            float halfH = wall.h * 0.5f;
            
            // AABB simple para las paredes (sin considerar rotación para simplificar)
            float left = wall.x - halfW;
            float right = wall.x + halfW;
            float top = wall.y - halfH;
            float bottom = wall.y + halfH;
            
            // Ray-AABB intersection
            float tMin = 0.0f, tMax = maxRayDistance;
            
            for (int axis = 0; axis < 2; ++axis) {
                float rayStart = (axis == 0) ? currentPos.x : currentPos.y;
                float rayDirComponent = (axis == 0) ? currentDir.x : currentDir.y;
                float minBound = (axis == 0) ? left : top;
                float maxBound = (axis == 0) ? right : bottom;
                
                if (std::abs(rayDirComponent) < 0.0001f) {
                    if (rayStart < minBound || rayStart > maxBound) {
                        tMin = maxRayDistance + 1.0f;
                        break;
                    }
                } else {
                    float t1 = (minBound - rayStart) / rayDirComponent;
                    float t2 = (maxBound - rayStart) / rayDirComponent;
                    if (t1 > t2) std::swap(t1, t2);
                    tMin = std::max(tMin, t1);
                    tMax = std::min(tMax, t2);
                    if (tMin > tMax) {
                        tMin = maxRayDistance + 1.0f;
                        break;
                    }
                }
            }
            
            if (tMin > 0.1f && tMin < closestT) {
                closestT = tMin;
                hitPoint = currentPos + currentDir * tMin;
                
                // Calcular normal aproximada
                float dx = hitPoint.x - wall.x;
                float dy = hitPoint.y - wall.y;
                
                if (std::abs(dx) > std::abs(dy)) {
                    nextDir = {-currentDir.x, currentDir.y};
                } else {
                    nextDir = {currentDir.x, -currentDir.y};
                }
            }
        }
        
        trajectory.push_back({currentPos, hitPoint});
        
        if (closestT >= maxRayDistance - 1.0f) {
            break;
        }
        
        currentPos = hitPoint + normalize(nextDir) * 5.0f; // Pequeño offset para evitar auto-intersección
        currentDir = nextDir;
    }
    
    return trajectory;
}


void Game::render() {
    if (m_showMainMenu) {
        drawMainMenu();
        return;
    }



    // Limpiamos la ventana (color azul de mesa)
    m_window.clear(sf::Color(20, 80, 150));
    
// Obtenemos la posición física
//b2Vec2 pos = b2Body_GetPosition(m_cueBallId);
// Convertimos a píxeles
// float pixelX = pos.x * SCALE;
   // float pixelY = pos.y * SCALE;


    // Imprimimos coordenadas en la terminal
   // std::cout << "Posicion del Coco: X=" << pixelX << ", Y=" << pixelY << std::endl;
    
    // Dibujar el fondo primero, luego el mantel atrás del marco y después las frutas
    m_window.draw(m_backgroundSprite);
    m_window.draw(m_tableSprite);   // Mantel centrado detras del marco
    m_window.draw(m_frameSprite);


    m_window.draw(m_cocoSprite);    // Luego las frutas


    // Dibuja el resto de las frutas de la mesa
    float SCALE = 30.0f;
    for (Fruit& fruit : m_fruits) {
        b2Vec2 pos = b2Body_GetPosition(fruit.bodyId);
        fruit.sprite.setPosition({pos.x * SCALE, pos.y * SCALE});
        m_window.draw(fruit.sprite);
    }
   
// NUEVO: Apuntado avanzado con retroceso y láser grueso
float powerPercentage = 0.0f;

if (m_isAiming && m_phase == GamePhase::AIMING) {
    // 1. Obtener la posición física central del Coco
    b2Vec2 pos = b2Body_GetPosition(m_cueBallId);
        float pixelX = pos.x * SCALE;
        float pixelY = pos.y * SCALE;


        // 2. Calcular el vector de "jalón"
        sf::Vector2i currentMousePos = sf::Mouse::getPosition(m_window);
        float pullX = m_mouseStartPos.x - currentMousePos.x;
        float pullY = m_mouseStartPos.y - currentMousePos.y;


        // 3. Limitar el retroceso máximo de la caña (Clamp)
        float pullDist = std::sqrt(pullX * pullX + pullY * pullY);
        float maxPull = 200.0f; // Píxeles máximos que la caña se hará hacia atrás
        if (pullDist > maxPull) {
            pullX = (pullX / pullDist) * maxPull;
            pullY = (pullY / pullDist) * maxPull;
            pullDist = maxPull;
        }


        // 4. Dibujar la Guía de Tiro
        sf::Vector2f aimDir = normalize({pullX, pullY});


        sf::Vector2f origin = {pixelX, pixelY};
        sf::Vector2f aimEnd = origin + aimDir * 1500.0f;


        sf::Vector2f fruitCollisionPoint;
        sf::Vector2f fruitCenter;
        sf::Vector2f fruitDirection;
        b2BodyId hitId;
        bool hasFruitCollision = predictBallCollision(origin, aimDir, fruitCollisionPoint, fruitCenter, fruitDirection, hitId);

        sf::Vector2f wallCollisionPoint;
        bool hasWallCollision = predictWallCollision(origin, aimDir, wallCollisionPoint);

        float fruitDistance = hasFruitCollision
            ? std::sqrt(dot(fruitCollisionPoint - origin, fruitCollisionPoint - origin))
            : std::numeric_limits<float>::infinity();
        float wallDistance = hasWallCollision
            ? std::sqrt(dot(wallCollisionPoint - origin, wallCollisionPoint - origin))
            : std::numeric_limits<float>::infinity();

        bool showFruitPrediction = hasFruitCollision && fruitDistance <= wallDistance;
        if (showFruitPrediction) {
            aimEnd = fruitCenter - fruitDirection * 30.0f;
        } else if (hasWallCollision) {
            aimEnd = wallCollisionPoint;
        }


        // Dibujar la línea blanca de la trayectoria del coco
        float shotAngle = std::atan2(aimDir.y, aimDir.x) * 180.0f / 3.14159265f;
        float aimLength = std::sqrt(dot(aimEnd - origin, aimEnd - origin));
        sf::RectangleShape aimLine({aimLength, 3.0f});
        aimLine.setOrigin({0.0f, 1.5f});
        aimLine.setPosition(origin);
        aimLine.setRotation(shotAngle);
        aimLine.setFillColor(sf::Color(255, 255, 255, 200));
        m_window.draw(aimLine);

        if (showFruitPrediction) {
            const float fruitRadiusPixels = 15.0f;
            const float ringRadius = 15.0f;
            const float ringThickness = 2.0f;
            const float shortGuideLength = 28.0f;
            const float reboundLineLength = 40.0f;

            sf::Vector2f ghostCenter = fruitCenter - fruitDirection * (fruitRadiusPixels + ringRadius);
            sf::Vector2f guideStart = fruitCenter + fruitDirection * (fruitRadiusPixels + 2.0f);
            sf::Vector2f guideEnd = guideStart + fruitDirection * shortGuideLength;
            float guideAngle = std::atan2(fruitDirection.y, fruitDirection.x) * 180.0f / 3.14159265f;
            sf::Vector2f cueReboundDir = normalize(aimDir - fruitDirection * dot(aimDir, fruitDirection));
            float reboundLenSq = dot(cueReboundDir, cueReboundDir);

            sf::CircleShape impactRing(ringRadius);
            impactRing.setOrigin({ringRadius, ringRadius});
            impactRing.setPosition(ghostCenter);
            impactRing.setFillColor(sf::Color::Transparent);
            impactRing.setOutlineThickness(ringThickness);
            impactRing.setOutlineColor(sf::Color(255, 220, 80, 230));
            m_window.draw(impactRing);

            sf::RectangleShape fruitPath({shortGuideLength, 3.0f});
            fruitPath.setOrigin({0.0f, 1.5f});
            fruitPath.setPosition(guideStart);
            fruitPath.setRotation(guideAngle);
            fruitPath.setFillColor(sf::Color(255, 150, 60, 220));
            m_window.draw(fruitPath);

            sf::CircleShape fruitPathEnd(3.5f);
            fruitPathEnd.setOrigin({3.5f, 3.5f});
            fruitPathEnd.setPosition(guideEnd);
            fruitPathEnd.setFillColor(sf::Color(255, 150, 60, 220));
            m_window.draw(fruitPathEnd);

            if (reboundLenSq > 0.0001f) {
                sf::RectangleShape reboundLine({reboundLineLength, 3.0f});
                reboundLine.setOrigin({0.0f, 1.5f});
                reboundLine.setPosition(ghostCenter);
                reboundLine.setRotation(std::atan2(cueReboundDir.y, cueReboundDir.x) * 180.0f / 3.14159265f);
                reboundLine.setFillColor(sf::Color(255, 255, 255, 150));
                m_window.draw(reboundLine);
            }
        }


        // 5. Dibujar la Caña de Azúcar con Retroceso
        float stickAngle = std::atan2(-pullY, -pullX) * 180.0f / 3.14159265f;
        m_cueSprite.setPosition({pixelX - pullX, pixelY - pullY});
        m_cueSprite.setRotation(stickAngle);
        m_window.draw(m_cueSprite);


        powerPercentage = std::min(pullDist / maxPull, 1.0f); // Valor entre 0 y 1


        if (false) {
        // Dimensiones de la barra
        float barWidth = 30.0f;
        float barHeight = 200.0f;
        float barX = 20.0f; // Lado izquierdo
        float barY = 260.0f; // Centrado verticalmente aproximadamente


        // Barra de fondo (gris oscuro)
        sf::RectangleShape barBackground({barWidth, barHeight});
        barBackground.setPosition({barX, barY});
        barBackground.setFillColor(sf::Color(50, 50, 50));
        m_window.draw(barBackground);


        // Barra de relleno con color dinámico
        float fillHeight = barHeight * powerPercentage;
        sf::RectangleShape barFill({barWidth, fillHeight});
        barFill.setPosition({barX, barY + barHeight - fillHeight}); // Desde abajo hacia arriba


        // Determinar el color según la potencia
        sf::Color powerColor;
        if (powerPercentage < 0.33f) {
            // Verde (bajo)
            powerColor = sf::Color(0, 255, 0);
        } else if (powerPercentage < 0.66f) {
            // Amarillo (intermedio)
            powerColor = sf::Color(255, 255, 0);
        } else {
            // Rojo (máximo)
            powerColor = sf::Color(255, 0, 0);
        }


        barFill.setFillColor(powerColor);
        m_window.draw(barFill);


        // Borde de la barra (blanco)
        sf::RectangleShape barBorder({barWidth, barHeight});
        barBorder.setPosition({barX, barY});
        barBorder.setFillColor(sf::Color::Transparent);
        barBorder.setOutlineThickness(2.0f);
        barBorder.setOutlineColor(sf::Color::White);
        m_window.draw(barBorder);
        }
    }

    const float powerBarWidth = 46.0f;
    const float powerBarHeight = 360.0f;
    const float powerBarX = 38.0f;
    const float powerBarY = (WINDOW_HEIGHT - powerBarHeight) * 0.5f;

    sf::RectangleShape powerBarBackground({powerBarWidth, powerBarHeight});
    powerBarBackground.setPosition({powerBarX, powerBarY});
    powerBarBackground.setFillColor(sf::Color(36, 44, 50, 225));
    m_window.draw(powerBarBackground);

    float powerFillHeight = powerBarHeight * powerPercentage;
    sf::RectangleShape powerBarFill({powerBarWidth, powerFillHeight});
    powerBarFill.setPosition({powerBarX, powerBarY + powerBarHeight - powerFillHeight});

    sf::Color powerColor(82, 210, 115);
    if (powerPercentage >= 0.66f) {
        powerColor = sf::Color(235, 85, 70);
    } else if (powerPercentage >= 0.33f) {
        powerColor = sf::Color(245, 210, 95);
    }

    powerBarFill.setFillColor(powerColor);
    m_window.draw(powerBarFill);

    sf::RectangleShape powerBarBorder({powerBarWidth, powerBarHeight});
    powerBarBorder.setPosition({powerBarX, powerBarY});
    powerBarBorder.setFillColor(sf::Color::Transparent);
    powerBarBorder.setOutlineThickness(3.0f);
    powerBarBorder.setOutlineColor(sf::Color(178, 226, 178));
    m_window.draw(powerBarBorder);

    drawHUD();
    m_window.display();
}


void Game::createWall(float x, float y, float width, float height, float angleDegrees) {
    y += TABLE_OFFSET_Y;

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody; // Estático: inamovible como una pared
    // Box2D ubica los rectángulos desde su centro, por eso dividimos entre SCALE
    bodyDef.position = {x / SCALE, y / SCALE};
    
    b2BodyId wallId = b2CreateBody(m_worldId, &bodyDef);
    if (std::abs(angleDegrees) > 0.001f) {
        b2Body_SetTransform(wallId, bodyDef.position, b2MakeRot(angleDegrees * 3.14159265f / 180.0f));
    }


    // Box2D requiere "half-widths" (la mitad del ancho y alto) para crear cajas
    b2Polygon box = b2MakeBox((width / 2.0f) / SCALE, (height / 2.0f) / SCALE);
    
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.friction = 0.2f;
    shapeDef.restitution = 0.6f; // Esto simula el rebote elástico de las bandas de billar


    b2CreatePolygonShape(wallId, &shapeDef, &box);


    // Guardar datos de render para el muro invisible con las mismas medidas
    m_wallRenders.push_back({x, y, width, height, angleDegrees});
}


void Game::loadAssets() {
    // 1. Cargar el Fondo
    bool backgroundLoaded = loadTexture(m_backgroundTexture, "assets/images/fondo.png");
    if (backgroundLoaded) {
        m_backgroundSprite.setTexture(m_backgroundTexture);
        m_backgroundSprite.setOrigin({m_backgroundTexture.getSize().x / 2.0f, m_backgroundTexture.getSize().y / 2.0f});
        m_backgroundSprite.setPosition({WINDOW_CENTER_X, WINDOW_CENTER_Y});
        m_backgroundSprite.setScale({WINDOW_WIDTH / static_cast<float>(m_backgroundTexture.getSize().x), WINDOW_HEIGHT / static_cast<float>(m_backgroundTexture.getSize().y)});
    }

    bool menuLoaded = loadOptionalTexture(m_menuTexture, "assets/images/menu.png");
    if (!menuLoaded && backgroundLoaded) {
        m_menuSprite.setTexture(m_backgroundTexture);
        m_menuSprite.setScale({WINDOW_WIDTH / static_cast<float>(m_backgroundTexture.getSize().x), WINDOW_HEIGHT / static_cast<float>(m_backgroundTexture.getSize().y)});
    } else if (menuLoaded) {
        m_menuSprite.setTexture(m_menuTexture);
        m_menuSprite.setScale({WINDOW_WIDTH / static_cast<float>(m_menuTexture.getSize().x), WINDOW_HEIGHT / static_cast<float>(m_menuTexture.getSize().y)});
    }
    m_menuSprite.setPosition({0.0f, 0.0f});


    // 1.5. Cargar el Marco
    if (!loadTexture(m_frameTexture, "assets/images/marco.png")) {
        // Manejo de error básico
    }
    m_frameSprite.setTexture(m_frameTexture);
    // Configurar el origen del marco al centro de la imagen
    m_frameSprite.setOrigin(m_frameTexture.getSize().x / 2.0f, m_frameTexture.getSize().y / 2.0f);
    // Posicionar el marco exactamente en el centro de la ventana (mismo que el mantel)
    m_frameSprite.setPosition(WINDOW_CENTER_X, WINDOW_CENTER_Y + TABLE_OFFSET_Y);
    // No escalamos el marco, se dibuja en su tamaño original (1194x683)


    // 2. Cargar el Mantel
    if (!loadTexture(m_tableTexture, "assets/images/mantel.jpg")) {
        // Manejo de error básico
    }
    m_tableSprite.setTexture(m_tableTexture);
    // 1. Configurar el origen del mantel al centro de su imagen real
    m_tableSprite.setOrigin(m_tableTexture.getSize().x / 2.0f, m_tableTexture.getSize().y / 2.0f);
    // 2. Posicionar el mantel exactamente en el centro de la ventana
    m_tableSprite.setPosition(WINDOW_CENTER_X, WINDOW_CENTER_Y + TABLE_OFFSET_Y);
    // 3. Usar el tamaño real del mantel.jpg sin escalar
    m_tableSprite.setScale(1.0f, 1.0f);


    // 4. Cargar el Spritesheet del Coco
    if (!loadTexture(m_cocoTexture, "assets/images/coco.png")) {
        // Manejo de error básico
    }
    m_cocoSprite.setTexture(m_cocoTexture);


    // 3. Configurar el primer recorte (Frame 0)
    m_cocoSprite.setTextureRect(sf::IntRect({0, 0}, {FRAME_WIDTH, FRAME_HEIGHT}));
    
    // 4. Centrar el origen de la imagen en su centro exacto (50, 50)
    m_cocoSprite.setOrigin({FRAME_WIDTH / 2.0f, FRAME_HEIGHT / 2.0f});
    
    // 5. Escalar visualmente al tamaño del cuerpo físico de Box2D
    // Diámetro físico (30.0f) / Tamaño del frame en píxeles (100.0f) = Escala de 0.3f
    m_cocoSprite.setScale({34.0f / FRAME_WIDTH, 34.0f / FRAME_HEIGHT});


    // Configurar información de spritesheets para todas las frutas
    // Asumiendo que todas tienen 100x100 píxeles y 35 frames (7 columnas x 5 filas)
    m_fruitSpriteInfo[SANDIA] = {100, 100, 35, 7};
    m_fruitSpriteInfo[LIMA] = {100, 100, 35, 7};
    m_fruitSpriteInfo[LIMON] = {100, 100, 35, 7};
    m_fruitSpriteInfo[TORONJA] = {100, 100, 35, 7};
    m_fruitSpriteInfo[MANDARINA] = {100, 100, 35, 7};
    m_fruitSpriteInfo[NARANJA] = {100, 100, 35, 7};
    m_fruitSpriteInfo[GRANADA] = {100, 100, 35, 7};
    m_fruitSpriteInfo[KIWI] = {100, 100, 35, 7};
    m_fruitSpriteInfo[FRESA] = {100, 100, 35, 7};
    m_fruitSpriteInfo[CEREZA] = {100, 100, 35, 7};
    m_fruitSpriteInfo[BLACKBERRY] = {100, 100, 35, 7};
    m_fruitSpriteInfo[FRAMBUESA] = {100, 100, 35, 7};
    m_fruitSpriteInfo[UVA_VERDE] = {100, 100, 35, 7};
    m_fruitSpriteInfo[UVA_MORADA] = {100, 100, 35, 7};
    m_fruitSpriteInfo[MORA_AZUL] = {100, 100, 35, 7};


    // Cargar los spritesheets de todas las frutas
    loadTexture(m_fruitTextures[SANDIA], "assets/images/sandia.png");
    loadTexture(m_fruitTextures[LIMA], "assets/images/lima.png");
    loadTexture(m_fruitTextures[LIMON], "assets/images/limon.png");
    loadTexture(m_fruitTextures[TORONJA], "assets/images/toronja.png");
    loadTexture(m_fruitTextures[MANDARINA], "assets/images/mandarina.png");
    loadTexture(m_fruitTextures[NARANJA], "assets/images/naranja.png");
    loadTexture(m_fruitTextures[GRANADA], "assets/images/granada.png");
    loadTexture(m_fruitTextures[KIWI], "assets/images/kiwi.png");
    loadTexture(m_fruitTextures[FRESA], "assets/images/fresa.png");
    loadTexture(m_fruitTextures[CEREZA], "assets/images/cereza.png");
    loadTexture(m_fruitTextures[BLACKBERRY], "assets/images/blackberry.png");
    loadTexture(m_fruitTextures[FRAMBUESA], "assets/images/frambuesa.png");
    loadTexture(m_fruitTextures[UVA_VERDE], "assets/images/uva_verde.png");
    loadTexture(m_fruitTextures[UVA_MORADA], "assets/images/uva_morada.png");
    loadTexture(m_fruitTextures[MORA_AZUL], "assets/images/mora_azul.png");


    // 6. Cargar el Taco de Billar
if (!loadTexture(m_cueTexture, "assets/images/taco.png")) {
    // Manejo de error básico
}
m_cueSprite.setTexture(m_cueTexture);


// Mover el punto de rotación al centro del borde izquierdo (la punta del taco)
float tacoHeight = static_cast<float>(m_cueTexture.getSize().y);
m_cueSprite.setOrigin({0.0f, tacoHeight / 2.0f});


// Opcional: Ajustar la escala si tu imagen del taco es muy grande
m_cueSprite.setScale({0.5f, 0.5f});

    m_hudFontLoaded =
        m_hudFont.loadFromFile("assets/fonts/hud.ttf") ||
        m_hudFont.loadFromFile("C:/Windows/Fonts/arial.ttf");
    if (!m_hudFontLoaded) {
        std::cerr << "No se pudo cargar fuente para HUD. Agrega assets/fonts/hud.ttf." << std::endl;
    }

    for (std::size_t i = 0; i < m_avatarTextures.size(); ++i) {
        std::string path = "assets/avatars/avatar_" + std::to_string(i + 1) + ".png";
        m_avatarLoaded[i] = loadOptionalTexture(m_avatarTextures[i], path);
    }

    std::vector<int> availableAvatarIDs;
    for (std::size_t i = 0; i < m_avatarLoaded.size(); ++i) {
        if (m_avatarLoaded[i]) {
            availableAvatarIDs.push_back(static_cast<int>(i) + 1);
        }
    }

    if (!availableAvatarIDs.empty()) {
        static std::random_device randomDevice;
        static std::mt19937 generator(randomDevice());
        std::shuffle(availableAvatarIDs.begin(), availableAvatarIDs.end(), generator);

        m_playerAvatarID[0] = availableAvatarIDs[0];
        m_playerAvatarID[1] = availableAvatarIDs.size() > 1
            ? availableAvatarIDs[1]
            : availableAvatarIDs[0];
    }

    m_cueHitLoaded = loadSoundBuffer(m_cueHitBuffer, "assets/music/golpe_taco.ogg");
    if (m_cueHitLoaded) {
        m_cueHitSound.setBuffer(m_cueHitBuffer);
        m_cueHitSound.setVolume(70.0f);
    }

    m_fruitCollisionLoaded = loadSoundBuffer(m_fruitCollisionBuffer, "assets/music/choque_frutas.ogg");
    if (m_fruitCollisionLoaded) {
        m_fruitCollisionSound.setBuffer(m_fruitCollisionBuffer);
        m_fruitCollisionSound.setVolume(55.0f);
    }

    m_backgroundMusicLoaded =
        openOptionalMusic(m_backgroundMusic, "assets/music/musica_fondo.ogg") ||
        openOptionalMusic(m_backgroundMusic, "assets/music/musica.ogg") ||
        openOptionalMusic(m_backgroundMusic, "assets/music/background_music.ogg") ||
        openOptionalMusic(m_backgroundMusic, "assets/music/fondo.ogg");
    if (m_backgroundMusicLoaded) {
        m_backgroundMusic.setLoop(true);
        m_backgroundMusic.setVolume(m_musicVolume);
        m_backgroundMusic.play();
    }



}


void Game::drawMainMenu() {
    m_window.clear(sf::Color(20, 80, 150));
    m_window.draw(m_menuSprite);

    m_menuButtonBounds = sf::FloatRect(WINDOW_WIDTH - 88.0f, 28.0f, 54.0f, 44.0f);
    sf::RectangleShape menuButton({m_menuButtonBounds.width, m_menuButtonBounds.height});
    menuButton.setPosition({m_menuButtonBounds.left, m_menuButtonBounds.top});
    menuButton.setFillColor(sf::Color(20, 28, 34, 205));
    menuButton.setOutlineThickness(2.0f);
    menuButton.setOutlineColor(sf::Color(178, 226, 178));
    m_window.draw(menuButton);

    for (int i = 0; i < 3; ++i) {
        sf::RectangleShape line({28.0f, 4.0f});
        line.setPosition({m_menuButtonBounds.left + 13.0f, m_menuButtonBounds.top + 11.0f + i * 10.0f});
        line.setFillColor(sf::Color::White);
        m_window.draw(line);
    }

    const sf::Vector2f buttonSize(260.0f, 82.0f);
    const sf::Vector2f buttonPos(
        WINDOW_CENTER_X - buttonSize.x * 0.5f,
        WINDOW_CENTER_Y - buttonSize.y * 0.5f + 200.0f
    );
    m_playButtonBounds = sf::FloatRect(buttonPos.x, buttonPos.y, buttonSize.x, buttonSize.y);

    sf::RectangleShape playButton(buttonSize);
    playButton.setPosition(buttonPos);
    playButton.setFillColor(sf::Color(178, 226, 178, 235));//verde claro
    playButton.setOutlineThickness(3.0f);
    playButton.setOutlineColor(sf::Color(0, 210, 0));//amarillo
    m_window.draw(playButton);

    if (m_hudFontLoaded) {
        sf::Text playText("PLAY", m_hudFont, 42);
        playText.setStyle(sf::Text::Bold);
        playText.setFillColor(sf::Color(255, 255, 255));//blanco 
        sf::FloatRect textBounds = playText.getLocalBounds();
        playText.setOrigin({
            textBounds.left + textBounds.width * 0.5f,
            textBounds.top + textBounds.height * 0.5f
        });
        playText.setPosition({
            buttonPos.x + buttonSize.x * 0.5f,
            buttonPos.y + buttonSize.y * 0.5f - 2.0f
        });
        m_window.draw(playText);
    }

    if (m_menuPanelOpen) {
        drawCenteredMenuPanel();
    }

    m_window.display();
}


void Game::drawCenteredMenuPanel() {
    sf::RectangleShape shade({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    shade.setFillColor(sf::Color(0, 0, 0, 115));
    m_window.draw(shade);

    const sf::Vector2f panelSize(560.0f, 410.0f);
    const sf::Vector2f panelPos(WINDOW_CENTER_X - panelSize.x * 0.5f, WINDOW_CENTER_Y - panelSize.y * 0.5f);
    sf::RectangleShape panel(panelSize);
    panel.setPosition(panelPos);
    panel.setFillColor(sf::Color(24, 34, 38, 245));
    panel.setOutlineThickness(3.0f);
    panel.setOutlineColor(sf::Color(178, 226, 178));
    m_window.draw(panel);

    auto drawText = [&](const std::string& value, unsigned int size, sf::Vector2f pos, sf::Color color) {
        if (!m_hudFontLoaded) {
            return;
        }

        sf::Text text(value, m_hudFont, size);
        text.setPosition(pos);
        text.setFillColor(color);
        text.setStyle(sf::Text::Bold);
        m_window.draw(text);
    };

    drawText("Menu", 34, {panelPos.x + 32.0f, panelPos.y + 22.0f}, sf::Color::White);

    m_closeMenuPanelBounds = sf::FloatRect(panelPos.x + panelSize.x - 58.0f, panelPos.y + 20.0f, 34.0f, 34.0f);
    sf::RectangleShape closeButton({m_closeMenuPanelBounds.width, m_closeMenuPanelBounds.height});
    closeButton.setPosition({m_closeMenuPanelBounds.left, m_closeMenuPanelBounds.top});
    closeButton.setFillColor(sf::Color(178, 226, 178, 230));
    m_window.draw(closeButton);
    drawText("X", 22, {m_closeMenuPanelBounds.left + 9.0f, m_closeMenuPanelBounds.top + 3.0f}, sf::Color(18, 38, 28));

    drawText("Volumen musica", 20, {panelPos.x + 42.0f, panelPos.y + 94.0f}, sf::Color(225, 236, 232));
    m_musicSliderBounds = sf::FloatRect(panelPos.x + 42.0f, panelPos.y + 130.0f, 330.0f, 14.0f);
    sf::RectangleShape sliderTrack({m_musicSliderBounds.width, m_musicSliderBounds.height});
    sliderTrack.setPosition({m_musicSliderBounds.left, m_musicSliderBounds.top});
    sliderTrack.setFillColor(sf::Color(85, 103, 96));
    m_window.draw(sliderTrack);

    float knobX = m_musicSliderBounds.left + (m_musicVolume / 100.0f) * m_musicSliderBounds.width;
    sf::CircleShape knob(14.0f);
    knob.setOrigin({14.0f, 14.0f});
    knob.setPosition({knobX, m_musicSliderBounds.top + m_musicSliderBounds.height * 0.5f});
    knob.setFillColor(sf::Color(178, 226, 178));
    knob.setOutlineThickness(2.0f);
    knob.setOutlineColor(sf::Color::White);
    m_window.draw(knob);
    drawText(std::to_string(static_cast<int>(m_musicVolume)) + "%", 18, {panelPos.x + 394.0f, panelPos.y + 120.0f}, sf::Color::White);

    auto drawPanelButton = [&](sf::FloatRect& bounds, const std::string& label, sf::Vector2f pos) {
        bounds = sf::FloatRect(pos.x, pos.y, 190.0f, 50.0f);
        sf::RectangleShape button({bounds.width, bounds.height});
        button.setPosition(pos);
        button.setFillColor(sf::Color(178, 226, 178, 230));
        button.setOutlineThickness(2.0f);
        button.setOutlineColor(sf::Color(245, 210, 95));
        m_window.draw(button);
        drawText(label, 20, {pos.x + 22.0f, pos.y + 12.0f}, sf::Color(18, 38, 28));
    };

    drawPanelButton(m_rulesButtonBounds, "Reglas", {panelPos.x + 42.0f, panelPos.y + 178.0f});
    drawPanelButton(m_creditsButtonBounds, "Creditos", {panelPos.x + 252.0f, panelPos.y + 178.0f});

    if (m_menuPanelView == 1) {
        drawText("Reglas", 22, {panelPos.x + 42.0f, panelPos.y + 254.0f}, sf::Color(178, 226, 178));
        drawText("Emboca tu grupo, evita faltas y deja la sandia para el final.", 14, {panelPos.x + 42.0f, panelPos.y + 292.0f}, sf::Color::White);
        drawText("Si se acaba el tiempo, el turno pasa al oponente.", 14, {panelPos.x + 42.0f, panelPos.y + 320.0f}, sf::Color::White);
    } else if (m_menuPanelView == 2) {
        drawText("Creditos", 22, {panelPos.x + 42.0f, panelPos.y + 254.0f}, sf::Color(178, 226, 178));
        drawText("Fruit Pool", 14, {panelPos.x + 42.0f, panelPos.y + 292.0f}, sf::Color::White);
        drawText("Desarrollo, arte y audio del equipo del proyecto.", 14, {panelPos.x + 42.0f, panelPos.y + 320.0f}, sf::Color::White);
    }
}


void Game::drawHUD() {
    const sf::Color panelActiveColor(48, 90, 78, 235);
    const sf::Color panelIdleColor(36, 44, 50, 225);
    const sf::Color hudGreen(178, 226, 178);

    sf::RectangleShape topPanel({static_cast<float>(WINDOW_WIDTH), 118.0f});
    topPanel.setPosition({0.0f, 0.0f});
    topPanel.setFillColor(sf::Color(20, 28, 34, 215));
    m_window.draw(topPanel);

    auto drawText = [&](const std::string& value, unsigned int size, sf::Vector2f pos, sf::Color color) {
        if (!m_hudFontLoaded) {
            return;
        }

        sf::Text text(value, m_hudFont, size);
        text.setPosition(pos);
        text.setFillColor(color);
        text.setStyle(sf::Text::Bold);
        m_window.draw(text);
    };

    auto drawCenteredText = [&](const std::string& value, unsigned int size, sf::Vector2f center, sf::Color color) {
        if (!m_hudFontLoaded) {
            return;
        }

        sf::Text text(value, m_hudFont, size);
        text.setFillColor(color);
        text.setStyle(sf::Text::Bold);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f});
        text.setPosition(center);
        m_window.draw(text);
    };

    auto drawPlayerPanel = [&](int playerIndex, sf::Vector2f pos) {
        bool active = playerIndex == m_currentPlayer && m_phase != GamePhase::GAME_OVER;
        sf::RectangleShape panel({420.0f, 88.0f});
        panel.setPosition(pos);
        panel.setFillColor(active ? panelActiveColor : panelIdleColor);
        panel.setOutlineThickness(2.0f);
        panel.setOutlineColor(active ? hudGreen : sf::Color(95, 105, 110));
        m_window.draw(panel);

        int avatarIndex = std::max(1, std::min(6, m_playerAvatarID[playerIndex])) - 1;
        sf::FloatRect avatarRect(pos.x + 16.0f, pos.y + 12.0f, 64.0f, 64.0f);
        if (m_avatarLoaded[avatarIndex]) {
            sf::Sprite avatar;
            avatar.setTexture(m_avatarTextures[avatarIndex]);
            sf::Vector2u size = m_avatarTextures[avatarIndex].getSize();
            avatar.setScale({avatarRect.width / static_cast<float>(size.x), avatarRect.height / static_cast<float>(size.y)});
            avatar.setPosition({avatarRect.left, avatarRect.top});
            m_window.draw(avatar);
        } else {
            sf::CircleShape placeholder(32.0f);
            placeholder.setPosition({avatarRect.left, avatarRect.top});
            placeholder.setFillColor(playerIndex == 0 ? sf::Color(235, 104, 82) : sf::Color(84, 158, 225));
            placeholder.setOutlineThickness(2.0f);
            placeholder.setOutlineColor(sf::Color::White);
            m_window.draw(placeholder);
        }

        drawText("Jugador " + std::to_string(playerIndex + 1), 24, {pos.x + 98.0f, pos.y + 14.0f}, sf::Color::White);
        drawText("Victorias: " + std::to_string(m_playerWins[playerIndex]), 18, {pos.x + 98.0f, pos.y + 48.0f}, sf::Color(220, 230, 230));
        drawText(getGroupName(m_playerGroups[playerIndex]), 16, {pos.x + 286.0f, pos.y + 50.0f}, hudGreen); 
    };

    drawPlayerPanel(0, {24.0f, 15.0f});
    drawPlayerPanel(1, {1036.0f, 15.0f});

    const sf::Vector2f timerPos(625.0f, 22.0f);
    const sf::Vector2f timerSize(230.0f, 74.0f);
    const sf::Vector2f actionButtonSize(152.0f, 54.0f);
    const float hudGap = 9.0f;

    sf::RectangleShape timerBox(timerSize);
    timerBox.setPosition(timerPos);
    timerBox.setFillColor(panelIdleColor);
    timerBox.setOutlineThickness(2.0f);
    timerBox.setOutlineColor(m_turnTimeRemaining <= 5.0f ? sf::Color(235, 85, 70) : hudGreen);
    m_window.draw(timerBox);

    drawCenteredText("Turno Jugador " + std::to_string(m_currentPlayer + 1), 16, {timerPos.x + timerSize.x * 0.5f, timerPos.y + 20.0f}, hudGreen);
    int seconds = std::max(0, static_cast<int>(std::ceil(m_turnTimeRemaining)));
    drawCenteredText(std::to_string(seconds) + "s", 34, {timerPos.x + timerSize.x * 0.5f, timerPos.y + 51.0f}, m_turnTimeRemaining <= 5.0f ? sf::Color(255, 120, 105) : sf::Color::White);

    m_restartButtonBounds = sf::FloatRect(timerPos.x - hudGap - actionButtonSize.x, 32.0f, actionButtonSize.x, actionButtonSize.y);
    sf::RectangleShape restartButton({m_restartButtonBounds.width, m_restartButtonBounds.height});
    restartButton.setPosition({m_restartButtonBounds.left, m_restartButtonBounds.top});
    restartButton.setFillColor(panelIdleColor);
    restartButton.setOutlineThickness(2.0f);
    restartButton.setOutlineColor(hudGreen);
    m_window.draw(restartButton);
    drawCenteredText("Reiniciar", 18, {m_restartButtonBounds.left + m_restartButtonBounds.width * 0.5f, m_restartButtonBounds.top + m_restartButtonBounds.height * 0.5f}, hudGreen);

    m_backToMenuButtonBounds = sf::FloatRect(timerPos.x + timerSize.x + hudGap, 32.0f, actionButtonSize.x, actionButtonSize.y);
    sf::RectangleShape backButton({m_backToMenuButtonBounds.width, m_backToMenuButtonBounds.height});
    backButton.setPosition({m_backToMenuButtonBounds.left, m_backToMenuButtonBounds.top});
    backButton.setFillColor(panelIdleColor);
    backButton.setOutlineThickness(2.0f);
    backButton.setOutlineColor(hudGreen);
    m_window.draw(backButton);
    drawCenteredText("Menu", 20, {m_backToMenuButtonBounds.left + m_backToMenuButtonBounds.width * 0.5f, m_backToMenuButtonBounds.top + m_backToMenuButtonBounds.height * 0.5f}, hudGreen);

    drawCenteredText(m_statusMessage, 12, {timerPos.x + timerSize.x * 0.5f, 105.0f}, sf::Color(225, 236, 232));//
}


void Game::resetTurnTimer() {
    m_turnTimeRemaining = 30.0f;
    m_turnClock.restart();
}


void Game::restartMatch() {
    m_isAiming = false;
    m_phase = GamePhase::AIMING;
    m_currentPlayer = 0;
    m_isPlayer1Turn = true;
    m_winner = -1;
    m_playerGroups = {FruitGroup::NONE, FruitGroup::NONE};
    m_cueBallPocketedThisShot = false;
    m_eightBallPocketedThisShot = false;
    m_shotPocketedGroups.clear();

    for (Fruit& fruit : m_fruits) {
        b2DestroyBody(fruit.bodyId);
    }
    m_fruits.clear();

    resetCueBall();
    spawnTriangle();
    resetTurnTimer();
    m_statusMessage = "Mesa abierta";
    updateWindowTitle();
}


void Game::updateTurnTimer() {
    float elapsed = m_turnClock.restart().asSeconds();
    if (m_phase != GamePhase::AIMING || !areBallsStopped()) {
        return;
    }

    m_turnTimeRemaining -= elapsed;
    if (m_turnTimeRemaining > 0.0f) {
        return;
    }

    m_isAiming = false;
    switchTurn();
    m_statusMessage = "Tiempo agotado";
    updateWindowTitle();
}


void Game::setMusicVolumeFromMouse(float mouseX) {
    if (m_musicSliderBounds.width <= 0.0f) {
        return;
    }

    float relative = (mouseX - m_musicSliderBounds.left) / m_musicSliderBounds.width;
    relative = std::max(0.0f, std::min(1.0f, relative));
    m_musicVolume = relative * 100.0f;
    if (m_backgroundMusicLoaded) {
        m_backgroundMusic.setVolume(m_musicVolume);
    }
}


void Game::playCueHitSound() {
    if (m_cueHitLoaded) {
        m_cueHitSound.play();
    }
}


void Game::playFruitCollisionSound() {
    if (!m_fruitCollisionLoaded || m_collisionSoundClock.getElapsedTime().asSeconds() < 0.12f) {
        return;
    }

    m_fruitCollisionSound.play();
    m_collisionSoundClock.restart();
}


void Game::checkCollisionAudio() {
    if (!m_fruitCollisionLoaded || m_phase == GamePhase::AIMING) {
        return;
    }

    const float collisionDistance = 32.0f / SCALE;
    const float collisionDistanceSq = collisionDistance * collisionDistance;
    const float velocityThreshold = 3.2f;
    b2Vec2 cuePos = b2Body_GetPosition(m_cueBallId);
    b2Vec2 cueVel = b2Body_GetLinearVelocity(m_cueBallId);

    for (std::size_t i = 0; i < m_fruits.size(); ++i) {
        b2Vec2 fruitPos = b2Body_GetPosition(m_fruits[i].bodyId);
        b2Vec2 fruitVel = b2Body_GetLinearVelocity(m_fruits[i].bodyId);
        float cueDx = cuePos.x - fruitPos.x;
        float cueDy = cuePos.y - fruitPos.y;
        float relCueX = cueVel.x - fruitVel.x;
        float relCueY = cueVel.y - fruitVel.y;
        float cueRelSpeed = std::sqrt(relCueX * relCueX + relCueY * relCueY);
        if ((cueDx * cueDx + cueDy * cueDy) <= collisionDistanceSq && cueRelSpeed >= velocityThreshold) {
            playFruitCollisionSound();
            return;
        }

        for (std::size_t j = i + 1; j < m_fruits.size(); ++j) {
            b2Vec2 otherPos = b2Body_GetPosition(m_fruits[j].bodyId);
            b2Vec2 otherVel = b2Body_GetLinearVelocity(m_fruits[j].bodyId);
            float dx = fruitPos.x - otherPos.x;
            float dy = fruitPos.y - otherPos.y;
            float relX = fruitVel.x - otherVel.x;
            float relY = fruitVel.y - otherVel.y;
            float relSpeed = std::sqrt(relX * relX + relY * relY);
            if ((dx * dx + dy * dy) <= collisionDistanceSq && relSpeed >= velocityThreshold) {
                playFruitCollisionSound();
                return;
            }
        }
    }
}


void Game::updateAnimation() {
    float deltaTime = m_animClock.restart().asSeconds();
    b2Vec2 velocity = b2Body_GetLinearVelocity(m_cueBallId);
    float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);


    if (speed > 0.5f) {
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
    }


    // Sincronizar posición con Box2D
b2Vec2 pos = b2Body_GetPosition(m_cueBallId);
    float SCALE = 30.0f; // Nuestra constante de conversión
    
    m_cocoSprite.setPosition({pos.x * SCALE, pos.y * SCALE});

    for (Fruit& fruit : m_fruits) {
        b2Vec2 fruitVelocity = b2Body_GetLinearVelocity(fruit.bodyId);
        float fruitSpeed = std::sqrt(fruitVelocity.x * fruitVelocity.x + fruitVelocity.y * fruitVelocity.y);
        const FruitSpriteInfo& info = m_fruitSpriteInfo[fruit.type];

        if (fruitSpeed > 0.5f) {
            fruit.frameTime += deltaTime;

            if (fruit.frameTime >= 0.05f) {
                fruit.currentFrame = (fruit.currentFrame + 1) % info.totalFrames;
                fruit.frameTime = 0.0f;
            }
        }

        int row = fruit.currentFrame / info.columns;
        int col = fruit.currentFrame % info.columns;
        fruit.sprite.setTextureRect(sf::IntRect({col * info.frameWidth, row * info.frameHeight}, {info.frameWidth, info.frameHeight}));
    }
}


void Game::spawnTriangle() {
    // Orden de frutas en el triángulo (5 filas: 1, 2, 3, 4, 5 = 15 total)
    std::vector<FruitType> fruitOrder = {
        // Fila 1 (1 fruta): Sandía (bola 8)
        SANDIA,
        // Fila 2 (2 frutas)
        LIMA, LIMON,
        // Fila 3 (3 frutas)
        TORONJA, MANDARINA, NARANJA,
        // Fila 4 (4 frutas)
        GRANADA, KIWI, FRESA, CEREZA,
        // Fila 5 (5 frutas)
        BLACKBERRY, FRAMBUESA, UVA_VERDE, UVA_MORADA, MORA_AZUL
    };


    // Inicio del triángulo
    float SCALE = 30.0f;
    float startX = 995.0f / SCALE;
    float startY = (460.0f + TABLE_OFFSET_Y) / SCALE;
    float radius = 15.0f / SCALE;  // Radio físico real
    
    // 1. EL SECRETO DEL BILLAR VIRTUAL: Un micro-espacio
    float gap = 0.05f / SCALE; // Un hueco invisible de 0.05 píxeles
    float effectiveRadius = radius + gap; 
    float effectiveDiameter = effectiveRadius * 2.0f;
    
    // 2. Fórmula matemática exacta para la separación de un hexágono/triángulo
    float rowSpacing = effectiveDiameter * std::sqrt(3.0f) / 2.0f;


    int fruitIndex = 0;
    for (int row = 0; row < 5; ++row) {
        // Calculamos el centro exacto en Y para cada columna
        float firstY = startY - (row * effectiveRadius); 


        for (int col = 0; col <= row; ++col) {
            float x = startX + (row * rowSpacing);
            float y = firstY + (col * effectiveDiameter);


            b2BodyDef bodyDef = b2DefaultBodyDef();
            bodyDef.type = b2_dynamicBody;
            bodyDef.position = {x, y};
            
            bodyDef.linearDamping = 1.2f; 
            bodyDef.angularDamping = 1.0f;


            b2BodyId fruitId = b2CreateBody(m_worldId, &bodyDef);


            // La forma física se mantiene en su radio original para que la física sea leal
            b2Circle circle = {{0.0f, 0.0f}, radius}; 
            b2ShapeDef shapeDef = b2DefaultShapeDef();
            
            shapeDef.restitution = 0.85f; 
            shapeDef.friction = 0.2f;     
            shapeDef.density = 1.0f;


            b2CreateCircleShape(fruitId, &shapeDef, &circle);


            // Crear la fruta con tipo específico
            Fruit fruit;
            fruit.bodyId = fruitId;
            fruit.type = fruitOrder[fruitIndex++];
            
            // Asignar la textura correspondiente
            if (m_fruitTextures.find(fruit.type) != m_fruitTextures.end()) {
                fruit.sprite.setTexture(m_fruitTextures[fruit.type]);
            }
            
            // Configurar el sprite
            const FruitSpriteInfo& info = m_fruitSpriteInfo[fruit.type];
            fruit.sprite.setTextureRect(sf::IntRect({0, 0}, {info.frameWidth, info.frameHeight}));
            fruit.sprite.setOrigin({info.frameWidth / 2.0f, info.frameHeight / 2.0f});
            fruit.sprite.setScale({34.0f / info.frameWidth, 34.0f / info.frameHeight});
            
            m_fruits.push_back(fruit);
        }
    }
}



void Game::initPockets() {
    float SCALE = 30.0f; 
    m_pockets.clear();


    m_pocketRadius = 33.0f / SCALE; // Radio de atracción del agujero


    // Troneras Superiores (Y = 105)
    m_pockets.push_back({210.0f / SCALE, (185.0f + TABLE_OFFSET_Y) / SCALE});   // Izquierda
    m_pockets.push_back({740.0f / SCALE, (170.0f + TABLE_OFFSET_Y) / SCALE});   // Centro
    m_pockets.push_back({1270.0f / SCALE, (185.0f + TABLE_OFFSET_Y) / SCALE});  // Derecha


    // Troneras Inferiores (Y = 745)
    m_pockets.push_back({210.0f / SCALE, (733.0f + TABLE_OFFSET_Y) / SCALE});   // Izquierda
    m_pockets.push_back({740.0f / SCALE, (750.0f + TABLE_OFFSET_Y) / SCALE});   // Centro
    m_pockets.push_back({1270.0f / SCALE, (733.0f + TABLE_OFFSET_Y) / SCALE});  // Derecha
}


Game::FruitGroup Game::getFruitGroup(FruitType type) const {
    switch (type) {
        case LIMA:
        case LIMON:
        case TORONJA:
        case MANDARINA:
        case NARANJA:
        case GRANADA:
        case KIWI:
            return FruitGroup::STRIPED;
        case FRESA:
        case CEREZA:
        case BLACKBERRY:
        case FRAMBUESA:
        case UVA_VERDE:
        case UVA_MORADA:
        case MORA_AZUL:
            return FruitGroup::SOLID;
        case SANDIA:
        default:
            return FruitGroup::NONE;
    }
}


std::string Game::getGroupName(FruitGroup group) const {
    switch (group) {
        case FruitGroup::SOLID:
            return "citricos";
        case FruitGroup::STRIPED:
            return "bayas";
        case FruitGroup::NONE:
        default:
            return "sin grupo";
    }
}


bool Game::areBallsStopped() const {
    const float stopSpeed = 0.08f;
    b2Vec2 cueVelocity = b2Body_GetLinearVelocity(m_cueBallId);
    if ((cueVelocity.x * cueVelocity.x + cueVelocity.y * cueVelocity.y) > stopSpeed * stopSpeed) {
        return false;
    }

    for (const Fruit& fruit : m_fruits) {
        b2Vec2 velocity = b2Body_GetLinearVelocity(fruit.bodyId);
        if ((velocity.x * velocity.x + velocity.y * velocity.y) > stopSpeed * stopSpeed) {
            return false;
        }
    }

    return true;
}


bool Game::hasClearedGroup(int playerIndex) const {
    FruitGroup group = m_playerGroups[playerIndex];
    if (group == FruitGroup::NONE) {
        return false;
    }

    for (const Fruit& fruit : m_fruits) {
        if (getFruitGroup(fruit.type) == group) {
            return false;
        }
    }

    return true;
}


void Game::switchTurn() {
    m_currentPlayer = 1 - m_currentPlayer;
    m_isPlayer1Turn = (m_currentPlayer == 0);
    resetTurnTimer();
}


void Game::assignGroups(FruitGroup pocketedGroup) {
    if (pocketedGroup == FruitGroup::NONE || m_playerGroups[0] != FruitGroup::NONE) {
        return;
    }

    m_playerGroups[m_currentPlayer] = pocketedGroup;
    m_playerGroups[1 - m_currentPlayer] = (pocketedGroup == FruitGroup::SOLID)
        ? FruitGroup::STRIPED
        : FruitGroup::SOLID;
}


void Game::resetCueBall() {
    b2Body_SetLinearVelocity(m_cueBallId, {0.0f, 0.0f});
    b2Body_SetAngularVelocity(m_cueBallId, 0.0f);
    b2Body_SetTransform(m_cueBallId, {485.0f / SCALE, (460.0f + TABLE_OFFSET_Y) / SCALE}, b2MakeRot(0.0f));
}


void Game::resolveShotIfReady() {
    if (m_phase != GamePhase::BALLS_MOVING || !areBallsStopped()) {
        return;
    }

    const int shooter = m_currentPlayer;
    const int opponent = 1 - m_currentPlayer;

    if (m_eightBallPocketedThisShot) {
        bool legalEight = hasClearedGroup(shooter) && !m_cueBallPocketedThisShot;
        m_winner = legalEight ? shooter : opponent;
        m_playerWins[m_winner]++;
        std::string winnerMessage = legalEight
            ? "Jugador " + std::to_string(shooter + 1) + " gana embocando la sandia."
            : "Jugador " + std::to_string(opponent + 1) + " gana: la sandia cayo antes de tiempo o con falta.";
        std::cout << winnerMessage << std::endl;
        restartMatch();
        m_statusMessage = "Ronda nueva";
        updateWindowTitle();
        return;
    }

    if (!m_cueBallPocketedThisShot && !m_shotPocketedGroups.empty() && m_playerGroups[0] == FruitGroup::NONE) {
        assignGroups(m_shotPocketedGroups.front());
        std::cout << "Grupos asignados: Jugador 1 = " << getGroupName(m_playerGroups[0])
                  << ", Jugador 2 = " << getGroupName(m_playerGroups[1]) << std::endl;
    }

    bool pocketedOwnGroup = false;
    FruitGroup shooterGroup = m_playerGroups[shooter];
    for (FruitGroup group : m_shotPocketedGroups) {
        if ((shooterGroup != FruitGroup::NONE && group == shooterGroup) ||
            (shooterGroup == FruitGroup::NONE && group != FruitGroup::NONE)) {
            pocketedOwnGroup = true;
            break;
        }
    }

    if (m_cueBallPocketedThisShot) {
        switchTurn();
        m_statusMessage = "Falta: cayo el coco";
    } else if (!pocketedOwnGroup) {
        switchTurn();
        m_statusMessage = "No emboco fruta propia";
    } else {
        resetTurnTimer();
        m_statusMessage = "Sigue tirando";
    }

    m_cueBallPocketedThisShot = false;
    m_eightBallPocketedThisShot = false;
    m_shotPocketedGroups.clear();
    m_phase = GamePhase::AIMING;
    updateWindowTitle();
}


void Game::updateWindowTitle() {
    std::ostringstream title;
    title << "Fruit Pool - Fase 8 | ";

    if (m_phase == GamePhase::GAME_OVER) {
        title << "Gana Jugador " << (m_winner + 1);
    } else {
        title << "Turno Jugador " << (m_currentPlayer + 1);
        title << " | J1: " << getGroupName(m_playerGroups[0]);
        title << " | J2: " << getGroupName(m_playerGroups[1]);
        if (m_phase == GamePhase::BALLS_MOVING) {
            title << " | bolas en movimiento";
        }
    }

    title << " | " << m_statusMessage;
    m_window.setTitle(title.str());
}


void Game::checkPockets() {
    float dropDistSq = m_pocketRadius * m_pocketRadius; // Distancia al cuadrado


    // 1. Revisar las frutas (Lisas y Rayadas)
    for (auto it = m_fruits.begin(); it != m_fruits.end(); ) {
        b2Vec2 pos = b2Body_GetPosition(it->bodyId);
        bool pocketed = false;


        for (const auto& pocket : m_pockets) {
            float dx = pos.x - pocket.x;
            float dy = pos.y - pocket.y;
            
            // Si la distancia al cuadrado es menor que el radio de caída al cuadrado
            if ((dx * dx + dy * dy) < dropDistSq) {
                pocketed = true;
                break;
            }
        }


        if (pocketed) {
            FruitGroup pocketedGroup = getFruitGroup(it->type);
            if (it->type == SANDIA) {
                m_eightBallPocketedThisShot = true;
            } else {
                m_shotPocketedGroups.push_back(pocketedGroup);
            }

            // Destruir el cuerpo físico en Box2D
            b2DestroyBody(it->bodyId);
            // Quitarlo de nuestra lista para no dibujarlo más
            it = m_fruits.erase(it); 
        } else {
            ++it;
        }
    }


    // 2. Revisar la bola blanca (El Coco)
    b2Vec2 cuePos = b2Body_GetPosition(m_cueBallId);
    for (const auto& pocket : m_pockets) {
        float dx = cuePos.x - pocket.x;
        float dy = cuePos.y - pocket.y;
        
        if ((dx * dx + dy * dy) < dropDistSq) {
            // ¡Falta! El Coco entró a la tronera. 
            // Detenemos su movimiento y lo regresamos al punto de saque.
            m_cueBallPocketedThisShot = true;
            resetCueBall();
            break; // Romper el ciclo porque ya sabemos que cayó
        }
    }
   
}




