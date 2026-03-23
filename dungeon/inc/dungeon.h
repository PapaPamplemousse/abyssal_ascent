#ifndef DUNGEON_H
#define DUNGEON_H

#include "../../core/inc/game.h"

#define MAP_WIDTH 21
#define MAP_HEIGHT 21

typedef enum { DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST } Direction;

typedef struct {
    char map[MAP_HEIGHT][MAP_WIDTH];
    int playerX;
    int playerY;
    Direction playerDir;
    int floor_level; // <-- NOUVEAU : L'étage actuel
} DungeonContext;

void Dungeon_Init(DungeonContext* dungeon);
void Dungeon_Generate(DungeonContext* dungeon); // <-- NOUVEAU : Le générateur
void Dungeon_Update(GameContext* game, DungeonContext* dungeon, int key);
void Dungeon_Render(DungeonContext* dungeon, Font uiFont, Font dungeonFont, int screenWidth, int screenHeight);

#endif // DUNGEON_H