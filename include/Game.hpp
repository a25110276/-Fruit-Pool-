
#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <box2d/box2d.h>
#include <array>
#include <vector>
#include <map>
#include <string>


// Enumeración de tipos de frutas
enum FruitType {
    SANDIA,       // Bola 8
    LIMA,         // Rallada
    LIMON,        // Rallada
    TORONJA,      // Rallada
    MANDARINA,    // Rallada
    NARANJA,      // Rallada
    GRANADA,      // Rallada
    KIWI,         // Rallada
    FRESA,        // Lisa
    CEREZA,       // Lisa
    BLACKBERRY,   // Lisa
    FRAMBUESA,    // Lisa
    UVA_VERDE,    // Lisa
    UVA_MORADA,   // Lisa
    MORA_AZUL     // Lisa
};

// Estructura para información de spritesheet de una fruta
struct FruitSpriteInfo {
    int frameWidth;
    int frameHeight;
    int totalFrames;
    int columns;
};

// Estructura para una fruta en el juego
struct Fruit {
    b2BodyId bodyId;
    FruitType type;
    sf::Sprite sprite;
    int currentFrame = 0;
    float frameTime = 0.1f;
};

class Game {
public:
    Game();
    ~Game(); // Importante: liberar la memoria del mundo físico
    void run();


private:
    enum class FruitGroup {
        NONE,
        SOLID,
        STRIPED
    };

    enum class GamePhase {
        AIMING,
        BALLS_MOVING,
        GAME_OVER
    };

    void processEvents();
    void update();
    void render();
    void initPhysics();


    // --- Variables de Troneras ---
std::vector<b2Vec2> m_pockets;
float m_pocketRadius;


void initPockets(); // Configura las posiciones de las troneras
void checkPockets(); // Revisa si alguna bola ha caído en una tronera y la elimina


// --- Variables del Triángulo de Frutas ---
std::vector<Fruit> m_fruits;
std::map<FruitType, FruitSpriteInfo> m_fruitSpriteInfo;
void spawnTriangle();


    // NUEVO: Función modular para crear muros invisibles
    void createWall(float x, float y, float width, float height, float angleDegrees = 0.0f);


    struct WallRender { float x, y, w, h, angle; };
    std::vector<WallRender> m_wallRenders;
    
    bool m_isAiming = false; // Variables para la mecánica del taco
    sf::Vector2f m_mouseStartPos;
    sf::RenderWindow m_window;
    
    // En Box2D v3, usamos "IDs" en lugar de punteros a clases
    b2WorldId m_worldId;
    b2BodyId m_cueBallId; 


    // Constante de conversión: 30 píxeles equivalen a 1 metro físico
    const float SCALE = 30.0f;


// --- Assets Visuales ---
sf::Texture m_backgroundTexture;
sf::Sprite m_backgroundSprite;
sf::Texture m_menuTexture;
sf::Sprite m_menuSprite;
sf::Texture m_frameTexture;
sf::Sprite m_frameSprite;
sf::Texture m_tableTexture;
sf::Texture m_cocoTexture;
sf::Sprite m_tableSprite;
sf::Sprite m_cocoSprite;


// --- Assets del Taco ---
sf::Texture m_cueTexture;
sf::Sprite m_cueSprite;

// --- Assets de Frutas ---
std::map<FruitType, sf::Texture> m_fruitTextures;

// --- Lógica de Animación ---
int m_currentFrame = 0;
float m_frameTime = 0.1f;
sf::Clock m_animClock;


    // --- Configuración del Spritesheet del Coco ---
const int FRAME_WIDTH = 100;  // Ancho de un solo frame
const int FRAME_HEIGHT = 100; // Alto de un solo frame
const int TOTAL_FRAMES = 35;  // Total de cuadros en la imagen
const int COLUMNS = 7;        // 700 px totales / 100 px por frame


// Nuevas funciones
void loadAssets();
void drawMainMenu();
void drawHUD();
void updateAnimation();
void updateTurnTimer();
void playCueHitSound();
void playFruitCollisionSound();
void checkCollisionAudio();
void resolveShotIfReady();
bool areBallsStopped() const;
bool hasClearedGroup(int playerIndex) const;
void switchTurn();
void resetTurnTimer();
void assignGroups(FruitGroup pocketedGroup);
void resetCueBall();
FruitGroup getFruitGroup(FruitType type) const;
std::string getGroupName(FruitGroup group) const;
void updateWindowTitle();


    // --- Ayudas de apuntado y predicción ---
    sf::Vector2f normalize(const sf::Vector2f& vector) const;
    float dot(const sf::Vector2f& a, const sf::Vector2f& b) const;
    bool predictBallCollision(const sf::Vector2f& origin,
                              const sf::Vector2f& direction,
                              sf::Vector2f& collisionPoint,
                              sf::Vector2f& fruitCenter,
                              sf::Vector2f& reboundDir,
                              b2BodyId& hitId) const;
    bool predictWallCollision(const sf::Vector2f& origin,
                              const sf::Vector2f& direction,
                              sf::Vector2f& collisionPoint) const;
    std::vector<std::pair<sf::Vector2f, sf::Vector2f>> predictTrajectory(const sf::Vector2f& origin, const sf::Vector2f& direction, int maxBounces = 5) const;

    // --- Fase 7: Reglas y maquina de estados ---
    bool m_showMainMenu = true;
    sf::FloatRect m_playButtonBounds;
    GamePhase m_phase = GamePhase::AIMING;
    int m_currentPlayer = 0;
    bool m_isPlayer1Turn = true;
    int m_winner = -1;
    std::array<FruitGroup, 2> m_playerGroups = {FruitGroup::NONE, FruitGroup::NONE};
    bool m_cueBallPocketedThisShot = false;
    bool m_eightBallPocketedThisShot = false;
    std::vector<FruitGroup> m_shotPocketedGroups;
    std::string m_statusMessage = "Mesa abierta - Turno del Jugador 1";

    // --- Fase 8: HUD ---
    sf::Font m_hudFont;
    bool m_hudFontLoaded = false;
    std::array<sf::Texture, 6> m_avatarTextures;
    std::array<bool, 6> m_avatarLoaded = {false, false, false, false, false, false};
    std::array<int, 2> m_playerAvatarID = {1, 2};
    std::array<int, 2> m_playerWins = {0, 0};
    sf::Clock m_turnClock;
    float m_turnTimeRemaining = 30.0f;

    // --- Fase 8: Audio ---
    sf::SoundBuffer m_cueHitBuffer;
    sf::SoundBuffer m_fruitCollisionBuffer;
    sf::Sound m_cueHitSound;
    sf::Sound m_fruitCollisionSound;
    bool m_cueHitLoaded = false;
    bool m_fruitCollisionLoaded = false;
    sf::Clock m_collisionSoundClock;
};
