#include "dungeon.h"
#include <stdio.h>

void Dungeon_Generate(DungeonContext* dungeon) {
    // 1. On remplit tout de murs ('#')
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            dungeon->map[y][x] = '#';
        }
    }

    int startX = MAP_WIDTH / 2;
    int startY = MAP_HEIGHT / 2;
    dungeon->playerX = startX;
    dungeon->playerY = startY;
    dungeon->playerDir = DIR_NORTH;

    // 2. Si c'est un étage de BOSS (Multiple de 10)
    if (dungeon->floor_level % 10 == 0) {
        // Crée une arène de 5x5
        for (int y = startY - 3; y <= startY + 1; y++) {
            for (int x = startX - 2; x <= startX + 2; x++) {
                dungeon->map[y][x] = '.';
            }
        }
        // Place le joueur en bas et le Boss en haut
        dungeon->playerY = startY + 1;
        dungeon->map[startY - 2][startX] = 'B'; // 'B' = Boss
        return;
    }

    // 3. Sinon, Génération Procédurale (Drunkard's Walk)
    int x = startX;
    int y = startY;
    dungeon->map[y][x] = '.';
    
    int maxTiles = (MAP_WIDTH * MAP_HEIGHT) / 4; // Environ 110 cases de vide
    int tilesCreated = 1;

    while (tilesCreated < maxTiles) {
        int dir = GetRandomValue(0, 3);
        if (dir == 0 && y > 2) y--;
        else if (dir == 1 && y < MAP_HEIGHT - 3) y++;
        else if (dir == 2 && x > 2) x--;
        else if (dir == 3 && x < MAP_WIDTH - 3) x++;

        if (dungeon->map[y][x] == '#') {
            dungeon->map[y][x] = '.';
            tilesCreated++;
        }
    }
    
    // Le dernier point creusé devient l'escalier '>'
    dungeon->map[y][x] = '>';
}

void Dungeon_Init(DungeonContext* dungeon) {
    dungeon->floor_level = 1;
    Dungeon_Generate(dungeon);
}

void Dungeon_Update(GameContext* game, DungeonContext* dungeon, int key) {
    int nextX = dungeon->playerX;
    int nextY = dungeon->playerY;
    bool hasMoved = false; // <-- LA CORRECTION EST ICI

    if (key == KEY_Z || key == KEY_W) {
        if (dungeon->playerDir == DIR_NORTH) nextY--;
        else if (dungeon->playerDir == DIR_SOUTH) nextY++;
        else if (dungeon->playerDir == DIR_EAST) nextX++;
        else if (dungeon->playerDir == DIR_WEST) nextX--;
        hasMoved = true; // Le joueur a tenté d'avancer
    } 
    else if (key == KEY_Q || key == KEY_A) {
        dungeon->playerDir = (dungeon->playerDir + 3) % 4; 
        return; 
    }
    else if (key == KEY_D) {
        dungeon->playerDir = (dungeon->playerDir + 1) % 4; 
        return;
    }
    else if (key == KEY_F) {
        game->currentState = STATE_CAMP;
        return;
    }

    // On ne vérifie la case QUE si le joueur s'est physiquement déplacé
    if (hasMoved && nextX >= 0 && nextX < MAP_WIDTH && nextY >= 0 && nextY < MAP_HEIGHT) {
        char nextTile = dungeon->map[nextY][nextX];
        
        if (nextTile == '.' || nextTile == '>' || nextTile == 'B') {
            // Le joueur avance sur la nouvelle case
            dungeon->playerX = nextX;
            dungeon->playerY = nextY;

            // Interactions avec la case
            if (nextTile == '>') {
                dungeon->floor_level++;
                Dungeon_Generate(dungeon);
                Combat_AddLog(&game->combat, "Vous descendez d'un etage...");
            }
            else if (nextTile == 'B') {
                Combat_StartEncounter(&game->combat, MONSTER_BOSS_SKELETON_KING);
                dungeon->map[nextY][nextX] = '.'; // Le boss disparait de la map
                dungeon->map[nextY-1][nextX] = '>'; // Ouvre l'escalier derriere lui
            }
            else if (nextTile == '.') {
                // Rencontre aléatoire normale (15% de chance) uniquement lors d'un pas
                if (GetRandomValue(1, 100) > 85) {
                    MonsterType randomMob = (MonsterType)GetRandomValue(MONSTER_RAT, MONSTER_ZOMBIE);
                    Combat_StartEncounter(&game->combat, randomMob);
                }
            }
        }
    }
}

char GetTileAhead(DungeonContext* dungeon, int distance) {
    int checkX = dungeon->playerX;
    int checkY = dungeon->playerY;

    if (dungeon->playerDir == DIR_NORTH) checkY -= distance;
    else if (dungeon->playerDir == DIR_SOUTH) checkY += distance;
    else if (dungeon->playerDir == DIR_EAST) checkX += distance;
    else if (dungeon->playerDir == DIR_WEST) checkX -= distance;

    if (checkX >= 0 && checkX < MAP_WIDTH && checkY >= 0 && checkY < MAP_HEIGHT)
        return dungeon->map[checkY][checkX];
    return '#';
}

void DrawTextCentered(Font font, const char* text, int centerX, int y, int fontSize, int spacing, Color color) {
    Vector2 textSize = MeasureTextEx(font, text, (float)fontSize, (float)spacing);
    Vector2 position = { centerX - (textSize.x / 2.0f), (float)y };
    DrawTextEx(font, text, position, (float)fontSize, (float)spacing, color);
}

void Dungeon_Render(DungeonContext* dungeon, Font uiFont, Font dungeonFont, int screenWidth, int screenHeight) {
    char dist1 = GetTileAhead(dungeon, 1);
    char dist2 = GetTileAhead(dungeon, 2);
    char dist3 = GetTileAhead(dungeon, 3);

    int viewStartX = (int)(screenWidth * 0.20f);
    int viewWidth = (int)(screenWidth * 0.55f);
    int centerX = viewStartX + (viewWidth / 2);
    
    char title[64];
    if (dungeon->floor_level % 10 == 0) sprintf(title, "=== ANTRE DU BOSS (Etage %d) ===", dungeon->floor_level);
    else sprintf(title, "=== DONJON PROFOND (Etage %d) ===", dungeon->floor_level);
    
    DrawTextCentered(uiFont, title, centerX, 50, 40, 1, RED);

    int startY = (int)(screenHeight * 0.25f);
    int fontSize = 50; 
    int spacing = 1; 

    // --- DESSIN DES MURS (dungeonFont) ---
    if (dist1 == '#') {
        DrawTextCentered(dungeonFont, "  ███████████████████  ", centerX, startY, fontSize, spacing, WHITE);
        DrawTextCentered(dungeonFont, "  █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█  ", centerX, startY + fontSize, fontSize, spacing, WHITE);
        DrawTextCentered(dungeonFont, "  █▓  ▓▓▓▓▓▓▓▓▓  ▓█  ", centerX, startY + (fontSize*2), fontSize, spacing, WHITE);
        DrawTextCentered(dungeonFont, "  █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█  ", centerX, startY + (fontSize*3), fontSize, spacing, WHITE);
        DrawTextCentered(dungeonFont, "  ███████████████████  ", centerX, startY + (fontSize*4), fontSize, spacing, WHITE);
    } else if (dist2 == '#') {
        DrawTextCentered(dungeonFont, "   ▓\\            /▓   ", centerX, startY, fontSize, spacing, GRAY);
        DrawTextCentered(dungeonFont, "   ▓▓\\████████/▓▓   ", centerX, startY + fontSize, fontSize, spacing, GRAY);
        DrawTextCentered(dungeonFont, "   ▓▓ |▓▓▓▓▓▓| ▓▓   ", centerX, startY + (fontSize*2), fontSize, spacing, GRAY);
        DrawTextCentered(dungeonFont, "   ▓▓/████████\\▓▓   ", centerX, startY + (fontSize*3), fontSize, spacing, GRAY);
        DrawTextCentered(dungeonFont, "   ▓/            \\▓   ", centerX, startY + (fontSize*4), fontSize, spacing, GRAY);
    } else if (dist3 == '#') {
        DrawTextCentered(dungeonFont, "    ░\\          /░    ", centerX, startY, fontSize, spacing, DARKGRAY);
        DrawTextCentered(dungeonFont, "    ░▓\\██████/▓░    ", centerX, startY + fontSize, fontSize, spacing, DARKGRAY);
        DrawTextCentered(dungeonFont, "    ░ |▓▓▓▓▓| ░    ", centerX, startY + (fontSize*2), fontSize, spacing, DARKGRAY);
        DrawTextCentered(dungeonFont, "    ░▓/██████\\▓░    ", centerX, startY + (fontSize*3), fontSize, spacing, DARKGRAY);
        DrawTextCentered(dungeonFont, "    ░/          \\░    ", centerX, startY + (fontSize*4), fontSize, spacing, DARKGRAY);
    } else {
        DrawTextCentered(dungeonFont, "     .            .     ", centerX, startY, fontSize, spacing, DARKGRAY);
        DrawTextCentered(dungeonFont, "       .        .       ", centerX, startY + fontSize, fontSize, spacing, DARKGRAY);
        DrawTextCentered(dungeonFont, "                       ", centerX, startY + (fontSize*2), fontSize, spacing, DARKGRAY);
        DrawTextCentered(dungeonFont, "       .        .       ", centerX, startY + (fontSize*3), fontSize, spacing, DARKGRAY);
        DrawTextCentered(dungeonFont, "     .            .     ", centerX, startY + (fontSize*4), fontSize, spacing, DARKGRAY);
    }

    // --- DESSIN DES OBJETS EN SURIMPRESSION (uiFont) ---
    // On dessine l'escalier ou le boss par-dessus le couloir vide
    if (dist1 == '>') DrawTextCentered(uiFont, "[ ESCALIER ]", centerX, startY + (fontSize*2), 30, 1, YELLOW);
    else if (dist2 == '>') DrawTextCentered(uiFont, "[ escalier ]", centerX, startY + (fontSize*2), 20, 1, GRAY);
    
    if (dist1 == 'B') DrawTextCentered(uiFont, " ☠ BOSS ☠ ", centerX, startY + (fontSize*2), 50, 1, RED);
    else if (dist2 == 'B') DrawTextCentered(uiFont, " ☠ ", centerX, startY + (fontSize*2), 30, 1, DARKGRAY);

    DrawTextCentered(uiFont, "[Z] Avancer | [Q] Gauche | [D] Droite", centerX, screenHeight - 120, 24, 1, LIGHTGRAY);
    DrawTextCentered(uiFont, "[F] Fuir vers le campement", centerX, screenHeight - 80, 24, 1, RED);
}