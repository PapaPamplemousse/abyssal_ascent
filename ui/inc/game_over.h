#ifndef GAME_OVER_H
#define GAME_OVER_H

#include "game.h"
#include "dungeon.h"

// Affiche l'écran de mort et gère le clic pour retourner au camp
void GameOver_Render(GameContext* game, DungeonContext* dungeon, int w, int h);

#endif // GAME_OVER_H