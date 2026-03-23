#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "raylib.h" 
#include "../../clicker/inc/clicker.h"
#include "../../combat/inc/combat.h"

typedef enum {
    STATE_MENU,
    STATE_CAMP,
    STATE_MINE,    
    STATE_FOREST,
    STATE_FORGE,      
    STATE_ALCHEMIST,  
    STATE_ARCHIFORGE,   
    STATE_DUNGEON,
    STATE_GAMEOVER
} GameState;


typedef struct {
    GameState currentState;
    bool isRunning;
    Font uiFont;    
    Font dungeonFont; 
    ClickerContext clicker;
    CombatContext combat;
} GameContext;

void Game_Init(GameContext* game);
void Game_Run(GameContext* game);
void Game_Update(GameContext* game);
void Game_Render(GameContext* game);
void Game_Close(GameContext* game);

#endif // GAME_H