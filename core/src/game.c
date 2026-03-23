#include "game.h"
#include "../../dungeon/inc/dungeon.h"
#include <stdlib.h>
#include <stdio.h>
#include "../../utils/inc/cJSON.h"
#include "../../utils/inc/lang.h"
#include "../../combat/inc/combat.h"

static DungeonContext myDungeon;
static int            selectedForgeIdx = -1;
static int            selectedSpellIdx = -1;
static int selectedPotionIdx = -1;

static void SaveGame(GameContext* game);
static void LoadGame(GameContext* game);
static bool DoShopButton(Font font, const char* text, int x, int y, int fontSize, bool canAfford);

extern PotionTemplate g_potionDB[MAX_POTIONS_DB];
extern SpellTemplate g_spellDB[MAX_SPELLS_DB];

#define LOAD_INT_ARRAY(jsonName, cArray, maxSize) \
        do { \
            cJSON* arr = cJSON_GetObjectItem(root, jsonName); \
            if (arr) { \
                int i = 0; cJSON* item; \
                cJSON_ArrayForEach(item, arr) { \
                    if (i < maxSize) { cArray[i] = item->valueint; i++; } \
                } \
            } \
        } while(0)


void Game_Init(GameContext* game)
{
    game->currentState = STATE_MENU;
    game->isRunning    = true;

    // 1. Chargement de la police pour l'Interface (l'ancienne)
    game->uiFont = LoadFontEx("assets/fonts/ui.ttf", 40, 0, 250);

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
    LoadGame(game);
    Inventory_Add(&game->combat, "rusty_sword");
    Inventory_Add(&game->combat, "torch");
}

void Game_Close(GameContext* game)
{
    // On libère bien les deux polices !
    UnloadFont(game->uiFont);
    UnloadFont(game->dungeonFont);
}

void Game_Update(GameContext* game)
{
    int key = GetKeyPressed();

    // L'automatisation tourne en permanence, peu importe l'écran !
    Clicker_ProcessAuto(&game->clicker, GetFrameTime());

    int w          = GetScreenWidth();
    int h          = GetScreenHeight();
    int viewStartX = w * 0.25f;
    int viewWidth  = w * 0.55f;

    bool pressedQuit = (key == KEY_Q || key == KEY_A);

    static float camp_heal_timer = 0.0f;
    if (game->currentState == STATE_CAMP)
    {
        camp_heal_timer += GetFrameTime();
        if (camp_heal_timer >= 0.2f)
        { // Toutes les 0.2s
            if (game->combat.player.hp < game->combat.player.max_hp)
            {
                game->combat.player.hp++;
            }
            camp_heal_timer = 0.0f;
        }
    }

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
                SaveGame(game);
            }
            else if (pressedQuit)
            {
                SaveGame(game);
                game->isRunning = false;
            }
            break;

        case STATE_CAMP:
            // Navigation depuis le camp
            if (key == KEY_ONE || key == KEY_KP_1)
                game->currentState = STATE_MINE;
            else if (key == KEY_TWO || key == KEY_KP_2)
                game->currentState = STATE_FOREST;
            else if (key == KEY_THREE || key == KEY_KP_3)
                game->currentState = STATE_FORGE;
            else if (key == KEY_FOUR || key == KEY_KP_4)
                game->currentState = STATE_ALCHEMIST;
            else if (key == KEY_FIVE || key == KEY_KP_5)
                game->currentState = STATE_ARCHIFORGE;
            else if (key == KEY_I || key == KEY_KP_6)
                game->currentState = STATE_INVENTORY;
            else if (key == KEY_SIX || key == KEY_KP_7)
            {
                game->currentState = STATE_DUNGEON;
                Dungeon_Enter(&myDungeon);
            }
            else if (pressedQuit)
                game->currentState = STATE_MENU;
            break;

        case STATE_MINE:
            // On gère les clics de la mine
            Clicker_UpdateMine(&game->clicker, viewStartX, viewWidth, h, game->uiFont);
            // Retour au camp avec Q (ou A)
            if (pressedQuit)
                game->currentState = STATE_CAMP;
            break;

        case STATE_FOREST:
            // On gère les clics de la forêt
            Clicker_UpdateForest(&game->clicker, viewStartX, viewWidth, h, game->uiFont);
            // Retour au camp avec Q (ou A)
            if (pressedQuit)
                game->currentState = STATE_CAMP;
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
            }
            break;
        case STATE_FORGE:
        case STATE_ALCHEMIST:
        case STATE_ARCHIFORGE:
        case STATE_INVENTORY:
            // Pour ces 3 menus, on quitte avec Q (ou A)
            if (pressedQuit)
                game->currentState = STATE_CAMP;
            break;
        case STATE_GAMEOVER:
            if (key == KEY_SPACE || key == KEY_ENTER)
            {
                Combat_ResetRun(&game->combat);      // Perd l'XP et les niveaux du donjon
                game->combat.player.hp = 1;          // Renaissance avec 1 PV
                game->currentState     = STATE_CAMP; // Retour à l'abri
            }
            break;
    }
}

void DrawGameUI(GameContext* game, int w, int h)
{
    float leftWidth   = w * 0.25f;
    float rightWidth  = w * 0.20f;
    float centerWidth = w - leftWidth - rightWidth;

    float inventoryHeight = h * 0.62f; // 62% pour l'inventaire
    float mapStartY       = inventoryHeight;
    float mapHeight       = h - mapStartY; // 38% pour la map

    Color uiBorder = DARKGRAY;
    Color uiBg     = (Color){10, 10, 10, 255};

    // Les 3 zones
    DrawRectangle(0, 0, leftWidth, h, uiBg);
    DrawRectangle(w - rightWidth, 0, rightWidth, h, uiBg);

    // Lignes de séparation
    DrawRectangleLinesEx((Rectangle){0, 0, leftWidth, h}, 2, uiBorder);
    DrawRectangleLinesEx((Rectangle){w - rightWidth, 0, rightWidth, h}, 2, uiBorder);
    // Séparateur horizontal entre l'inventaire et la mini-carte
    DrawLine(0, inventoryHeight, leftWidth, inventoryHeight, uiBorder);

    // =========================================================
    // --- EN HAUT DU CENTRE : STATS DU JOUEUR ---
    // =========================================================
    int topBarHeight = 80;
    DrawRectangle(leftWidth, 0, centerWidth, topBarHeight, uiBg);
    DrawRectangleLinesEx((Rectangle){leftWidth, 0, centerWidth, topBarHeight}, 2, uiBorder);

    char statHeader[100];
    sprintf(statHeader, "KAAN ASLANBAS - Niv %d  ( XP: %d/%d )", game->combat.player.level, game->combat.player.xp, game->combat.player.max_xp);
    DrawTextEx(game->uiFont, statHeader, (Vector2){leftWidth + 20, 10}, 24, 1, WHITE);

    char combatStats[100];
    sprintf(combatStats, "HP: %d/%d  |  Mana: %d/%d  |  ATK: %d", game->combat.player.hp, game->combat.player.max_hp, game->combat.player.mana, game->combat.player.max_mana, game->combat.player.atk);
    DrawTextEx(game->uiFont, combatStats, (Vector2){leftWidth + 20, 40}, 20, 1, (game->combat.player.hp < 20) ? RED : GREEN);

    // =========================================================
    // --- PANNEAU GAUCHE : RESSOURCES ET CARTE ---
    // =========================================================
    
    // Ressources du clicker (En haut à gauche)
    DrawTextEx(game->uiFont, T("UI_RESOURCES_TITLE"), (Vector2){20, 20}, 24, 1, LIGHTGRAY);
    DrawTextEx(game->uiFont, TextFormat(T("RES_IRON"), game->clicker.inventory.fer), (Vector2){20, 60}, 20, 1, GRAY);
    DrawTextEx(game->uiFont, TextFormat(T("RES_GOLD"), game->clicker.inventory.or), (Vector2){20, 90}, 20, 1, YELLOW);
    DrawTextEx(game->uiFont, TextFormat(T("RES_CRYSTALS"), game->clicker.inventory.cristaux), (Vector2){20, 120}, 20, 1, PURPLE);
    DrawTextEx(game->uiFont, TextFormat(T("RES_HERBS"), game->clicker.inventory.herbes), (Vector2){20, 150}, 20, 1, GREEN);
    DrawTextEx(game->uiFont, TextFormat(T("RES_WOOD"), game->clicker.inventory.bois), (Vector2){20, 180}, 20, 1, BROWN);
    DrawTextEx(game->uiFont, TextFormat(T("RES_MEAT"), game->clicker.inventory.viande), (Vector2){20, 210}, 20, 1, RED);

    // Mini-carte (En bas à gauche)
    DrawTextEx(game->uiFont, T("MAP_TITLE"), (Vector2){20, mapStartY + 15}, 24, 1, LIGHTGRAY);

    if (game->currentState == STATE_DUNGEON)
    {
        int cell_size = 12;
        int spacing   = 0;

        Color mapWallColor    = DARKGRAY;
        Color mapFloorColor   = (Color){40, 40, 40, 255};
        Color mapStairsColor  = YELLOW;
        Color mapPlayerColor  = WHITE;
        Color mapSpecialColor = PURPLE;

        float gridTotalWidth  = MAP_WIDTH * cell_size;
        float gridTotalHeight = MAP_HEIGHT * cell_size;
        float mapStartX       = (leftWidth / 2.0f) - (gridTotalWidth / 2.0f);
        float mapRenderStartY = mapStartY + 50 + ((mapHeight - 60) / 2.0f) - (gridTotalHeight / 2.0f);

        for (int y = 0; y < MAP_HEIGHT; y++)
        {
            for (int x = 0; x < MAP_WIDTH; x++)
            {
                if (myDungeon.explored[y][x])
                {
                    char    tile = myDungeon.map[y][x];
                    Vector2 pos  = {mapStartX + (x * cell_size), mapRenderStartY + (y * cell_size)};

                    if (x == myDungeon.playerX && y == myDungeon.playerY)
                    {
                        char* dirStr = "^";
                        if (myDungeon.playerDir == DIR_EAST) dirStr = ">";
                        else if (myDungeon.playerDir == DIR_SOUTH) dirStr = "v";
                        else if (myDungeon.playerDir == DIR_WEST) dirStr = "<";
                        DrawTextEx(game->dungeonFont, dirStr, pos, cell_size, spacing, mapPlayerColor);
                    }
                    else if (tile == '#') DrawTextEx(game->dungeonFont, "#", pos, cell_size, spacing, mapWallColor);
                    else if (tile == '.') DrawTextEx(game->dungeonFont, ".", pos, cell_size, spacing, mapFloorColor);
                    else if (tile == '>') DrawTextEx(game->dungeonFont, ">", pos, cell_size, spacing, mapStairsColor);
                    else if (tile == 'E' || tile == 'B') DrawTextEx(game->dungeonFont, "?", pos, cell_size, spacing, mapSpecialColor);
                }
            }
        }
    }

    // =========================================================
    // --- PANNEAU DROIT : COMBAT ET EQUIPEMENT ---
    // =========================================================
    int rightX = w - rightWidth + 20;

    // 1. Journal de bataille
    DrawTextEx(game->uiFont, T("UI_LOG_TITLE"), (Vector2){rightX, 20}, 24, 1, LIGHTGRAY);
    for (int i = 0; i < 5; i++)
    {
        DrawTextEx(game->uiFont, game->combat.battle_log[i], (Vector2){rightX, 60 + (i * 25)}, 20, 1, (i == 0) ? WHITE : GRAY);
    }

    // 2. Equipement Actuel (Remonté à 35% de l'écran pour faire de la place)
    float equipStartY = h * 0.35f; 
    DrawLine(w - rightWidth, equipStartY, w, equipStartY, uiBorder);
    DrawTextEx(game->uiFont, T("UI_EQUIP_VISUAL"), (Vector2){rightX, equipStartY + 10}, 24, 1, LIGHTGRAY);

    const char* slot_names[MAX_SLOTS] = {T("SLOT_HELMET"), T("SLOT_ARMOR"), T("SLOT_GLOVES"), T("SLOT_LEGGINGS"), T("SLOT_BOOTS"), T("SLOT_HAND_1"), T("SLOT_HAND_2")};

    for (int i = 0; i < MAX_SLOTS; i++)
    {
        int  inv_idx = game->combat.player.equipped[i];
        char eq_text[64];
        if (inv_idx == -1)
        {
            sprintf(eq_text, "%s: [ Vide ]", slot_names[i]);
            DrawTextEx(game->uiFont, eq_text, (Vector2){rightX, equipStartY + 45 + (i * 20)}, 20, 1, DARKGRAY);
        }
        else
        {
            OwnedItem* item = &game->combat.player.inventory[inv_idx];
            ItemTemplate* t    = &g_itemDB[item->template_idx];
            sprintf(eq_text, "%s: %s (+%d)", slot_names[i], g_isEnglish ? t->name_en : t->name_fr, item->level);
            DrawTextEx(game->uiFont, eq_text, (Vector2){rightX, equipStartY + 45 + (i * 20)}, 20, 1, WHITE);
        }
    }

    // 3. Potions Equipées
    float magicStartY = equipStartY + 195; // Placé juste sous l'équipement
    DrawLine(w - rightWidth, magicStartY, w, magicStartY, uiBorder);
    DrawTextEx(game->uiFont, "[ CONSOMMABLES ]", (Vector2){rightX, magicStartY + 10}, 24, 1, LIGHTGRAY);
    
    for (int i = 0; i < 3; i++)
    {
        int p_idx = game->combat.player.equipped_potions[i];
        if (p_idx != -1)
        {
            PotionTemplate* t = &g_potionDB[p_idx];
            char            pText[64];
            sprintf(pText, "(%d) %s x%d", i + 1, g_isEnglish ? t->name_en : t->name_fr, game->combat.player.potion_qty[p_idx]);
            DrawTextEx(game->uiFont, pText, (Vector2){rightX, magicStartY + 45 + (i * 20)}, 20, 1, RED);
        }
        else
        {
            char pText[64];
            sprintf(pText, "(%d) [ Vide ]", i + 1);
            DrawTextEx(game->uiFont, pText, (Vector2){rightX, magicStartY + 45 + (i * 20)}, 20, 1, DARKGRAY);
        }
    }

    // 4. Sorts Equipés
    float spellStartY = magicStartY + 115; // Placé juste sous les potions
    DrawLine(w - rightWidth, spellStartY, w, spellStartY, uiBorder);
    DrawTextEx(game->uiFont, "[ SORTS (Cost Mana) ]", (Vector2){rightX, spellStartY + 10}, 24, 1, LIGHTGRAY);
    
    for (int i = 0; i < 3; i++)
    {
        int s_idx = game->combat.player.equipped_spells[i];
        if (s_idx != -1)
        {
            SpellTemplate* t = &g_spellDB[s_idx];
            char           sText[64];
            sprintf(sText, "(%d) %s (%dM)", i + 4, g_isEnglish ? t->name_en : t->name_fr, t->mana_cost);
            DrawTextEx(game->uiFont, sText, (Vector2){rightX, spellStartY + 45 + (i * 20)}, 20, 1, ORANGE);
        }
        else
        {
            char sText[64];
            sprintf(sText, "(%d) [ Vide ]", i + 4);
            DrawTextEx(game->uiFont, sText, (Vector2){rightX, spellStartY + 45 + (i * 20)}, 20, 1, DARKGRAY);
        }
    }
}

// Fonction utilitaire pour dessiner une case de la grille d'équipement avec ASCII
void DrawEquipSlotGrid(GameContext* game, int inv_idx, const char* slot_label, int x, int y, int width, int height)
{
    Color borderColor = DARKGRAY;
    Color asciiColor  = LIGHTGRAY;

    // Dessine le contour de la case
    DrawRectangleLinesEx((Rectangle){x, y, width, height}, 2, borderColor);

    // Titre du slot (ex: "MAIN 1")
    DrawTextEx(game->uiFont, slot_label, (Vector2){x + 5, y + 5}, 20, 1, GRAY);

    if (inv_idx == -1)
    {
        // Slot vide
        Vector2 tSize = MeasureTextEx(game->uiFont, "[ Vide ]", 20, 1);
        DrawTextEx(game->uiFont, "[ Vide ]", (Vector2){x + (width / 2) - (tSize.x / 2), y + (height / 2) - (tSize.y / 2)}, 20, 1, DARKGRAY);
    }
    else
    {
        // Slot occupé : Uniquement l'ASCII art, sans aucun autre texte !
        OwnedItem*    item = &game->combat.player.inventory[inv_idx];
        ItemTemplate* t    = &g_itemDB[item->template_idx];

        int line_height        = 18;
        int ascii_total_height = t->ascii_line_count * line_height;
        int currentY           = y + (height / 2) - (ascii_total_height / 2); // Centre verticalement

        for (int i = 0; i < t->ascii_line_count; i++)
        {
            Vector2 tSize = MeasureTextEx(game->dungeonFont, t->ascii[i], 18, 1);
            // Centre horizontalement chaque ligne d'ASCII
            DrawTextEx(game->dungeonFont, t->ascii[i], (Vector2){x + (width / 2) - (tSize.x / 2), currentY + (i * line_height)}, 18, 1, asciiColor);
        }
    }
}

// --- LE RENDER DE L'INVENTAIRE ---
void Game_RenderInventory(GameContext* game, int w, int h)
{
    int startX      = (w * 0.25f) + 30;
    int centerWidth = w * 0.55f;

    DrawLine(startX, 80, startX + centerWidth, 80, DARKGRAY);

    int visualWidth = centerWidth * 0.45f;
    int listWidth   = centerWidth * 0.55f;

    DrawLine(startX + visualWidth, 80, startX + visualWidth, h, DARKGRAY);

    // --- ZONE GAUCHE : VISUEL EQUIPEMENT ---
    DrawTextEx(game->uiFont, T("UI_EQUIP_VISUAL"), (Vector2){startX + 20, 100}, 24, 1, WHITE);

    int cell_w = (visualWidth - 60) / 3;
    int cell_h = 90; // Réduit à 90 pour faire rentrer 5 lignes verticales
    int cx     = startX + 20;
    int cy     = 135;

    // Ligne 1 : Casque
    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_HELMET], T("SLOT_HELMET"), cx + cell_w + 10, cy, cell_w, cell_h);

    // Ligne 2 : Main 1 | Armure | Main 2
    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_HAND_1], T("SLOT_HAND_1"), cx, cy + cell_h + 5, cell_w, cell_h);
    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_ARMOR], T("SLOT_ARMOR"), cx + cell_w + 10, cy + cell_h + 5, cell_w, cell_h);
    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_HAND_2], T("SLOT_HAND_2"), cx + (cell_w * 2) + 20, cy + cell_h + 5, cell_w, cell_h);

    // Ligne 3 : Gants
    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_GLOVES], T("SLOT_GLOVES"), cx + cell_w + 10, cy + (cell_h * 2) + 10, cell_w, cell_h);

    // Ligne 4 : Jambières
    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_LEGGINGS], T("SLOT_LEGGINGS"), cx + cell_w + 10, cy + (cell_h * 3) + 15, cell_w, cell_h);

    // Ligne 5 : Bottes
    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_BOOTS], T("SLOT_BOOTS"), cx + cell_w + 10, cy + (cell_h * 4) + 20, cell_w, cell_h);

    // --- ZONE DROITE : LISTE INVENTAIRE ---
    int listX = startX + visualWidth + 20;
    DrawTextEx(game->uiFont, T("UI_INV_LIST"), (Vector2){listX, 100}, 24, 1, WHITE);

    int sorted_indices[MAX_INVENTORY];
    Inventory_GetSortedIndices(&game->combat, sorted_indices);

    for (int i = 0; i < game->combat.player.inventory_count; i++)
    {
        int           inv_idx = sorted_indices[i];
        OwnedItem*    item    = &game->combat.player.inventory[inv_idx];
        ItemTemplate* t       = &g_itemDB[item->template_idx];

        bool is_eq = false;
        // CORRECTION : MAX_SLOTS
        for (int j = 0; j < MAX_SLOTS; j++)
            if (game->combat.player.equipped[j] == inv_idx)
                is_eq = true;

        char btn_text[128];
        sprintf(btn_text, "%s %s (Niv %d)", is_eq ? "[E]" : "[ ]", g_isEnglish ? t->name_en : t->name_fr, item->level);

        if (DoShopButton(game->uiFont, btn_text, listX, 150 + (i * 30), 20, true))
        {
            if (is_eq)
            {
                // CORRECTION : MAX_SLOTS
                for (int j = 0; j < MAX_SLOTS; j++)
                {
                    if (game->combat.player.equipped[j] == inv_idx)
                    {
                        game->combat.player.equipped[j] = -1;
                    }
                }
            }
            else
            {
                Inventory_Equip(&game->combat, inv_idx);
            }
            Combat_RecalculateStats(&game->combat);
        }
    }
}

void Game_RenderForge(GameContext* game, int w, int h)
{
    int startX      = (w * 0.25f) + 30; // Zone centrale
    int centerWidth = w * 0.55f;

    DrawLine(startX, 80, startX + centerWidth, 80, DARKGRAY);

    // LAYOUT : GAUCHE (Liste Équipement) | DROITE (Atelier Amélioration)
    int listWidth = centerWidth * 0.45f; // 45% pour la liste
    int shopWidth = centerWidth * 0.55f; // 55% pour l'atelier

    DrawLine(startX + listWidth, 80, startX + listWidth, h, DARKGRAY);

    // --- ZONE GAUCHE : LISTE ÉQUIPEMENT (Triée) ---
    int listX = startX + 20;
    DrawTextEx(game->uiFont, T("UI_FORGE_LIST"), (Vector2){listX, 100}, 24, 1, WHITE);

    // Récupère les indices triés (Equipés d'abord, puis nouveaux)
    int sorted_indices[MAX_INVENTORY];
    Inventory_GetSortedIndices(&game->combat, sorted_indices);

    for (int i = 0; i < game->combat.player.inventory_count; i++)
    {
        int           inv_idx = sorted_indices[i]; // Utilise l'index trié
        OwnedItem*    item    = &game->combat.player.inventory[inv_idx];
        ItemTemplate* t       = &g_itemDB[item->template_idx];

        bool is_eq = false;
        for (int j = 0; j < MAX_SLOTS; j++)
            if (game->combat.player.equipped[j] == inv_idx)
                is_eq = true;

        char itemText[128];
        sprintf(itemText, "%s %s (Niv %d)", is_eq ? "[E]" : "[ ]", g_isEnglish ? t->name_en : t->name_fr, item->level);

        Color itemColor = (inv_idx == selectedForgeIdx) ? WHITE : GRAY; // Blanc si sélectionné pour upgrade

        // DoShopButton pour la sélection
        if (DoShopButton(game->uiFont, itemText, listX, 150 + (i * 25), 18, true))
        {
            selectedForgeIdx = inv_idx; // Sélectionne l'objet pour l'améliorer
        }
    }

    // --- ZONE DROITE : ATELIER D'AMÉLIORATION ---
    int shopX = startX + listWidth + 20;
    DrawTextEx(game->uiFont, T("UI_FORGE_CENTER"), (Vector2){shopX, 100}, 24, 1, WHITE);

    if (selectedForgeIdx == -1)
    {
        // Aucun objet sélectionné
        DrawTextCentered(game->uiFont, T("UI_FORGE_SELECT_PROMPT"), shopX + (shopWidth / 2) - 20, h / 2, 20, 1, DARKGRAY);
    }
    else
    {
        // Affichage de l'objet sélectionné et des coûts d'upgrade
        OwnedItem*    item = &game->combat.player.inventory[selectedForgeIdx];
        ItemTemplate* t    = &g_itemDB[item->template_idx];

        // 1. ASCII Art (Centré)
        int line_height = 20;
        int asciiStartY = 150;
        for (int i = 0; i < t->ascii_line_count; i++)
        {
            Vector2 tSize = MeasureTextEx(game->dungeonFont, t->ascii[i], 20, 1);
            DrawTextEx(game->dungeonFont, t->ascii[i], (Vector2){shopX + (shopWidth / 2) - (tSize.x / 2) - 20, asciiStartY + (i * line_height)}, 20, 1, ORANGE);
        }

        // 2. Stats Actuelles vs Next (Calcul dynamique)
        int  infoY = asciiStartY + (t->ascii_line_count * line_height) + 30;
        char statsText[128];
        sprintf(statsText, "%s (Niv %d -> %d)", g_isEnglish ? t->name_en : t->name_fr, item->level, item->level + 1);
        DrawTextEx(game->uiFont, statsText, (Vector2){shopX, infoY}, 20, 1, WHITE);

        // Affichage de la stat principale (Ex: ATK si >0, sinon HP)
        if (t->inc_atk > 0)
        {
            int cur_atk  = t->atk + (item->level * t->inc_atk);
            int next_atk = cur_atk + t->inc_atk;
            sprintf(statsText, "ATK: %d -> %d", cur_atk, next_atk);
        }
        else if (t->inc_hp > 0)
        {
            int cur_hp  = t->hp + (item->level * t->inc_hp);
            int next_hp = cur_hp + t->inc_hp;
            sprintf(statsText, "HP: %d -> %d", cur_hp, next_hp);
        }
        DrawTextEx(game->uiFont, statsText, (Vector2){shopX, infoY + 30}, 20, 1, GREEN);

        // 3. Coûts d'Upgrade (La formule Mathématique pour extensibilité !)
        int costY = infoY + 70;
        DrawTextEx(game->uiFont, "[ COUTS AMELIORATION ]", (Vector2){shopX, costY}, 20, 1, LIGHTGRAY);

        // Formule du coût = Base + (Niveau * Incrément)
        int cur_cost_fer  = t->cost_fer_base + (item->level * t->cost_fer_inc);
        int cur_cost_bois = t->cost_bois_base + (item->level * t->cost_bois_inc);

        bool canAfford = true;

        // Affichage dynamique des coûts s'ils sont > 0
        int costLineY = costY + 30;
        if (cur_cost_fer > 0)
        {
            bool hasFer = game->clicker.inventory.fer >= cur_cost_fer;
            if (!hasFer)
                canAfford = false;
            DrawTextEx(game->uiFont, TextFormat("Fer: %d", cur_cost_fer), (Vector2){shopX, costLineY}, 20, 1, hasFer ? GRAY : RED);
            costLineY += 25;
        }
        if (cur_cost_bois > 0)
        {
            bool hasBois = game->clicker.inventory.bois >= cur_cost_bois;
            if (!hasBois)
                canAfford = false;
            DrawTextEx(game->uiFont, TextFormat("Bois: %d", cur_cost_bois), (Vector2){shopX, costLineY}, 20, 1, hasBois ? BROWN : RED);
            costLineY += 25;
        }

        // 4. LE BOUTON UPGRADE (Au milieu)
        Rectangle upgradeRect  = {shopX + (shopWidth / 2) - 100, costLineY + 30, 200, 50};
        bool      hoverUpgrade = CheckCollisionPointRec(GetMousePosition(), upgradeRect);

        if (canAfford)
        {
            DrawRectangleRec(upgradeRect, hoverUpgrade ? WHITE : GREEN);
            DrawTextCentered(game->uiFont, T("BTN_UPGRADE"), upgradeRect.x + 100, upgradeRect.y + 15, 24, 1, BLACK);

            // Logique de l'upgrade
            if (hoverUpgrade && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                // Retire les ressources
                game->clicker.inventory.fer -= cur_cost_fer;
                game->clicker.inventory.bois -= cur_cost_bois;
                // Augmente le niveau !
                item->level++;
                // Recalcule les stats du perso
                Combat_RecalculateStats(&game->combat);
                Combat_AddLog(&game->combat, T("LOG_UPGRADE_SUCCESS"));
            }
        }
        else
        {
            // Bouton grisé si pas assez de ressources
            DrawRectangleRec(upgradeRect, DARKGRAY);
            DrawTextCentered(game->uiFont, T("BTN_UPGRADE"), upgradeRect.x + 100, upgradeRect.y + 15, 24, 1, GRAY);
        }
    }
}

void Game_RenderArchiforge(GameContext* game, int w, int h)
{
    int startX      = (w * 0.25f) + 30;
    int centerWidth = w * 0.55f;
    int listWidth   = centerWidth * 0.45f;
    int shopX       = startX + listWidth + 20;

    // --- LISTE GAUCHE ---
    for (int i = 0; i < g_spellCount; i++)
    {
        SpellTemplate* t        = &g_spellDB[i];
        bool           unlocked = game->combat.player.spell_unlocked[i];
        char           text[64];
        if (unlocked)
            sprintf(text, "%s (Niv %d)", g_isEnglish ? t->name_en : t->name_fr, game->combat.player.spell_level[i]);
        else
            sprintf(text, "[LOCKED] %s", g_isEnglish ? t->name_en : t->name_fr);

        if (DoShopButton(game->uiFont, text, startX, 150 + (i * 30), 20, true))
            selectedSpellIdx = i;
    }

    // --- ATELIER DROITE ---
    if (selectedSpellIdx != -1)
    {
        SpellTemplate* t        = &g_spellDB[selectedSpellIdx];
        bool           unlocked = game->combat.player.spell_unlocked[selectedSpellIdx];

        DrawTextEx(game->uiFont, g_isEnglish ? t->name_en : t->name_fr, (Vector2){shopX, 150}, 30, 1, WHITE);

        if (!unlocked)
        {
            // BOUTON APPRENDRE
            char costText[64];
            sprintf(costText, "Cout: %d Or, %d Cristal", t->learn_gold, t->learn_crystal);
            DrawTextEx(game->uiFont, costText, (Vector2){shopX, 200}, 20, 1, GRAY);

            bool canAfford = (game->clicker.inventory.or >= t->learn_gold && game->clicker.inventory.cristaux >= t->learn_crystal);
            if (DoShopButton(game->uiFont, "[ APPRENDRE ]", shopX, 250, 24, canAfford))
            {
                game->clicker.inventory.or -= t->learn_gold;
                game->clicker.inventory.cristaux -= t->learn_crystal;
                game->combat.player.spell_unlocked[selectedSpellIdx] = true;
            }
        }
        else
        {
            // BOUTON UPGRADE
            int  lvl      = game->combat.player.spell_level[selectedSpellIdx];
            int  upg_cost = t->upg_gold_base + (lvl * t->upg_gold_inc);
            bool canUpg   = (game->clicker.inventory.or >= upg_cost && lvl < 10);

            if (DoShopButton(game->uiFont, TextFormat("[ AMELIORER (-%d Or) ]", upg_cost), shopX, 250, 24, canUpg))
            {
                game->clicker.inventory.or -= upg_cost;
                game->combat.player.spell_level[selectedSpellIdx]++;
            }

            // BOUTONS PREPARER (EQUIPER DANS UN SLOT 1, 2 ou 3)
            DrawTextEx(game->uiFont, "Equiper dans le slot :", (Vector2){shopX, 320}, 20, 1, LIGHTGRAY);
            bool canPrep = (game->clicker.inventory.cristaux >= t->prep_crystal);

            for (int slot = 0; slot < 3; slot++)
            {
                if (DoShopButton(game->uiFont, TextFormat("[ Slot %d (-%d Cristal) ]", slot + 1, t->prep_crystal), shopX + (slot * 150), 360, 20, canPrep))
                {
                    game->clicker.inventory.cristaux -= t->prep_crystal;
                    game->combat.player.equipped_spells[slot] = selectedSpellIdx;
                }
            }
        }
    }
}

void Game_RenderAlchemist(GameContext* game, int w, int h) {
    int startX = (w * 0.25f) + 30; 
    int centerWidth = w * 0.55f;
    int listWidth = centerWidth * 0.45f; 
    int shopX = startX + listWidth + 20;
    
    DrawLine(startX, 80, startX + centerWidth, 80, DARKGRAY);
    DrawLine(startX + listWidth, 80, startX + listWidth, h, DARKGRAY);

    DrawTextEx(game->uiFont, "=== L'ALCHIMISTE ===", (Vector2){startX + 20, 100}, 24, 1, PURPLE);

    // --- LISTE GAUCHE ---
    for (int i = 0; i < g_potionCount; i++) {
        PotionTemplate* t = &g_potionDB[i];
        bool unlocked = game->combat.player.potion_unlocked[i];
        char text[128];
        
        if (unlocked) {
            sprintf(text, "%s (Niv %d) x%d", g_isEnglish ? t->name_en : t->name_fr, game->combat.player.potion_level[i], game->combat.player.potion_qty[i]);
        } else {
            sprintf(text, "[LOCKED] %s", g_isEnglish ? t->name_en : t->name_fr);
        }

        // On rend le bouton cliquable pour sélectionner la potion
        if (DoShopButton(game->uiFont, text, startX + 20, 150 + (i * 30), 20, true)) {
            selectedPotionIdx = i;
        }
    }

    // --- ATELIER DROITE ---
    if (selectedPotionIdx != -1) {
        PotionTemplate* t = &g_potionDB[selectedPotionIdx];
        bool unlocked = game->combat.player.potion_unlocked[selectedPotionIdx];

        DrawTextEx(game->uiFont, g_isEnglish ? t->name_en : t->name_fr, (Vector2){shopX, 150}, 30, 1, WHITE);

        if (!unlocked) {
            // BOUTON APPRENDRE
            char costText[64]; sprintf(costText, "Cout: %d Or", t->learn_gold);
            DrawTextEx(game->uiFont, costText, (Vector2){shopX, 200}, 20, 1, GRAY);
            
            bool canAfford = (game->clicker.inventory.or >= t->learn_gold);
            if (DoShopButton(game->uiFont, "[ APPRENDRE ]", shopX, 250, 24, canAfford)) {
                game->clicker.inventory.or -= t->learn_gold;
                game->combat.player.potion_unlocked[selectedPotionIdx] = true;
            }
        } else {
            // BOUTONS AMELIORER ET CRAFTER
            int lvl = game->combat.player.potion_level[selectedPotionIdx];
            int upg_cost = t->upg_gold_base + (lvl * t->upg_gold_inc);
            int craft_cost = t->craft_herbs_base + (lvl * t->craft_herbs_inc);

            bool canUpg = (game->clicker.inventory.or >= upg_cost && lvl < 10);
            if (DoShopButton(game->uiFont, TextFormat("[ AMELIORER (-%d Or) ]", upg_cost), shopX, 200, 20, canUpg)) {
                game->clicker.inventory.or -= upg_cost;
                game->combat.player.potion_level[selectedPotionIdx]++;
            }

            bool canCraft = (game->clicker.inventory.herbes >= craft_cost);
            if (DoShopButton(game->uiFont, TextFormat("[ CRAFTER (-%d Herbes) ]", craft_cost), shopX, 250, 24, canCraft)) {
                game->clicker.inventory.herbes -= craft_cost;
                game->combat.player.potion_qty[selectedPotionIdx]++;
            }

            // GESTION DE L'ÉQUIPEMENT (SLOTS 1, 2, 3)
            DrawTextEx(game->uiFont, "Equiper dans le slot (Touches 1, 2, 3):", (Vector2){shopX, 320}, 20, 1, LIGHTGRAY);
            for(int slot = 0; slot < 3; slot++) {
                bool is_eq = (game->combat.player.equipped_potions[slot] == selectedPotionIdx);
                const char* btnLabel = is_eq ? TextFormat("[E] Slot %d", slot+1) : TextFormat("[ ] Slot %d", slot+1);
                
                if (DoShopButton(game->uiFont, btnLabel, shopX + (slot * 110), 360, 20, true)) {
                    if (is_eq) {
                        game->combat.player.equipped_potions[slot] = -1; // Déséquipe
                    } else {
                        game->combat.player.equipped_potions[slot] = selectedPotionIdx; // Equipe
                    }
                }
            }
        }
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
            // --- TIMER POUR LE CRÉPITEMENT ---
            // On change l'état du feu toutes les 0.15 secondes
            float frameTime = 0.30f;
            int   seed      = (int)(GetTime() / frameTime);

            // On "fixe" le hasard pour cette frame précise
            SetRandomSeed(seed);
            int   cx            = (w * 0.2f) + ((w * 0.55f) / 2);
            int   cy            = h / 2;
            float asciiFontSize = 20;
            float spacing       = 2;

            DrawTextEx(game->uiFont, T("CAMP_TITLE"), (Vector2){cx - 100, 120}, 40, 1, GREEN);
            // --- RENDU DE LA FUMÉE ---
            for (int s = 0; s < 3; s++)
            {
                // Ces positions ne changeront que toutes les 0.15s grâce au seed
                int   smokeX   = cx + GetRandomValue(-40, 40);
                int   smokeY   = cy - 130 + GetRandomValue(-20, 20);
                Color smokeCol = (Color){120, 120, 120, (unsigned char)GetRandomValue(100, 180)};

                DrawTextEx(game->dungeonFont, "▒", (Vector2){(float)smokeX, (float)smokeY}, asciiFontSize, spacing, smokeCol);
            }

            // --- 2. L'ASCII DU FEU (SANS LA FUMÉE STATIQUE) ---
            const char* fireAscii[] = {"                       ", // Ligne vide pour laisser place à la fumée
                                       "    ▓██▄      ▓██▄     ",          "    ▀███  ███████████   ",         "         █████████████  ",        "   ▄████  ████████████▀  ",
                                       "  ████████████▓▀███▀████     ▄▄ ", "  █████████████▀   █████▄   ▄███", "  ▀██████████▀     ▀███████████", "   ████████      ▄▄   ▀███████▀",
                                       "    ▀▀█████████████     ██████ ",  "  ▄▄██████████████▀█▄█████████▄▄", "  █████████████████████████▀▀▀",  "   ▀███████████▀  ▀█████████▀ "};

            int lineCount = 13;
            for (int i = 0; i < lineCount; i++)
            {
                float flickerX = (i < 11) ? (float)GetRandomValue(-1, 1) : 0;

                Vector2 textSize = MeasureTextEx(game->dungeonFont, fireAscii[i], asciiFontSize, spacing);
                Vector2 pos      = {cx - (textSize.x / 2) + flickerX, cy - 100 + (i * asciiFontSize)};

                Color col;
                int   intensity = GetRandomValue(0, 40);

                if (i <= 4)
                {
                    col = (Color){255, 255 - intensity, intensity, 255}; // Jaune/Blanc
                }
                else if (i <= 8)
                {
                    col = (Color){255, 160 - intensity, 0, 255}; // Orange
                }
                else if (i <= 10)
                {
                    col = (Color){220 - intensity, 20, 0, 255}; // Rouge
                }
                else
                {
                    col = (Color){100, 60, 30, 255}; // Bûches
                }

                DrawTextEx(game->dungeonFont, fireAscii[i], (Vector2){pos.x + flickerX, pos.y}, asciiFontSize, spacing, col);
            }
            DrawTextEx(game->uiFont, T("CAMP_BTN_MINE"), (Vector2){cx - 150, h - 320}, 24, 1, LIGHTGRAY);
            DrawTextEx(game->uiFont, T("CAMP_BTN_FOREST"), (Vector2){cx - 150, h - 280}, 24, 1, GREEN);
            DrawTextEx(game->uiFont, T("CAMP_BTN_FORGE"), (Vector2){cx - 150, h - 240}, 24, 1, ORANGE);
            DrawTextEx(game->uiFont, T("CAMP_BTN_ALCHEMIST"), (Vector2){cx - 150, h - 200}, 24, 1, PURPLE);
            DrawTextEx(game->uiFont, T("CAMP_BTN_ARCHIFORGE"), (Vector2){cx - 150, h - 160}, 24, 1, BLUE);
            DrawTextEx(game->uiFont, T("CAMP_BTN_INVENTORY"), (Vector2){cx - 150, h - 120}, 24, 1, YELLOW);
            DrawTextEx(game->uiFont, T("CAMP_BTN_DUNGEON"), (Vector2){cx - 150, h - 80}, 24, 1, RED);
            DrawTextEx(game->uiFont, T("CAMP_BTN_MAIN_MENU"), (Vector2){cx - 150, h - 40}, 20, 1, DARKGRAY);
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
        DrawGameUI(game, w, h);
    }

    EndDrawing();
}

void Game_Run(GameContext* game)
{
    while (game->isRunning && !WindowShouldClose())
    {
        Game_Update(game);
        Game_Render(game);
    }
}

static void SaveGame(GameContext* game)
{
    cJSON* root = cJSON_CreateObject();

    // Sauvegarde de la langue et du donjon
    cJSON_AddBoolToObject(root, "is_english", g_isEnglish);
    cJSON_AddNumberToObject(root, "highest_floor", myDungeon.highest_floor);
    
    // Sauvegarde de l'inventaire des ressources
    cJSON_AddNumberToObject(root, "fer", game->clicker.inventory.fer);
    cJSON_AddNumberToObject(root, "or", game->clicker.inventory.or);
    cJSON_AddNumberToObject(root, "cristaux", game->clicker.inventory.cristaux);
    cJSON_AddNumberToObject(root, "bois", game->clicker.inventory.bois);
    cJSON_AddNumberToObject(root, "viande", game->clicker.inventory.viande);
    cJSON_AddNumberToObject(root, "herbes", game->clicker.inventory.herbes);

    // Sauvegarde des auto-clickers
    cJSON_AddNumberToObject(root, "auto_fer", game->clicker.inventory.auto_fer);
    cJSON_AddNumberToObject(root, "auto_bois", game->clicker.inventory.auto_bois);
    cJSON_AddNumberToObject(root, "auto_or", game->clicker.inventory.auto_or);
    cJSON_AddNumberToObject(root, "auto_cristaux", game->clicker.inventory.auto_cristaux);
    cJSON_AddNumberToObject(root, "auto_viande", game->clicker.inventory.auto_viande);
    cJSON_AddNumberToObject(root, "auto_herbes", game->clicker.inventory.auto_herbes);

    // 1. Sauvegarde des Sorts (Tableaux)
    cJSON* sp_unl = cJSON_CreateArray();
    cJSON* sp_lvl = cJSON_CreateArray();
    for(int i = 0; i < g_spellCount; i++) {
        cJSON_AddItemToArray(sp_unl, cJSON_CreateBool(game->combat.player.spell_unlocked[i]));
        cJSON_AddItemToArray(sp_lvl, cJSON_CreateNumber(game->combat.player.spell_level[i]));
    }
    cJSON_AddItemToObject(root, "spell_unlocked", sp_unl);
    cJSON_AddItemToObject(root, "spell_level", sp_lvl);

    // 2. Sauvegarde des Potions (Tableaux)
    cJSON* po_unl = cJSON_CreateArray();
    cJSON* po_lvl = cJSON_CreateArray();
    cJSON* po_qty = cJSON_CreateArray();
    for(int i = 0; i < g_potionCount; i++) {
        cJSON_AddItemToArray(po_unl, cJSON_CreateBool(game->combat.player.potion_unlocked[i]));
        cJSON_AddItemToArray(po_lvl, cJSON_CreateNumber(game->combat.player.potion_level[i]));
        cJSON_AddItemToArray(po_qty, cJSON_CreateNumber(game->combat.player.potion_qty[i]));
    }
    cJSON_AddItemToObject(root, "potion_unlocked", po_unl);
    cJSON_AddItemToObject(root, "potion_level", po_lvl);
    cJSON_AddItemToObject(root, "potion_qty", po_qty);

    // 3. Sauvegarde des Équipements Magiques (Slots 1,2,3)
    cJSON* eq_sp = cJSON_CreateArray();
    cJSON* eq_po = cJSON_CreateArray();
    for(int i = 0; i < 3; i++) {
        cJSON_AddItemToArray(eq_sp, cJSON_CreateNumber(game->combat.player.equipped_spells[i]));
        cJSON_AddItemToArray(eq_po, cJSON_CreateNumber(game->combat.player.equipped_potions[i]));
    }
    cJSON_AddItemToObject(root, "equipped_spells", eq_sp);
    cJSON_AddItemToObject(root, "equipped_potions", eq_po);

    //  4. Sauvegarde de l'Inventaire d'Equipement Physique ---
    cJSON_AddNumberToObject(root, "inventory_count", game->combat.player.inventory_count);
    
    cJSON* inv_arr = cJSON_CreateArray();
    for (int i = 0; i < game->combat.player.inventory_count; i++) {
        cJSON* itemObj = cJSON_CreateObject();
        // On sauvegarde l'identifiant du template et le niveau d'amélioration
        cJSON_AddNumberToObject(itemObj, "template_idx", game->combat.player.inventory[i].template_idx);
        cJSON_AddNumberToObject(itemObj, "level", game->combat.player.inventory[i].level);
        cJSON_AddItemToArray(inv_arr, itemObj);
    }
    cJSON_AddItemToObject(root, "inventory", inv_arr);

    //  5. Sauvegarde des Equipements Actifs (Armure, Casque, etc.) ---
    cJSON* eq_arr = cJSON_CreateArray();
    for (int i = 0; i < MAX_SLOTS; i++) {
        cJSON_AddItemToArray(eq_arr, cJSON_CreateNumber(game->combat.player.equipped[i]));
    }
    cJSON_AddItemToObject(root, "equipped", eq_arr);

    char* jsonStr = cJSON_Print(root);
    SaveFileText("save.json", jsonStr); 

    free(jsonStr);
    cJSON_Delete(root);
}

static void LoadGame(GameContext* game)
{
    char* file = LoadFileText("save.json");
    if (!file) return;

    cJSON* root = cJSON_Parse(file);
    if (!root) { UnloadFileText(file); return; }

    cJSON* langNode = cJSON_GetObjectItem(root, "is_english");
    if (langNode) g_isEnglish = langNode->valueint; 

    cJSON* hf = cJSON_GetObjectItem(root, "highest_floor");
    if (hf) myDungeon.highest_floor = hf->valueint;

    game->clicker.inventory.fer      = cJSON_GetObjectItem(root, "fer") ? cJSON_GetObjectItem(root, "fer")->valueint : 0;
    game->clicker.inventory.or       = cJSON_GetObjectItem(root, "or") ? cJSON_GetObjectItem(root, "or")->valueint : 0;
    game->clicker.inventory.cristaux = cJSON_GetObjectItem(root, "cristaux") ? cJSON_GetObjectItem(root, "cristaux")->valueint : 0;
    game->clicker.inventory.bois     = cJSON_GetObjectItem(root, "bois") ? cJSON_GetObjectItem(root, "bois")->valueint : 0;
    game->clicker.inventory.viande   = cJSON_GetObjectItem(root, "viande") ? cJSON_GetObjectItem(root, "viande")->valueint : 0;
    game->clicker.inventory.herbes   = cJSON_GetObjectItem(root, "herbes") ? cJSON_GetObjectItem(root, "herbes")->valueint : 0;

    game->clicker.inventory.auto_fer  = cJSON_GetObjectItem(root, "auto_fer") ? cJSON_GetObjectItem(root, "auto_fer")->valueint : 0;
    game->clicker.inventory.auto_bois = cJSON_GetObjectItem(root, "auto_bois") ? cJSON_GetObjectItem(root, "auto_bois")->valueint : 0;
    game->clicker.inventory.auto_or  = cJSON_GetObjectItem(root, "auto_or") ? cJSON_GetObjectItem(root, "auto_or")->valueint : 0;
    game->clicker.inventory.auto_cristaux = cJSON_GetObjectItem(root, "auto_cristaux") ? cJSON_GetObjectItem(root, "auto_cristaux")->valueint : 0;
    game->clicker.inventory.auto_viande = cJSON_GetObjectItem(root, "auto_viande") ? cJSON_GetObjectItem(root, "auto_viande")->valueint : 0;
    game->clicker.inventory.auto_herbes = cJSON_GetObjectItem(root, "auto_herbes") ? cJSON_GetObjectItem(root, "auto_herbes")->valueint : 0;

    #define LOAD_INT_ARRAY(jsonName, cArray, maxSize) \
        do { \
            cJSON* arr = cJSON_GetObjectItem(root, jsonName); \
            if (arr) { \
                int i = 0; cJSON* item; \
                cJSON_ArrayForEach(item, arr) { \
                    if (i < maxSize) { cArray[i] = item->valueint; i++; } \
                } \
            } \
        } while(0)

    // Chargement de la magie
    LOAD_INT_ARRAY("spell_unlocked", game->combat.player.spell_unlocked, MAX_SPELLS_DB);
    LOAD_INT_ARRAY("spell_level", game->combat.player.spell_level, MAX_SPELLS_DB);
    LOAD_INT_ARRAY("potion_unlocked", game->combat.player.potion_unlocked, MAX_POTIONS_DB);
    LOAD_INT_ARRAY("potion_level", game->combat.player.potion_level, MAX_POTIONS_DB);
    LOAD_INT_ARRAY("potion_qty", game->combat.player.potion_qty, MAX_POTIONS_DB);
    LOAD_INT_ARRAY("equipped_spells", game->combat.player.equipped_spells, 3);
    LOAD_INT_ARRAY("equipped_potions", game->combat.player.equipped_potions, 3);

    //  Chargement de l'Inventaire Physique ---
    cJSON* inv_count_node = cJSON_GetObjectItem(root, "inventory_count");
    if (inv_count_node) {
        game->combat.player.inventory_count = inv_count_node->valueint;
        cJSON* inv_arr = cJSON_GetObjectItem(root, "inventory");
        if (inv_arr) {
            int i = 0;
            cJSON* itemNode = NULL;
            cJSON_ArrayForEach(itemNode, inv_arr) {
                if (i < MAX_INVENTORY) {
                    game->combat.player.inventory[i].template_idx = cJSON_GetObjectItem(itemNode, "template_idx")->valueint;
                    game->combat.player.inventory[i].level = cJSON_GetObjectItem(itemNode, "level")->valueint;
                    i++;
                }
            }
        }
    }

    //  Chargement des Équipements ---
    LOAD_INT_ARRAY("equipped", game->combat.player.equipped, MAX_SLOTS);

    cJSON_Delete(root);
    UnloadFileText(file);

    // Recalcule toutes les stats avec l'équipement chargé
    Combat_RecalculateStats(&game->combat);
}

bool DoShopButton(Font font, const char* text, int x, int y, int fontSize, bool canAfford)
{
    Vector2   textSize  = MeasureTextEx(font, text, fontSize, 1);
    Rectangle hitbox    = {x, y, textSize.x, textSize.y};
    bool      isHovered = CheckCollisionPointRec(GetMousePosition(), hitbox);

    Color drawColor = GRAY;
    if (canAfford)
        drawColor = isHovered ? WHITE : LIGHTGRAY;
    else
        drawColor = DARKGRAY; // Grisé si on ne peut pas acheter

    DrawTextEx(font, text, (Vector2){x, y}, fontSize, 1, drawColor);

    return isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && canAfford;
}
