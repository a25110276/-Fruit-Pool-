#include "Game.hpp"
#include <iostream> 
#include <cmath>
#include <limits>


static const unsigned int WINDOW_WIDTH = 1480;
static const unsigned int WINDOW_HEIGHT = 920;
static const float WINDOW_CENTER_X = WINDOW_WIDTH / 2.0f;
static const float WINDOW_CENTER_Y = WINDOW_HEIGHT / 2.0f;


// NOTA: Este código asume que tienes las imágenes "mantel.jpg", "coco.png" y "taco.png" en la carpeta "assets/images/" de tu proyecto.
Game::Game() : m_window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Fruit Pool - Fase 5") {
    m_window.setFramerateLimit(60);
    loadAssets();
    initPhysics();// 1. Inicializar el mundo físico de Box2D y sus bandas
    spawnTriangle(); // Llenamos la mesa con el triángulo de frutas al iniciar
    initPockets();   // Configuramos las posiciones de las troneras


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
    bodyDef.position = {485.0f / SCALE, 460.0f / SCALE}; // Posición inicial del Coco
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


                // Creamos un vector de fuerza. Multiplicamos por 0.40f para mucho más potencia.
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
    // Calculamos el siguiente frame físico
    updateAnimation();
    b2World_Step(m_worldId, 1.0f / 60.0f, 4);
    checkPockets();
    
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
            sf::Vector2f normal = normalize(collisionPoint - center);
            reboundDir = normal;
        }
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
    m_window.draw(m_tableSprite);   // Mantel centrado detrás del marco
    m_window.draw(m_frameSprite);   // Marco encima del mantel


    m_window.draw(m_cocoSprite);    // Luego las frutas


    // Dibuja el resto de las frutas de la mesa
    float SCALE = 30.0f;
    for (Fruit& fruit : m_fruits) {
        b2Vec2 pos = b2Body_GetPosition(fruit.bodyId);
        fruit.sprite.setPosition({pos.x * SCALE, pos.y * SCALE});
        m_window.draw(fruit.sprite);
    }
   
// NUEVO: Apuntado avanzado con retroceso y láser grueso
if (m_isAiming) {
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


        sf::Vector2f collisionPoint;
        sf::Vector2f reboundDir;
        b2BodyId hitId;
        bool hasCollision = predictBallCollision(origin, aimDir, collisionPoint, reboundDir, hitId);
        if (hasCollision) {
            aimEnd = collisionPoint;
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


        // 5. Dibujar la Caña de Azúcar con Retroceso
        float stickAngle = std::atan2(-pullY, -pullX) * 180.0f / 3.14159265f;
        m_cueSprite.setPosition({pixelX - pullX, pixelY - pullY});
        m_cueSprite.setRotation(stickAngle);
        m_window.draw(m_cueSprite);


        float powerPercentage = std::min(pullDist / maxPull, 1.0f); // Valor entre 0 y 1


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
    m_window.display();
}


void Game::createWall(float x, float y, float width, float height, float angleDegrees) {
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
    bool backgroundLoaded = m_backgroundTexture.loadFromFile("assets/images/fondo.png");
    if (backgroundLoaded) {
        m_backgroundSprite.setTexture(m_backgroundTexture);
        m_backgroundSprite.setOrigin({m_backgroundTexture.getSize().x / 2.0f, m_backgroundTexture.getSize().y / 2.0f});
        m_backgroundSprite.setPosition({WINDOW_CENTER_X, WINDOW_CENTER_Y});
        m_backgroundSprite.setScale({WINDOW_WIDTH / static_cast<float>(m_backgroundTexture.getSize().x), WINDOW_HEIGHT / static_cast<float>(m_backgroundTexture.getSize().y)});
    }


    // 1.5. Cargar el Marco
    if (!m_frameTexture.loadFromFile("assets/images/marco.png")) {
        // Manejo de error básico
    }
    m_frameSprite.setTexture(m_frameTexture);
    // Configurar el origen del marco al centro de la imagen
    m_frameSprite.setOrigin(m_frameTexture.getSize().x / 2.0f, m_frameTexture.getSize().y / 2.0f);
    // Posicionar el marco exactamente en el centro de la ventana (mismo que el mantel)
    m_frameSprite.setPosition(WINDOW_CENTER_X, WINDOW_CENTER_Y);
    // No escalamos el marco, se dibuja en su tamaño original (1194x683)


    // 2. Cargar el Mantel
    if (!m_tableTexture.loadFromFile("assets/images/mantel.jpg")) {
        // Manejo de error básico
    }
    m_tableSprite.setTexture(m_tableTexture);
    // 1. Configurar el origen del mantel al centro de su imagen real
    m_tableSprite.setOrigin(m_tableTexture.getSize().x / 2.0f, m_tableTexture.getSize().y / 2.0f);
    // 2. Posicionar el mantel exactamente en el centro de la ventana
    m_tableSprite.setPosition(WINDOW_CENTER_X, WINDOW_CENTER_Y);
    // 3. Usar el tamaño real del mantel.jpg sin escalar
    m_tableSprite.setScale(1.0f, 1.0f);


    // 4. Cargar el Spritesheet del Coco
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
    m_fruitTextures[SANDIA].loadFromFile("assets/images/sandia.png");
    m_fruitTextures[LIMA].loadFromFile("assets/images/lima.png");
    m_fruitTextures[LIMON].loadFromFile("assets/images/limon.png");
    m_fruitTextures[TORONJA].loadFromFile("assets/images/toronja.png");
    m_fruitTextures[MANDARINA].loadFromFile("assets/images/mandarina.png");
    m_fruitTextures[NARANJA].loadFromFile("assets/images/naranja.png");
    m_fruitTextures[GRANADA].loadFromFile("assets/images/granada.png");
    m_fruitTextures[KIWI].loadFromFile("assets/images/kiwi.png");
    m_fruitTextures[FRESA].loadFromFile("assets/images/fresa.png");
    m_fruitTextures[CEREZA].loadFromFile("assets/images/cereza.png");
    m_fruitTextures[BLACKBERRY].loadFromFile("assets/images/blackberry.png");
    m_fruitTextures[FRAMBUESA].loadFromFile("assets/images/frambuesa.png");
    m_fruitTextures[UVA_VERDE].loadFromFile("assets/images/uva_verde.png");
    m_fruitTextures[UVA_MORADA].loadFromFile("assets/images/uva_morada.png");
    m_fruitTextures[MORA_AZUL].loadFromFile("assets/images/mora_azul.png");


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
    float startY = 460.0f / SCALE;
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
                fruit.texture = m_fruitTextures[fruit.type];
                fruit.sprite.setTexture(fruit.texture);
            }
            
            // Configurar el sprite
            const FruitSpriteInfo& info = m_fruitSpriteInfo[fruit.type];
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
    m_pockets.push_back({210.0f / SCALE, 185.0f / SCALE});   // Izquierda
    m_pockets.push_back({740.0f / SCALE, 170.0f / SCALE});   // Centro
    m_pockets.push_back({1270.0f / SCALE, 185.0f / SCALE});  // Derecha


    // Troneras Inferiores (Y = 745)
    m_pockets.push_back({210.0f / SCALE, 733.0f / SCALE});   // Izquierda
    m_pockets.push_back({740.0f / SCALE, 750.0f / SCALE});   // Centro
    m_pockets.push_back({1270.0f / SCALE, 733.0f / SCALE});  // Derecha
}


void Game::checkPockets() {
    float SCALE = 30.0f;
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
            b2Body_SetLinearVelocity(m_cueBallId, {0.0f, 0.0f});
            b2Body_SetAngularVelocity(m_cueBallId, 0.0f);
            b2Body_SetTransform(m_cueBallId, {485.0f / SCALE, 460.0f / SCALE}, b2MakeRot(0.0f));
            break; // Romper el ciclo porque ya sabemos que cayó
        }
    }
   
}


