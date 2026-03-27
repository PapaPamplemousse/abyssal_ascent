#include "dungeon.h"
#include <stdio.h>
#include <string.h>
#include "lang.h"
#include "cJSON.h"
#include "combat.h"

// Bases de données globales
#define MAX_AMBIANCE 20
#define MAX_EVENTS 20

extern PotionTemplate g_potionDB[MAX_POTIONS_DB];
extern int            g_potionCount;
extern int            g_itemCount;

char g_ambiance_en[MAX_AMBIANCE][128];
char g_ambiance_fr[MAX_AMBIANCE][128];
int  g_ambianceCount = 0;

EventRoomTemplate g_eventDB[MAX_EVENTS];
int               g_eventCount = 0;

static void Dungeon_UpdateFog(DungeonContext* dungeon, int fog_bonus);




// --- LA FORMULE DE CHECKPOINT ---
void Dungeon_Enter(DungeonContext* dungeon)
{
    // La formule : a1 = 1, an = 10(n-1) -> ex: 1, 10, 20...
    int checkpoint = (dungeon->highest_floor >= 10) ? (dungeon->highest_floor / 10) * 10 : 1;
    dungeon->floor_level = checkpoint;
    dungeon->room_type   = ROOM_NORMAL;
    Dungeon_Generate(dungeon); // Génère la carte
}

void Dungeon_Generate(DungeonContext* dungeon)
{
    // 1. Initialisation des murs et... du brouillard de guerre !
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
        {
            dungeon->map[y][x]      = '#';
            dungeon->explored[y][x] = false;
        }
    }

    // 2. Positionnement du joueur
    int startX         = MAP_WIDTH / 2;
    int startY         = MAP_HEIGHT / 2;
    dungeon->playerX   = startX;
    dungeon->playerY   = startY;
    dungeon->playerDir = DIR_NORTH;

    // 3. Révèle la zone de départ
    Dungeon_UpdateFog(dungeon, 0);

    // Salles Spéciales (Boss ou Événement)
    if (dungeon->room_type == ROOM_BOSS || dungeon->room_type == ROOM_EVENT)
    {
        // Crée une arène de 5x5
        for (int y = startY - 3; y <= startY + 1; y++)
        {
            for (int x = startX - 2; x <= startX + 2; x++)
                dungeon->map[y][x] = '.';
        }
        dungeon->playerY = startY + 1;

        if (dungeon->room_type == ROOM_BOSS)
        {
            dungeon->map[startY - 2][startX] = 'B';
        }
        else
        {
            // Pour les événements, on place l'Entité au centre, et l'escalier derrière !
            // Ainsi le joueur peut l'esquiver par les côtés s'il ne veut pas acheter.
            dungeon->map[startY - 2][startX] = 'E';
            dungeon->map[startY - 3][startX] = '>';
        }
        return;
    }

    // Sinon, Génération Procédurale (Drunkard's Walk)
    int x              = startX;
    int y              = startY;
    dungeon->map[y][x] = '.';

    int maxTiles     = (MAP_WIDTH * MAP_HEIGHT) / 4;
    int tilesCreated = 1;

    while (tilesCreated < maxTiles)
    {
        int dir = GetRandomValue(0, 3);
        if (dir == 0 && y > 2)
            y--;
        else if (dir == 1 && y < MAP_HEIGHT - 3)
            y++;
        else if (dir == 2 && x > 2)
            x--;
        else if (dir == 3 && x < MAP_WIDTH - 3)
            x++;

        if (dungeon->map[y][x] == '#')
        {
            dungeon->map[y][x] = '.';
            tilesCreated++;
        }
    }

    dungeon->map[y][x] = '>';
}

void Dungeon_Init(DungeonContext* dungeon)
{
    dungeon->floor_level   = 1;
    dungeon->highest_floor = 1;
    dungeon->room_type     = ROOM_NORMAL;
}

void Dungeon_Update(GameContext* game, DungeonContext* dungeon, int key)
{
    int  nextX    = dungeon->playerX;
    int  nextY    = dungeon->playerY;
    bool hasMoved = false; // <-- LA CORRECTION EST ICI

    if (key == KEY_UP)
    {
        if (dungeon->playerDir == DIR_NORTH)
            nextY--;
        else if (dungeon->playerDir == DIR_SOUTH)
            nextY++;
        else if (dungeon->playerDir == DIR_EAST)
            nextX++;
        else if (dungeon->playerDir == DIR_WEST)
            nextX--;
        hasMoved = true; // Le joueur a tenté d'avancer
    }
    else if (key == KEY_LEFT)
    {
        dungeon->playerDir = (dungeon->playerDir + 3) % 4;
        Dungeon_UpdateFog(dungeon, game->combat.player.fog_bonus);
        return;
    }
    else if (key == KEY_RIGHT)
    {
        dungeon->playerDir = (dungeon->playerDir + 1) % 4;
        Dungeon_UpdateFog(dungeon, game->combat.player.fog_bonus);
        return;
    }
    else if (key == KEY_Q || key == KEY_A)
    {
        game->currentState = STATE_CAMP;
        Dungeon_UpdateFog(dungeon, game->combat.player.fog_bonus);
        // On sauvegarde le nombre d'objets, ce qui les valide définitivement !
        game->combat.player.inventory_safe_count = game->combat.player.inventory_count;
        return;
    }

    // On ne vérifie la case QUE si le joueur s'est physiquement déplacé
    if (hasMoved && nextX >= 0 && nextX < MAP_WIDTH && nextY >= 0 && nextY < MAP_HEIGHT)
    {
        char nextTile = dungeon->map[nextY][nextX];
        if (nextTile == '.' || nextTile == '>' || nextTile == 'B' || nextTile == 'E')
        {
            dungeon->playerX = nextX;
            dungeon->playerY = nextY;

            Dungeon_UpdateFog(dungeon, game->combat.player.fog_bonus);

            // 1. Prise d'un Escalier
            if (nextTile == '>')
            {
                if (dungeon->room_type == ROOM_NORMAL)
                {
                    if (dungeon->floor_level > dungeon->highest_floor)
                    {
                        dungeon->highest_floor = dungeon->floor_level;
                    }

                    // Si le prochain étage est un multiple de 5 (Ex: on est au 4, on passe au 5)
                    if ((dungeon->floor_level + 1) % 5 == 0)
                    {
                        dungeon->floor_level++;
                        dungeon->room_type = ROOM_BOSS;
                    }
                    else
                    {
                        // Sinon, c'est forcément une salle d'événement (hors coffre)
                        dungeon->room_type = ROOM_EVENT;
                        if (g_eventCount > 0)
                        {
                            do
                            {
                                dungeon->current_event = g_eventDB[GetRandomValue(0, g_eventCount - 1)];
                            } while (strcmp(dungeon->current_event.type, "CHEST") == 0);
                        }
                    }
                }
                else if (dungeon->room_type == ROOM_BOSS)
                {
                    // Après le boss, c'est FORCEMENT le coffre !
                    dungeon->room_type = ROOM_EVENT;
                    for (int i = 0; i < g_eventCount; i++)
                    {
                        if (strcmp(g_eventDB[i].type, "CHEST") == 0)
                        {
                            dungeon->current_event = g_eventDB[i];
                            break;
                        }
                    }
                }
                else if (dungeon->room_type == ROOM_EVENT)
                {
                    // En sortant de la salle d'événement, on valide l'étage normal
                    dungeon->floor_level++;
                    dungeon->room_type = ROOM_NORMAL;
                }
                Dungeon_Generate(dungeon);
                Combat_AddLog(&game->combat, T("LOG_DESCEND"));
            }

            // 2. Interaction avec un Événement 'E'
            else if (nextTile == 'E')
            {
                bool eventSuccess = false;

                if (strcmp(dungeon->current_event.type, "HEAL") == 0)
                {
                    // Soin des PV
                    game->combat.player.hp += dungeon->current_event.amount;
                    if (game->combat.player.hp > game->combat.player.max_hp) {
                        game->combat.player.hp = game->combat.player.max_hp;
                    }

                    // Régénération de la moitié du Mana (50% du max)
                    game->combat.player.mana += (game->combat.player.max_mana / 2);
                    if (game->combat.player.mana > game->combat.player.max_mana) {
                        game->combat.player.mana = game->combat.player.max_mana;
                    }

                    eventSuccess = true;
                }
                else if (strcmp(dungeon->current_event.type, "MERCHANT") == 0)
                {
                    if (game->clicker.inventory.or >= dungeon->current_event.amount * (dungeon->floor_level / 2))
                    {
                        game->clicker.inventory.or -= dungeon->current_event.amount;

                        // --- NOUVELLE LOGIQUE MARCHAND (Potion aléatoire & scalée) ---
                        if (g_potionCount > 0)
                        {
                            // 1. Choisir une potion au hasard parmi celles existantes
                            int rand_idx = GetRandomValue(0, g_potionCount - 1);

                            // 2. Débloquer la potion (au cas où) et ajouter 1 à la quantité
                            game->combat.player.potion_unlocked[rand_idx] = true;
                            game->combat.player.potion_qty[rand_idx]++;

                            // 3. Calculer un niveau aléatoire basé sur l'étage (max niveau 10)
                            // Ex : Etage 25 -> Base 2. Peut donner une potion niveau 2, 3 ou 4.
                            int base_lvl     = dungeon->floor_level / 10;
                            int max_possible = base_lvl + 2;
                            if (max_possible > 10)
                                max_possible = 10;

                            int random_lvl = GetRandomValue(base_lvl, max_possible);

                            // On met à jour le niveau de la potion SEULEMENT si la nouvelle est meilleure
                            // (On ne veut pas qu'une potion redescende de niveau)
                            if (game->combat.player.potion_level[rand_idx] < random_lvl)
                            {
                                game->combat.player.potion_level[rand_idx] = random_lvl;
                            }

                            // 4. Afficher un joli message dans le log avec le nom et le niveau !
                            char logMsg[128];
                            sprintf(logMsg, "Achat : %s (Niv %d)", g_isEnglish ? g_potionDB[rand_idx].name_en : g_potionDB[rand_idx].name_fr, random_lvl);
                            Combat_AddLog(&game->combat, logMsg);
                        }

                        eventSuccess = true;
                    }
                    else
                    {
                        Combat_AddLog(&game->combat, g_isEnglish ? "Not enough gold!" : "Pas assez d'or !");
                    }
                }
                else if (strcmp(dungeon->current_event.type, "CHEST") == 0)
                {
                    if (g_itemCount > 0) {
                        int rand_item = GetRandomValue(0, g_itemCount - 1);
                        int base_lvl = dungeon->floor_level / 5;
                        int rand_lvl = base_lvl + GetRandomValue(0, 2);
                        
                        ItemEffect fx = ITEM_EFFECT_NONE;
                        if (GetRandomValue(1, 100) <= 30) {
                            fx = (ItemEffect)GetRandomValue(1, 4);
                        }

                        // --- NOUVEAU : LE TIRAGE DE LA RARETÉ ---
                        ItemRarity rarity = RARITY_COMMON;
                        int luck_bonus = game->combat.player.passive_loot_level * 2;
                        int roll = GetRandomValue(1, 100);
                        if (roll <= 5 + luck_bonus) rarity = RARITY_LEGENDARY;
                        else if (roll <= 20 + (luck_bonus * 2)) rarity = RARITY_EPIC;
                        else if (roll <= 50 + (luck_bonus * 3)) rarity = RARITY_RARE;
                        // Reste (50%) = Commun
                        
                        // Ajout avec la rareté !
                        Inventory_AddLoot(&game->combat, rand_item, rand_lvl, fx, rarity);
                        Combat_AddLog(&game->combat, "*** COFFRE OUVERT ! ***");
                    }
                    eventSuccess = true;
                }

                if (eventSuccess)
                {
                    Combat_AddLog(&game->combat, g_isEnglish ? dungeon->current_event.flavor_en : dungeon->current_event.flavor_fr);
                    dungeon->map[nextY][nextX] = '.'; // L'entité disparaît après utilisation
                }
                else
                {
                    // Si on a raté (pas d'or), on recule le joueur d'une case pour qu'il puisse réessayer ou partir
                    dungeon->playerX = dungeon->playerX - (nextX - dungeon->playerX);
                    dungeon->playerY = dungeon->playerY - (nextY - dungeon->playerY);
                }
            }

            // 3. Boss
            else if (nextTile == 'B')
            {
                Combat_StartEncounter(&game->combat, dungeon->floor_level, true);
                dungeon->map[nextY][nextX]     = '.';
                dungeon->map[nextY - 1][nextX] = '>';
            }
            // 4. Case vide (Monstres)
            else if (nextTile == '.')
            {
                if (GetRandomValue(1, 100) > 85 && dungeon->room_type == ROOM_NORMAL)
                {
                    Combat_StartEncounter(&game->combat, dungeon->floor_level, false);
                    return;
                }
            }
        }
    }
}

char GetTileAhead(DungeonContext* dungeon, int distance)
{
    int checkX = dungeon->playerX;
    int checkY = dungeon->playerY;

    if (dungeon->playerDir == DIR_NORTH)
        checkY -= distance;
    else if (dungeon->playerDir == DIR_SOUTH)
        checkY += distance;
    else if (dungeon->playerDir == DIR_EAST)
        checkX += distance;
    else if (dungeon->playerDir == DIR_WEST)
        checkX -= distance;

    if (checkX >= 0 && checkX < MAP_WIDTH && checkY >= 0 && checkY < MAP_HEIGHT)
        return dungeon->map[checkY][checkX];
    return '#';
}

void DrawTextCentered(Font font, const char* text, int centerX, int y, int fontSize, int spacing, Color color)
{
    Vector2 textSize = MeasureTextEx(font, text, (float)fontSize, (float)spacing);
    Vector2 position = {centerX - (textSize.x / 2.0f), (float)y};
    DrawTextEx(font, text, position, (float)fontSize, (float)spacing, color);
}

void Dungeon_Render(DungeonContext* dungeon, Font uiFont, Font dungeonFont, int screenWidth, int screenHeight)
{
    char dist1 = GetTileAhead(dungeon, 1);
    char dist2 = GetTileAhead(dungeon, 2);
    char dist3 = GetTileAhead(dungeon, 3);

    int viewStartX = (int)(screenWidth * 0.25f);
    int viewWidth  = (int)(screenWidth * 0.55f);
    int centerX    = viewStartX + (viewWidth / 2);

    char title[64];
    if (dungeon->room_type == ROOM_BOSS)
        sprintf(title, T("DUNGEON_TITLE_BOSS"), dungeon->floor_level);
    else if (dungeon->room_type == ROOM_EVENT)
        sprintf(title, "=== %s ===", g_isEnglish ? dungeon->current_event.name_en : dungeon->current_event.name_fr);
    else
        sprintf(title, T("DUNGEON_TITLE_DEEP"), dungeon->floor_level);

    // Titre remonté légèrement pour laisser de la place
    DrawTextCentered(uiFont, title, centerX, 80, 50, 1, RED); 

    // --- NOUVEAUX PARAMETRES DE TAILLE ---
    int startY   = (int)(screenHeight * 0.18f); // On commence plus haut sur l'écran
    int fontSize = 48; // Taille de police optimale
    int spacing  = 1;
    int lines    = 10; // 10 LIGNES AU LIEU DE 6 !

    // --- 1. LES MATRICES DU DONJON (10 lignes de profondeur) ---
    const char* view_dist1[] = {
        "  █████████████████████████████████████████████  ",
        "  █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█  ",
        "  █▓   ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓   ▓█  ",
        "  █▓   ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓   ▓█  ",
        "  █▓   ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓   ▓█  ",
        "  █▓   ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓   ▓█  ",
        "  █▓   ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓   ▓█  ",
        "  █▓   ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓   ▓█  ",
        "  █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█  ",
        "  █████████████████████████████████████████████  "
    };

    const char* view_dist2[] = {
        "    ▓\\                              /▓    ",
        "    ▓▓\\██████████████████████████/▓▓    ",
        "    ▓▓ |▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓| ▓▓    ",
        "    ▓▓ |▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓| ▓▓    ",
        "    ▓▓ |▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓| ▓▓    ",
        "    ▓▓ |▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓| ▓▓    ",
        "    ▓▓ |▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓| ▓▓    ",
        "    ▓▓ |▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓| ▓▓    ",
        "    ▓▓/██████████████████████████\\▓▓    ",
        "    ▓/                              \\▓    "
    };

    const char* view_dist3[] = {
        "      ░\\                          /░      ",
        "      ░▓\\██████████████████████/▓░      ",
        "      ░ |▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓| ░      ",
        "      ░ |▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓| ░      ",
        "      ░ |▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓| ░      ",
        "      ░ |▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓| ░      ",
        "      ░ |▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓| ░      ",
        "      ░ |▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓| ░      ",
        "      ░▓/██████████████████████\\▓░      ",
        "      ░/                          \\░      "
    };

    const char* view_empty[] = {
        "      █░                        ░█      ",
        "      ░█                        █░      ",
        "      ░█                        █░      ",
        "      ░█                        █░      ",
        "      ░█                        █░      ",
        "      ░█                        █░      ",
        "      ░█                        █░      ",
        "      ░█                        █░      ",
        "      ░█                        █░      ",
        "      █░                        ░█      "
    };

    // --- 2. LOGIQUE DE DESSIN OPTIMISÉE ---
    const char** active_view = NULL;
    Color view_color = WHITE;

    if (dist1 == '#')      { active_view = view_dist1; view_color = WHITE; }
    else if (dist2 == '#') { active_view = view_dist2; view_color = GRAY; }
    else if (dist3 == '#') { active_view = view_dist3; view_color = DARKGRAY; }
    else                   { active_view = view_empty; view_color = DARKGRAY; }

    // Une seule boucle pour tout dessiner, peu importe la distance !
    for(int i = 0; i < lines; i++) {
        DrawTextCentered(dungeonFont, active_view[i], centerX, startY + (i * fontSize), fontSize, spacing, view_color);
    }

    // --- 3. DESSIN DES OBJETS EN SURIMPRESSION ---
    int centerHeightY = startY + (fontSize * 4); // On place les objets au milieu du couloir

    if (dist1 == '>')
        DrawTextCentered(uiFont, T("DUNGEON_STAIRS_CLOSE"), centerX, centerHeightY, 30, 1, YELLOW);
    else if (dist2 == '>')
        DrawTextCentered(uiFont, T("DUNGEON_STAIRS_FAR"), centerX, centerHeightY, 20, 1, GRAY);

    if (dist1 == 'B')
        DrawTextCentered(uiFont, T("DUNGEON_BOSS_CLOSE"), centerX, centerHeightY, 50, 1, RED);
    else if (dist2 == 'B')
        DrawTextCentered(uiFont, T("DUNGEON_BOSS_FAR"), centerX, centerHeightY, 30, 1, DARKGRAY);

    // --- 4. DESSIN DU ASCII ART DE L'ÉVÉNEMENT ---
    if (dist1 == 'E' || dist2 == 'E')
    {
        Color eColor = (strcmp(dungeon->current_event.type, "MERCHANT") == 0) ? GOLD : (strcmp(dungeon->current_event.type, "HEAL") == 0) ? GREEN : SKYBLUE;

        int line_height = 20;
        int evStartY    = startY + (fontSize * 3); // Centré verticalement dans le grand couloir

        for (int i = 0; i < dungeon->current_event.ascii_line_count; i++)
        {
            DrawTextCentered(dungeonFont, dungeon->current_event.ascii[i], centerX, evStartY + (i * line_height), 20, 1, eColor);
        }

        // Si on est à côté, on affiche le prix !
        if (dist1 == 'E')
        {
            char prompt[100];
            if (strcmp(dungeon->current_event.type, "MERCHANT") == 0)
                sprintf(prompt, g_isEnglish ? "BUMP to Buy (-%d Gold)" : "BUMPER pour Acheter (-%d Or)", dungeon->current_event.amount);
            else
                sprintf(prompt, g_isEnglish ? "BUMP to Interact" : "BUMPER pour Interagir");
                
            DrawTextCentered(uiFont, prompt, centerX, evStartY + (dungeon->current_event.ascii_line_count * line_height) + 20, 24, 1, YELLOW);
        }
    }

    // --- 5. CONTRÔLES EN BAS ---
    DrawTextCentered(uiFont, T("DUNGEON_CONTROLS_MOVE"), centerX, screenHeight - 120, 24, 1, LIGHTGRAY);
    DrawTextCentered(uiFont, T("DUNGEON_CONTROLS_FLEE"), centerX, screenHeight - 80, 24, 1, RED);
}

void Dungeon_UpdateFog(DungeonContext* dungeon, int fog_bonus)
{
    // Le rayon de base est 1 (donc un carré de 3x3).
    // Si on a une torche (+2), le rayon passe à 3 (carré de 7x7) !
    int radius = 1 + fog_bonus;

    for (int y = dungeon->playerY - radius; y <= dungeon->playerY + radius; y++)
    {
        for (int x = dungeon->playerX - radius; x <= dungeon->playerX + radius; x++)
        {
            if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT)
            {
                dungeon->explored[y][x] = true;
            }
        }
    }
}

