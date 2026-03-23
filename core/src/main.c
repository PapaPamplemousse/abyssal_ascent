#include "game.h"
#include <stdio.h>
#include "../../utils/inc/lang.h"
#include "../../combat/inc/combat.h"
#include "../../dungeon/inc/dungeon.h"

int main(void)
{
    // On active la synchronisation verticale et on autorise le redimensionnement
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);

    // On initialise une fenêtre de base
    InitWindow(1280, 720, "Abyssal Ascent - Alpha");

    // On récupère la taille de l'écran principal et on ajuste la fenêtre
    int monitor = GetCurrentMonitor();
    SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
    ToggleFullscreen(); // Passage en plein écran

    SetTargetFPS(60);

    Lang_Init("assets/data/lang.json");
    LoadMonstersDB("assets/data/monsters.json");
    LoadDungeonDB("assets/data/ambiance.json", "assets/data/rooms.json");
    LoadItemsDB("assets/data/items.json");
    LoadMagicDB("assets/data/spells.json", "assets/data/potions.json");

    GameContext game;
    Game_Init(&game);
    printf("Game initialized\n");
    Game_Run(&game);

    Game_Close(&game);
    CloseWindow();

    return 0;
}