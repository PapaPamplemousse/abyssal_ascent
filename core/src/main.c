#include "game.h"
#include <stdio.h>
int main(void) {
    // On active la synchronisation verticale et on autorise le redimensionnement
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    
    // On initialise une fenêtre de base
    InitWindow(1280, 720, "Abyssal Ascent - Alpha");
    
    // On récupère la taille de l'écran principal et on ajuste la fenêtre
    int monitor = GetCurrentMonitor();
    SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
    ToggleFullscreen(); // Passage en plein écran
    
    SetTargetFPS(60);

    GameContext game;
    Game_Init(&game);
    printf("Game initialized\n");
    Game_Run(&game);
    
    Game_Close(&game);
    CloseWindow(); 
    
    return 0;
}