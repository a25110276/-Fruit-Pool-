# -Fruit-Pool 🥥🍉-

Este proyecto es una adaptación temática del clásico Billar Bola 8, donde las tradicionales bolas son reemplazadas por coloridas frutas tropicales. Desarrollado en C++ utilizando SFML para el renderizado y Box2D para físicas precisas.

## 🎯 Objetivo del Juego
Ser el primer jugador en embocar todas las frutas de tu grupo asignado (Lisas/Citricos o Rayadas/bayas) y, finalmente, embocar la "Sandía" (Bola 8) para ganar la partida.

## 🎮 Controles
* **Mouse (Movimiento):** Apuntar el taco.
* **Click Izquierdo (Mantener):** Cargar fuerza de tiro.
* **Click Izquierdo (Soltar):** Golpear el "Coco" (Bola Blanca).

## ⚙ Mecánicas Principales
* **Físicas Realistas:** Colisiones y rebotes implementados con Box2D, simulando el peso y fricción de las bolas de billar estándar (156-170g).
* **Reglas de Bola 8:** Implementación de turnos, faltas (ej. meter el Coco), mesa abierta y victoria/derrota instantánea al embocar la Sandía.

## 🛠 Tecnologías
* **Lenguaje:** C++17
* **Librerías:** SFML (Gráficos/Audio), Box2D (Física)
* **Compilación:** Makefile con MinGW64/MSYS2