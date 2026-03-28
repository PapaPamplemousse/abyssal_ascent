#ifndef DUNGEON_H
#define DUNGEON_H

#include "game.h"

#define MAP_WIDTH 21
#define MAP_HEIGHT 21
#ifndef MAX_ASCII_LINES
#define MAX_ASCII_LINES 60
#endif
typedef enum
{
    DIR_NORTH,
    DIR_EAST,
    DIR_SOUTH,
    DIR_WEST
} Direction;
typedef enum
{
    ROOM_NORMAL,
    ROOM_BOSS,
    ROOM_EVENT
} RoomType;

// Template JSON pour les salles d'événements
typedef struct
{
    char id[32];
    char name_en[32];
    char name_fr[32];
    char type[32]; // Ex: "HEAL", "GIVE_POTION"
    int  amount;
    char flavor_en[128];
    char flavor_fr[128];

    char ascii[MAX_ASCII_LINES][128];
    int  ascii_line_count;
} EventRoomTemplate;

typedef struct
{
    char      map[MAP_HEIGHT][MAP_WIDTH];
    int       playerX;
    int       playerY;
    Direction playerDir;
    bool      explored[MAP_HEIGHT][MAP_WIDTH];
    int       floor_level;
    int       highest_floor;

    RoomType          room_type;
    EventRoomTemplate current_event;
} DungeonContext;


// Bases de données globales
#define MAX_AMBIANCE 20
#define MAX_EVENTS 20


extern char g_ambiance_en[MAX_AMBIANCE][128];
extern char g_ambiance_fr[MAX_AMBIANCE][128];
extern int  g_ambianceCount;

extern EventRoomTemplate g_eventDB[MAX_EVENTS];
extern int               g_eventCount;


void Dungeon_Init(DungeonContext* dungeon);
void Dungeon_Enter(DungeonContext* dungeon);
void Dungeon_Generate(DungeonContext* dungeon);
void Dungeon_Update(GameContext* game, DungeonContext* dungeon, int key);
void Dungeon_Render(DungeonContext* dungeon, Font uiFont, Font dungeonFont, int screenWidth, int screenHeight);

void DrawTextCentered(Font font, const char* text, int centerX, int y, int fontSize, int spacing, Color color);
#endif // DUNGEON_H