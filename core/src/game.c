#include "game.h"
#include "../../dungeon/inc/dungeon.h"
#include <stdlib.h> 
#include <stdio.h>

static DungeonContext myDungeon;

bool DoShopButton(Font font, const char* text, int x, int y, int fontSize, bool canAfford) {
    Vector2 textSize = MeasureTextEx(font, text, fontSize, 1);
    Rectangle hitbox = { x, y, textSize.x, textSize.y };
    bool isHovered = CheckCollisionPointRec(GetMousePosition(), hitbox);
    
    Color drawColor = GRAY;
    if (canAfford) drawColor = isHovered ? WHITE : LIGHTGRAY;
    else drawColor = DARKGRAY; // Grisé si on ne peut pas acheter

    DrawTextEx(font, text, (Vector2){x, y}, fontSize, 1, drawColor);
    
    return isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && canAfford;
}

void Game_Init(GameContext* game) {
    game->currentState = STATE_MENU;
    game->isRunning = true;

    // 1. Chargement de la police pour l'Interface (l'ancienne)
    game->uiFont = LoadFontEx("assets/fonts/ui.ttf", 40, 0, 250);

    // 2. Chargement de la police pour le Donjon (la nouvelle avec les blocs)
    int codepointCount = 95 + 32; 
    int *codepoints = (int *)malloc(codepointCount * sizeof(int));
    int c = 0;

    // 1. ASCII standard (Espace à ~)
    for (int i = 32; i <= 126; i++) codepoints[c++] = i;

    // 2. Block Elements (0x2580 à 0x259F)
    // Contient : ▀,  , ▂, ▃, ▄, ▅, ▆, ▇, █, ▉, ▊, ▋, ▌, ▍, ▎, ▏, ▐, ░, ▒, ▓, etc.
    for (int i = 0x2580; i <= 0x259F; i++) codepoints[c++] = i;

    // On charge à 50px pour avoir une bonne résolution sur les blocs
    game->dungeonFont = LoadFontEx("assets/fonts/dungeon.ttf", 50, codepoints, c);
    
    free(codepoints);
    
    Clicker_Init(&game->clicker);
    Combat_Init(&game->combat); 
    Dungeon_Init(&myDungeon);
}

void Game_Close(GameContext* game) {
    // On libère bien les deux polices !
    UnloadFont(game->uiFont);
    UnloadFont(game->dungeonFont);
}


void Game_Update(GameContext* game) {
    int key = GetKeyPressed(); 
    
    // L'automatisation tourne en permanence, peu importe l'écran !
    Clicker_ProcessAuto(&game->clicker, GetFrameTime());

    int w = GetScreenWidth();
    int h = GetScreenHeight();
    int viewStartX = w * 0.20f;
    int viewWidth = w * 0.55f;

    bool pressedQuit = (key == KEY_Q || key == KEY_A);

    static float camp_heal_timer = 0.0f;
    if (game->currentState == STATE_CAMP) {
        camp_heal_timer += GetFrameTime();
        if (camp_heal_timer >= 0.2f) { // Toutes les 0.2s
            if (game->combat.player.hp < game->combat.player.max_hp) {
                game->combat.player.hp++;
            }
            camp_heal_timer = 0.0f;
        }
    }

    if (game->currentState == STATE_DUNGEON && game->combat.player.hp <= 0) {
        game->currentState = STATE_GAMEOVER;
        game->combat.is_active = false;
        game->combat.player.hp = 0; // Pour l'affichage propre
    }

    switch (game->currentState) {
        case STATE_MENU:
            if (key == KEY_ONE || key == KEY_KP_1) game->currentState = STATE_CAMP;
            else if (pressedQuit) game->isRunning = false;
            break;
            
        case STATE_CAMP:
            // Navigation depuis le camp
            if (key == KEY_ONE || key == KEY_KP_1) game->currentState = STATE_MINE;
            else if (key == KEY_TWO || key == KEY_KP_2) game->currentState = STATE_FOREST;
            else if (key == KEY_THREE || key == KEY_KP_3) game->currentState = STATE_FORGE;
            else if (key == KEY_FOUR || key == KEY_KP_4) game->currentState = STATE_ALCHEMIST;
            else if (key == KEY_FIVE || key == KEY_KP_5) game->currentState = STATE_ARCHIFORGE;
            else if (key == KEY_SIX || key == KEY_KP_6) game->currentState = STATE_DUNGEON;
            else if (pressedQuit) game->currentState = STATE_MENU;
            break;
            
        case STATE_MINE:
            // On gère les clics de la mine
            Clicker_UpdateMine(&game->clicker, viewStartX, viewWidth, h, game->uiFont);
            // Retour au camp avec Q (ou A)
            if (pressedQuit) game->currentState = STATE_CAMP;
            break;
            
        case STATE_FOREST:
            // On gère les clics de la forêt
            Clicker_UpdateForest(&game->clicker, viewStartX, viewWidth, h, game->uiFont);
            // Retour au camp avec Q (ou A)
            if (pressedQuit) game->currentState = STATE_CAMP;
            break;
            
        case STATE_DUNGEON:
            if (game->combat.is_active) {
                Combat_Update(&game->combat, GetFrameTime(), viewStartX + (viewWidth/2), h/2);
            } else {
                // On délègue TOUT le déplacement ET les combats au donjon.
                Dungeon_Update(game, &myDungeon, key); 
            }
            break;
        case STATE_FORGE:
        case STATE_ALCHEMIST:
        case STATE_ARCHIFORGE:
            // Pour ces 3 menus, on quitte avec Q (ou A)
            if (pressedQuit) game->currentState = STATE_CAMP;
            break;
        case STATE_GAMEOVER:
            if (key == KEY_SPACE || key == KEY_ENTER) {
                Combat_ResetRun(&game->combat); // Perd l'XP et les niveaux du donjon
                game->combat.player.hp = 1;     // Renaissance avec 1 PV
                game->currentState = STATE_CAMP; // Retour à l'abri
            }
            break;
    }
}

void DrawGameUI(GameContext* game, int w, int h) {
    float leftWidth = w * 0.25f;
    float rightWidth = w * 0.20f;
    float centerWidth = w - leftWidth - rightWidth;

    Color uiBorder = DARKGRAY;
    Color uiBg = (Color){ 10, 10, 10, 255 };

    // Les 3 zones
    DrawRectangle(0, 0, leftWidth, h, uiBg);
    DrawRectangle(w - rightWidth, 0, rightWidth, h, uiBg);

    // Lignes de séparation
    DrawRectangleLinesEx((Rectangle){0, 0, leftWidth, h}, 2, uiBorder);
    DrawRectangleLinesEx((Rectangle){w - rightWidth, 0, rightWidth, h}, 2, uiBorder);

    // --- EN HAUT DU CENTRE : STATS DU JOUEUR ---
    int topBarHeight = 80;
    DrawRectangle(leftWidth, 0, centerWidth, topBarHeight, uiBg);
    DrawRectangleLinesEx((Rectangle){leftWidth, 0, centerWidth, topBarHeight}, 2, uiBorder);
    
    char statHeader[100];
    sprintf(statHeader, "KAAN ASLANBAS - Niv %d  ( XP: %d/%d )", game->combat.player.level, game->combat.player.xp, game->combat.player.max_xp);
    DrawTextEx(game->uiFont, statHeader, (Vector2){leftWidth + 20, 10}, 24, 1, WHITE);
    
    char combatStats[100];
    sprintf(combatStats, "HP: %d/%d  |  Mana: %d/%d  |  ATK: %d", 
        game->combat.player.hp, game->combat.player.max_hp, 
        game->combat.player.mana, game->combat.player.max_mana,
        game->combat.player.atk);
    DrawTextEx(game->uiFont, combatStats, (Vector2){leftWidth + 20, 40}, 20, 1, (game->combat.player.hp < 20) ? RED : GREEN);

    // --- PANNEAU GAUCHE : INVENTAIRE ET SORTS ---
    DrawTextEx(game->uiFont, "[ INVENTAIRE / EQUIPEMENT ]", (Vector2){20, 20}, 24, 1, LIGHTGRAY);
    
    // Potions (Cliquables)
    char potText[50];
    sprintf(potText, "(%d) Potion de Soin [UTILISER]", game->combat.player.potions_hp);
    Rectangle potHitbox = { 20, 80, 300, 24 };
    bool hoverPot = CheckCollisionPointRec(GetMousePosition(), potHitbox);
    DrawTextEx(game->uiFont, potText, (Vector2){20, 80}, 20, 1, hoverPot ? WHITE : RED);
    
    if (hoverPot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && game->combat.player.potions_hp > 0) {
        game->combat.player.potions_hp--;
        game->combat.player.hp += 20;
        if (game->combat.player.hp > game->combat.player.max_hp) game->combat.player.hp = game->combat.player.max_hp;
        Combat_AddLog(&game->combat, "Vous buvez une Potion !");
    }

    DrawTextEx(game->uiFont, "[ SORTS ]", (Vector2){20, 150}, 24, 1, LIGHTGRAY);
    DrawTextEx(game->uiFont, "(>) Boule de Feu (10 Mana)", (Vector2){20, 190}, 20, 1, ORANGE);

    // Ressources du clicker (plus bas à gauche)
    DrawTextEx(game->uiFont, "[ RESSOURCES ]", (Vector2){20, 400}, 24, 1, LIGHTGRAY);
    DrawTextEx(game->uiFont, TextFormat("Fer: %d", game->clicker.inventory.fer), (Vector2){20, 440}, 20, 1, GRAY);
    DrawTextEx(game->uiFont, TextFormat("Or: %d", game->clicker.inventory.or), (Vector2){20, 470}, 20, 1, YELLOW);
    DrawTextEx(game->uiFont, TextFormat("Cristaux: %d", game->clicker.inventory.cristaux), (Vector2){20, 500}, 20, 1, PURPLE);
    DrawTextEx(game->uiFont, TextFormat("Herbes: %d", game->clicker.inventory.herbes), (Vector2){20, 530}, 20, 1, GREEN);
    DrawTextEx(game->uiFont, TextFormat("Bois: %d", game->clicker.inventory.bois), (Vector2){20, 560}, 20, 1, BROWN);
    DrawTextEx(game->uiFont, TextFormat("Viande: %d", game->clicker.inventory.viande), (Vector2){20, 590}, 20, 1, RED);
    
    // --- PANNEAU DROIT : JOURNAL DE BATAILLE ---
    DrawTextEx(game->uiFont, "[ JOURNAL DE BATAILLE ]", (Vector2){w - rightWidth + 20, 20}, 24, 1, LIGHTGRAY);
    for (int i = 0; i < 5; i++) {
        // Affiche les messages du plus récent au plus ancien
        DrawTextEx(game->uiFont, game->combat.battle_log[i], (Vector2){w - rightWidth + 20, 80 + (i * 30)}, 20, 1, (i == 0) ? WHITE : GRAY);
    }
}

void Game_Render(GameContext* game) {
    BeginDrawing();
    ClearBackground(BLACK);

    int w = GetScreenWidth();
    int h = GetScreenHeight();

    if (game->currentState == STATE_MENU) {
        DrawTextEx(game->uiFont, "=== ABYSSAL ASCENT ===", (Vector2){w/2 - 200, h/2 - 100}, 40, 1, DARKGRAY);
        DrawTextEx(game->uiFont, "[1] Jouer   [Q] Quitter", (Vector2){w/2 - 150, h/2}, 24, 1, LIGHTGRAY);
    } 
    else {
        if (game->currentState == STATE_CAMP) {
            int cx = (w * 0.2f) + ((w * 0.55f) / 2);
            int cy = h / 2;
            int baseFontSize = 40; // Taille de base pour le texte du titre
            float asciiFontSize = 20; // Taille plus petite pour l'ASCII pour plus de détail
            float spacing = 2; // Espacement léger entre les caractères

            // 2. Titre
            DrawTextEx(game->dungeonFont, "LE CAMPEMENT", (Vector2){cx - 100, cy - 200}, baseFontSize, 1, GREEN);

            // 3. Définition de l'ASCII Art "Blocs" (13 lignes)
            const char* fireAscii[] = {
                "        ▒       .      ",
                "    ▓██▄  ▒   ▓██▄     ",
                "    ▀███  ███████████   ",
                "    ▒    █████████████  ",
                "   ▄████  ████████████▀  ",
                "  ████████████▓▀███▀████     ▄▄ ",
                "  █████████████▀   █████▄   ▄███",
                "  ▀██████████▀     ▀███████████",
                "   ████████      ▄▄   ▀███████▀",
                "    ▀▀█████████████     ██████ ",
                "  ▄▄██████████████▀█▄█████████▄▄",
                "  █████████████████████████▀▀▀",
                "   ▀███████████▀  ▀█████████▀ "
            };

            int lineCount = 13;

            // 4. Boucle de rendu de l'ASCII avec dégradé
            for (int i = 0; i < lineCount; i++) {
                // Calcul du centrage pour chaque ligne
                Vector2 textSize = MeasureTextEx(game->dungeonFont, fireAscii[i], asciiFontSize, spacing);
                Vector2 pos = { cx - (textSize.x / 2), cy - 100 + (i * asciiFontSize) };
                
                // Dégradé de couleur pour l'effet de feu :
                // Jaune -> Orange -> Rouge -> Marron (pour les bûches/cendre)
                Color col = YELLOW; // Haut (flammes)
                if (i > 3) col = ORANGE; // Milieu
                if (i > 8) col = RED; // Bas du feu
                if (i > 10) col = (Color){101, 67, 33, 255}; // Bûches (Marron foncé)

                DrawTextEx(game->dungeonFont, fireAscii[i], pos, asciiFontSize, spacing, col);
            }
            DrawTextEx(game->uiFont, "[1] Mine", (Vector2){cx - 150, h - 300}, 24, 1, LIGHTGRAY);
            DrawTextEx(game->uiFont, "[2] Foret", (Vector2){cx - 150, h - 260}, 24, 1, GREEN);
            DrawTextEx(game->uiFont, "[3] La Forge (Equipement)", (Vector2){cx - 150, h - 220}, 24, 1, ORANGE);
            DrawTextEx(game->uiFont, "[4] Alchimiste (Potions)", (Vector2){cx - 150, h - 180}, 24, 1, PURPLE);
            DrawTextEx(game->uiFont, "[5] Archiforge (Sorts)", (Vector2){cx - 150, h - 140}, 24, 1, BLUE);
            DrawTextEx(game->uiFont, "[6] >>> DONJON <<<", (Vector2){cx - 150, h - 100}, 24, 1, RED);
            DrawTextEx(game->uiFont, "[Q] Menu Principal", (Vector2){cx - 150, h - 50}, 20, 1, DARKGRAY);

        }
        else if (game->currentState == STATE_FORGE) {
            int startX = (w * 0.25f) + 50;
            // On descend le titre à 120 (sous la barre du haut)
            DrawTextEx(game->uiFont, "=== LA FORGE (Ameliorations) ===", (Vector2){startX, 120}, 40, 1, ORANGE);
            
            int costFer = 50 + (game->combat.player.eq_epee * 25);
            int costBois = 20 + (game->combat.player.eq_epee * 10);
            // On descend les boutons à 200 et 260
            if (DoShopButton(game->uiFont, TextFormat("> Ameliorer EPEE (Niv %d) -> (-%d Fer, -%d Bois)", game->combat.player.eq_epee, costFer, costBois), startX, 200, 24, (game->clicker.inventory.fer >= costFer && game->clicker.inventory.bois >= costBois))) {
                game->clicker.inventory.fer -= costFer; game->clicker.inventory.bois -= costBois;
                game->combat.player.eq_epee++;
                Combat_RecalculateStats(&game->combat);
            }

            int costArmureFer = 40 + (game->combat.player.eq_armure * 30);
            int costArmureCuir = 20 + (game->combat.player.eq_armure * 20);
            if (DoShopButton(game->uiFont, TextFormat("> Ameliorer ARMURE (Niv %d) -> (-%d Fer, -%d Viande)", game->combat.player.eq_armure, costArmureFer, costArmureCuir), startX, 260, 24, (game->clicker.inventory.fer >= costArmureFer && game->clicker.inventory.viande >= costArmureCuir))) {
                game->clicker.inventory.fer -= costArmureFer; game->clicker.inventory.viande -= costArmureCuir;
                game->combat.player.eq_armure++;
                Combat_RecalculateStats(&game->combat);
            }
            DrawTextEx(game->uiFont, "[Q] Retour au Camp", (Vector2){startX, h - 80}, 20, 1, GRAY);
        }
        else if (game->currentState == STATE_ALCHEMIST) {
            int startX = (w * 0.25f) + 50;
            DrawTextEx(game->uiFont, "=== L'ALCHIMISTE ===", (Vector2){startX, 120}, 40, 1, PURPLE);
            
            if (DoShopButton(game->uiFont, "> Crafter Potion de Soin (-10 Herbes)", startX, 200, 24, (game->clicker.inventory.herbes >= 10))) {
                game->clicker.inventory.herbes -= 10;
                game->combat.player.potions_hp++;
            }
            
            if (DoShopButton(game->uiFont, "> Crafter Potion de Mana (-5 Herbes, -5 Cristaux)", startX, 260, 24, (game->clicker.inventory.herbes >= 5 && game->clicker.inventory.cristaux >= 5))) {
                game->clicker.inventory.herbes -= 5; game->clicker.inventory.cristaux -= 5;
                game->combat.player.potions_mana++;
            }
            DrawTextEx(game->uiFont, "[Q] Retour au Camp", (Vector2){startX, h - 80}, 20, 1, GRAY);
        }
        else if (game->currentState == STATE_ARCHIFORGE) {
            int startX = (w * 0.25f) + 50;
            DrawTextEx(game->uiFont, "=== L'ARCHIFORGE ===", (Vector2){startX, 120}, 40, 1, BLUE);
            
            if (!game->combat.player.spell_fireball) {
                if (DoShopButton(game->uiFont, "> Apprendre BOULE DE FEU (-50 Or, -50 Cristaux)", startX, 200, 24, (game->clicker.inventory.or >= 50 && game->clicker.inventory.cristaux >= 50))) {
                    game->clicker.inventory.or -= 50; game->clicker.inventory.cristaux -= 50;
                    game->combat.player.spell_fireball = true;
                }
            } else DrawTextEx(game->uiFont, "> BOULE DE FEU (Acquis)", (Vector2){startX, 200}, 24, 1, DARKGRAY);

            if (!game->combat.player.spell_heal) {
                if (DoShopButton(game->uiFont, "> Apprendre SOIN (-50 Herbes, -50 Cristaux)", startX, 260, 24, (game->clicker.inventory.herbes >= 50 && game->clicker.inventory.cristaux >= 50))) {
                    game->clicker.inventory.herbes -= 50; game->clicker.inventory.cristaux -= 50;
                    game->combat.player.spell_heal = true;
                }
            } else DrawTextEx(game->uiFont, "> SOIN (Acquis)", (Vector2){startX, 260}, 24, 1, DARKGRAY);
            
            DrawTextEx(game->uiFont, "[Q] Retour au Camp", (Vector2){startX, h - 80}, 20, 1, GRAY);
        }
        else if (game->currentState == STATE_MINE) {
            Clicker_RenderMine(&game->clicker, w * 0.20f, w * 0.55f, h, game->uiFont);
            
            int cx = (w * 0.20f) + ((w * 0.55f) / 2);
            DrawTextEx(game->uiFont, "[Q] Retour au Campement", (Vector2){cx - 150, h - 50}, 20, 1, GRAY);
        }
        else if (game->currentState == STATE_FOREST) {
            Clicker_RenderForest(&game->clicker, w * 0.20f, w * 0.55f, h, game->uiFont);
            
            int cx = (w * 0.20f) + ((w * 0.55f) / 2);
            DrawTextEx(game->uiFont, "[Q] Retour au Campement", (Vector2){cx - 150, h - 50}, 20, 1, GRAY);
        }
        else if (game->currentState == STATE_DUNGEON) {
            if (game->combat.is_active) {
                // Si en combat, on ne dessine que le monstre au centre
                int cx = (w * 0.25f) + ((w * 0.55f) / 2); // Centre de la zone de jeu
                int cy = h / 2;
                Combat_RenderCenter(&game->combat, game->uiFont, cx, cy);
            } else {
                Dungeon_Render(&myDungeon, game->uiFont, game->dungeonFont, w, h);
            }
        }
        else if (game->currentState == STATE_GAMEOVER) {
            int cx = (w * 0.20f) + ((w * 0.55f) / 2);
            int cy = h / 2;
            
            DrawTextEx(game->uiFont, "VOUS ETES MORT", (Vector2){cx - 150, cy - 150}, 50, 1, RED);
            
            // Un crâne stylisé
            DrawTextEx(game->uiFont, "      _.--\"\"\"--._      ", (Vector2){cx - 150, cy - 80}, 24, 1, GRAY);
            DrawTextEx(game->uiFont, "     /  _   _  \\     ", (Vector2){cx - 150, cy - 50}, 24, 1, GRAY);
            DrawTextEx(game->uiFont, "    |  (o) (o)  |    ", (Vector2){cx - 150, cy - 20}, 24, 1, GRAY);
            DrawTextEx(game->uiFont, "    |    / \\    |    ", (Vector2){cx - 150, cy + 10}, 24, 1, GRAY);
            DrawTextEx(game->uiFont, "     \\  '---'  /     ", (Vector2){cx - 150, cy + 40}, 24, 1, GRAY);
            DrawTextEx(game->uiFont, "      '-------'      ", (Vector2){cx - 150, cy + 70}, 24, 1, GRAY);

            DrawTextEx(game->uiFont, "Vos statistiques de donjon sont perdues.", (Vector2){cx - 250, cy + 140}, 24, 1, GRAY);
            DrawTextEx(game->uiFont, "[ESPACE] pour renaitre au Campement", (Vector2){cx - 230, cy + 200}, 24, 1, LIGHTGRAY);
        }

        // Dessiner l'IHM latérale par-dessus
        DrawGameUI(game, w, h);
    }

    EndDrawing();
}

void Game_Run(GameContext* game) {
    while (game->isRunning && !WindowShouldClose()) {
        Game_Update(game);
        Game_Render(game);
    }
}