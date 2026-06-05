#include "Game.hpp"

// Inicializamos la ventana a 1280x720 como sugiere la plantilla [cite: 35, 67]
Game::Game() : m_window(sf::VideoMode(1280, 720), "Fruit Pool - Fase 1") {
    m_window.setFramerateLimit(60); // Estabilizar el renderizado
}

void Game::run() {
    // El Core Loop: Eventos -> Actualización lógica -> Dibujo
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
    // Aquí implementaremos Box2D en la Fase 2
}

void Game::render() {
    // Limpiamos con un color azul similar al tapete de billar
    m_window.clear(sf::Color(20, 80, 150));
    
    // Aquí dibujaremos los sprites de las frutas
    
    m_window.display();
}