#include "game.h"
#include "dungeon.h"
#include <stdlib.h>
#include <stdio.h>
#include "save.h"
#include "cJSON.h"
#include "lang.h"
#include "combat.h"
#include "ui.h"
#include "camp_menus.h"
#include "audio_manager.h"

/**
 * @brief Contexte du donjon (interne au module).
 */
static DungeonContext myDungeon;
/**
 * @brief Base de données des potions (externe).
 */
extern PotionTemplate g_potionDB[MAX_POTIONS_DB];
/**
 * @brief Base de données des sorts (externe).
 */
extern SpellTemplate g_spellDB[MAX_SPELLS_DB];

void Game_Init(GameContext* game)
{
    game->currentState = STATE_MENU;
    game->isRunning    = true;

    // 1. Chargement de la police pour l'Interface (l'ancienne)
    game->uiFont = LoadFontEx("assets/fonts/ui.ttf", 50, 0, 250);

    // 2. Chargement de la police pour le Donjon (la nouvelle avec les blocs)
    int  codepointCount = 95 + 32;
    int* codepoints     = (int*)malloc(codepointCount * sizeof(int));
    int  c              = 0;

    // 1. ASCII standard (Espace à ~)
    for (int i = 32; i <= 126; i++)
        codepoints[c++] = i;

    // 2. Block Elements (0x2580 à 0x259F)
    // Contient : ▀,  , ▂, ▃, ▄, ▅, ▆, ▇, █, ▉, ▊, ▋, ▌, ▍, ▎, ▏, ▐, ░, ▒, ▓, etc.
    for (int i = 0x2580; i <= 0x259F; i++)
        codepoints[c++] = i;

    // On charge à 50px pour avoir une bonne résolution sur les blocs
    game->dungeonFont = LoadFontEx("assets/fonts/dungeon.ttf", 50, codepoints, c);

    free(codepoints);

    Clicker_Init(&game->clicker);
    Combat_Init(&game->combat);
    Dungeon_Init(&myDungeon);
    LoadGame(game, &myDungeon);
}

void Game_Close(GameContext* game)
{
    // On libère bien les deux polices !
    UnloadFont(game->uiFont);
    UnloadFont(game->dungeonFont);
}

void Game_Update(GameContext* game)
{
    // =========================================================
    // --- GESTION DE L'AUDIO (Musiques) ---
    // =========================================================
    Audio_Update();

    if (game->currentState == STATE_DUNGEON) 
    {
        Audio_StopBGM(MUS_CAMP);

        // NOUVEAU : On vérifie si c'est un combat de boss !
        if (game->combat.is_active && game->combat.current_enemy.is_boss) {
            Audio_StopBGM(MUS_DUNGEON);
            Audio_PlayBGM(MUS_BOSS);
        } else {
            Audio_StopBGM(MUS_BOSS); // On coupe la musique du boss si le combat est fini
            Audio_PlayBGM(MUS_DUNGEON);
        }
    } 
    else if (game->currentState != STATE_MENU && game->currentState != STATE_GAMEOVER) 
    {
        // Au campement
        Audio_StopBGM(MUS_DUNGEON);
        Audio_StopBGM(MUS_BOSS); // Sécurité
        Audio_PlayBGM(MUS_CAMP);
    } 
    else 
    {
        // Menu principal ou Ecran de mort
        Audio_StopBGM(MUS_CAMP);
        Audio_StopBGM(MUS_DUNGEON);
        Audio_StopBGM(MUS_BOSS);
    }

    int key = GetKeyPressed();

    // =========================================================
    // --- GESTION GLOBALE DU FEU DE CAMP ---
    // =========================================================
    if (g_camp_fire_lit) 
    {
        Audio_PlayBGM(MUS_FIRE);
        g_camp_fire_timer += GetFrameTime();
        if (g_camp_fire_lit >= 1.0f) 
        {
            if (game->clicker.inventory.bois >= 5) {
                game->clicker.inventory.bois -= 5;
            } else {
                // LE FEU S'ÉTEINT PENDANT QU'ON EST AILLEURS !
                g_camp_fire_lit = false;  // Plus de bois, le feu meurt !
                game->combat.player.is_freezing = true;
                Combat_RecalculateStats(&game->combat);
            }
            g_camp_fire_timer -= 1.0f;
        }
    }
    else
    {
        Audio_StopBGM(MUS_FIRE);
    }

    Clicker_ProcessAuto(&game->clicker, GetFrameTime(), g_camp_fire_lit);


    int w          = GetScreenWidth();
    int h          = GetScreenHeight();
    int viewStartX = w * 0.25f;
    int viewWidth  = w * 0.55f;

    bool pressedQuit = (key == KEY_Q || key == KEY_A);

    if (game->currentState == STATE_DUNGEON && game->combat.player.hp <= 0)
    {
        game->currentState     = STATE_GAMEOVER;
        game->combat.is_active = false;
        game->combat.player.hp = 0; // Pour l'affichage propre
    }

    switch (game->currentState)
    {
        case STATE_MENU:
            if (key == KEY_ONE || key == KEY_KP_1)
                game->currentState = STATE_CAMP;
            else if (key == KEY_L)
            {
                g_isEnglish = !g_isEnglish;
                SaveGame(game, &myDungeon);
            }
            else if (pressedQuit)
            {
                SaveGame(game, &myDungeon);
                game->isRunning = false;
            }
            break;

        case STATE_CAMP:
            // Navigation depuis le camp
            if (key == KEY_ONE || key == KEY_KP_1)
            {
                game->currentState = STATE_MINE;
                SaveGame(game, &myDungeon);
            }
            else if (key == KEY_TWO || key == KEY_KP_2)
            {
                game->currentState = STATE_FOREST;
                SaveGame(game, &myDungeon);
            }
            else if (key == KEY_THREE || key == KEY_KP_3)
            {
                game->currentState = STATE_FORGE;
                SaveGame(game, &myDungeon);
            }
            else if (key == KEY_FOUR || key == KEY_KP_4)
            {
                game->currentState = STATE_ALCHEMIST;
                SaveGame(game, &myDungeon);
            }
            else if (key == KEY_FIVE || key == KEY_KP_5)
            {
                game->currentState = STATE_ARCHIFORGE;
                SaveGame(game, &myDungeon);
            }
            else if (key == KEY_SIX || key == KEY_KP_6)
            {
                game->currentState = STATE_INVENTORY;
                SaveGame(game, &myDungeon);
            }
            else if (key == KEY_SEVEN || key == KEY_KP_7)
            {
                game->currentState = STATE_DUNGEON;
                SaveGame(game, &myDungeon);
                Dungeon_Enter(&myDungeon);
                game->combat.player.inventory_safe_count = game->combat.player.inventory_count;
            }
            else if ((key == KEY_EIGHT) || (key == KEY_KP_8))
            {
                SaveGame(game, &myDungeon);
                game->currentState = STATE_ALTAR;
            }
            else if (pressedQuit)
            {
                SaveGame(game, &myDungeon);
                game->currentState = STATE_MENU;
            }
            break;

        case STATE_MINE:
            // On gère les clics de la mine
            Clicker_UpdateMine(&game->clicker, viewStartX, viewWidth, h, game->uiFont);
            // Retour au camp avec Q (ou A)
            if (pressedQuit)
            {
                SaveGame(game, &myDungeon);
                game->currentState = STATE_CAMP;
            }
            break;

        case STATE_FOREST:
            // On gère les clics de la forêt
            Clicker_UpdateForest(&game->clicker, viewStartX, viewWidth, h, game->uiFont);
            // Retour au camp avec Q (ou A)
            if (pressedQuit)
            {
                SaveGame(game, &myDungeon);
                game->currentState = STATE_CAMP;
            }
            break;

        case STATE_DUNGEON:
            if (game->combat.is_active)
            {
                Combat_Update(&game->combat, GetFrameTime(), viewStartX + (viewWidth / 2), h / 2);
            }
            else
            {
                // On délègue TOUT le déplacement ET les combats au donjon.
                Dungeon_Update(game, &myDungeon, key);
                for (int i = 0; i < 3; i++)
                {
                    if (IsKeyPressed(KEY_ONE + i) || IsKeyPressed(KEY_KP_1 + i))
                    {
                        Combat_TryUsePotion(&game->combat, i);
                    }
                }
            }
            break;
        case STATE_FORGE:
        case STATE_ALCHEMIST:
        case STATE_ARCHIFORGE:
        case STATE_INVENTORY:
        case STATE_ALTAR:
            // Pour ces 3 menus, on quitte avec Q (ou A)
            if (pressedQuit)
            {
                game->currentState = STATE_CAMP;
                SaveGame(game, &myDungeon);
            }
            break;
        case STATE_GAMEOVER:
            if (key == KEY_SPACE || key == KEY_ENTER)
            {
                Combat_ResetRun(&game->combat);      // Perd l'XP et les niveaux du donjon
                game->combat.player.hp = 1;          // Renaissance avec 1 PV
                game->currentState     = STATE_CAMP; // Retour à l'abri
                SaveGame(game, &myDungeon);
            }
            break;
    }
}

void Game_Render(GameContext* game)
{
    BeginDrawing();
    ClearBackground(BLACK);

    int w = GetScreenWidth();
    int h = GetScreenHeight();

    if (game->currentState == STATE_MENU)
    {
        DrawTextEx(game->uiFont, T("MENU_TITLE"), (Vector2){w / 2 - 200, h / 2 - 100}, 40, 1, DARKGRAY);
        DrawTextEx(game->uiFont, T("MENU_PROMPT"), (Vector2){w / 2 - 150, h / 2}, 24, 1, LIGHTGRAY);
    }
    else
    {
        int startX = (w * 0.25f) + 50;
        if (game->currentState == STATE_CAMP)
        {
            Game_RenderCamp(game, w, h);
        }
        else if (game->currentState == STATE_FORGE)
        {
            Game_RenderForge(game, w, h);
            DrawTextEx(game->uiFont, T("BTN_BACK_CAMP"), (Vector2){startX, h - 80}, 20, 1, GRAY);
        }
        else if (game->currentState == STATE_INVENTORY)
        {
            Game_RenderInventory(game, w, h);
            DrawTextEx(game->uiFont, T("BTN_BACK_CAMP"), (Vector2){startX, h - 80}, 20, 1, GRAY);
        }
        else if (game->currentState == STATE_ALCHEMIST)
        {
            Game_RenderAlchemist(game, w, h);
            DrawTextEx(game->uiFont, T("BTN_BACK_CAMP"), (Vector2){startX, h - 80}, 20, 1, GRAY);
        }
        else if (game->currentState == STATE_ARCHIFORGE)
        {
            Game_RenderArchiforge(game, w, h);

            DrawTextEx(game->uiFont, T("BTN_BACK_CAMP"), (Vector2){startX, h - 80}, 20, 1, GRAY);
        }
        else if (game->currentState == STATE_MINE)
        {
            Clicker_RenderMine(&game->clicker, w * 0.25f, w * 0.55f, h, game->uiFont);

            int cx = (w * 0.25f) + ((w * 0.55f) / 2);
            DrawTextEx(game->uiFont, T("BTN_BACK_CAMP"), (Vector2){cx - 150, h - 50}, 20, 1, GRAY);
        }
        else if (game->currentState == STATE_FOREST)
        {
            Clicker_RenderForest(&game->clicker, w * 0.25f, w * 0.55f, h, game->uiFont);

            int cx = (w * 0.25f) + ((w * 0.55f) / 2);
            DrawTextEx(game->uiFont, T("BTN_BACK_CAMP"), (Vector2){cx - 150, h - 50}, 20, 1, GRAY);
        }
        else if (game->currentState == STATE_ALTAR)
        {
            Game_RenderAltar(game, w, h);
            DrawTextEx(game->uiFont, T("BTN_BACK_CAMP"), (Vector2){startX, h - 80}, 20, 1, GRAY);
        }
        else if (game->currentState == STATE_DUNGEON)
        {
            if (game->combat.is_active)
            {
                int cx = (w * 0.25f) + ((w * 0.55f) / 2); // Centre de la zone de jeu
                int cy = h / 2;
                Combat_RenderCenter(&game->combat, game->uiFont, cx, cy);
            }
            else
            {
                Dungeon_Render(&myDungeon, game->uiFont, game->dungeonFont, w, h);
            }
        }
        else if (game->currentState == STATE_GAMEOVER)
        {
            int cx = (w * 0.20f) + ((w * 0.55f) / 2);
            int cy = h / 2;

            DrawTextEx(game->uiFont, T("LOG_DEAD"), (Vector2){cx - 150, cy - 150}, 50, 1, RED);

            // Un crâne stylisé
            DrawTextEx(game->uiFont, "      _.--\"\"\"--._      ", (Vector2){cx - 150, cy - 80}, 24, 1, GRAY);
            DrawTextEx(game->uiFont, "     /  _   _  \\     ", (Vector2){cx - 150, cy - 50}, 24, 1, GRAY);
            DrawTextEx(game->uiFont, "    |  (o) (o)  |    ", (Vector2){cx - 150, cy - 20}, 24, 1, GRAY);
            DrawTextEx(game->uiFont, "    |    / \\    |    ", (Vector2){cx - 150, cy + 10}, 24, 1, GRAY);
            DrawTextEx(game->uiFont, "     \\  '---'  /     ", (Vector2){cx - 150, cy + 40}, 24, 1, GRAY);
            DrawTextEx(game->uiFont, "      '-------'      ", (Vector2){cx - 150, cy + 70}, 24, 1, GRAY);

            DrawTextEx(game->uiFont, T("GAMEOVER_STATS_LOST"), (Vector2){cx - 250, cy + 140}, 24, 1, GRAY);
            DrawTextEx(game->uiFont, T("GAMEOVER_RESPAWN"), (Vector2){cx - 230, cy + 200}, 24, 1, LIGHTGRAY);
        }

        // Dessiner l'IHM latérale par-dessus
        DrawGameUI(game, &myDungeon, w, h);
    }

    EndDrawing();
}

void Game_Run(GameContext* game)
{
    while (game->isRunning)
    {
        Game_Update(game);
        Game_Render(game);
    }
}
