#include "ui.h"
#include "../../utils/inc/lang.h"
#include <stdio.h>
#include <string.h>

// Déclarations externes nécessaires pour accéder aux bases de données du jeu
extern PotionTemplate g_potionDB[MAX_POTIONS_DB];
extern SpellTemplate  g_spellDB[MAX_SPELLS_DB];
extern ItemTemplate   g_itemDB[];
extern bool           g_isEnglish;

const char* GetEffectString(int effect)
{
    if (effect == 1)
        return " [Feu]";
    if (effect == 2)
        return " [Poison]";
    if (effect == 3)
        return " [Vamp]";
    if (effect == 4)
        return " [Vif]";
    return "";
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

void DrawEquipSlotGrid(GameContext* game, int inv_idx, const char* slot_label, int x, int y, int width, int height)
{
    Color borderColor = DARKGRAY;
    Color asciiColor  = LIGHTGRAY;

    DrawRectangleLinesEx((Rectangle){x, y, width, height}, 2, borderColor);
    DrawTextEx(game->uiFont, slot_label, (Vector2){x + 5, y + 5}, 20, 1, GRAY);

    if (inv_idx == -1)
    {
        Vector2 tSize = MeasureTextEx(game->uiFont, "[ Vide ]", 20, 1);
        DrawTextEx(game->uiFont, "[ Vide ]", (Vector2){x + (width / 2) - (tSize.x / 2), y + (height / 2) - (tSize.y / 2)}, 20, 1, DARKGRAY);
    }
    else
    {
        OwnedItem*    item = &game->combat.player.inventory[inv_idx];
        ItemTemplate* t    = &g_itemDB[item->template_idx];

        int line_height        = 18;
        int ascii_total_height = t->ascii_line_count * line_height;
        int currentY           = y + (height / 2) - (ascii_total_height / 2);

        for (int i = 0; i < t->ascii_line_count; i++)
        {
            Vector2 tSize = MeasureTextEx(game->dungeonFont, t->ascii[i], 18, 1);
            DrawTextEx(game->dungeonFont, t->ascii[i], (Vector2){x + (width / 2) - (tSize.x / 2), currentY + (i * line_height)}, 18, 1, asciiColor);
        }
    }
}

void DrawGameUI(GameContext* game, DungeonContext* dungeon, int w, int h)
{
    float leftWidth   = w * 0.25f;
    float rightWidth  = w * 0.20f;
    float centerWidth = w - leftWidth - rightWidth;

    float inventoryHeight = h * 0.62f;
    float mapStartY       = inventoryHeight;
    float mapHeight       = h - mapStartY;

    Color uiBorder = DARKGRAY;
    Color uiBg     = (Color){10, 10, 10, 255};

    DrawRectangle(0, 0, leftWidth, h, uiBg);
    DrawRectangle(w - rightWidth, 0, rightWidth, h, uiBg);
    DrawRectangleLinesEx((Rectangle){0, 0, leftWidth, h}, 2, uiBorder);
    DrawRectangleLinesEx((Rectangle){w - rightWidth, 0, rightWidth, h}, 2, uiBorder);
    DrawLine(0, inventoryHeight, leftWidth, inventoryHeight, uiBorder);

    // --- EN HAUT DU CENTRE : STATS DU JOUEUR ---
    int topBarHeight = 80;
    DrawRectangle(leftWidth, 0, centerWidth, topBarHeight, uiBg);
    DrawRectangleLinesEx((Rectangle){leftWidth, 0, centerWidth, topBarHeight}, 2, uiBorder);

    char statHeader[100];
    sprintf(statHeader, "KAAN ASLANBAS - Niv %d  ( XP: %d/%d )", game->combat.player.level, game->combat.player.xp, game->combat.player.max_xp);
    DrawTextEx(game->uiFont, statHeader, (Vector2){leftWidth + 20, 10}, 24, 1, WHITE);

    char combatStats[100];
    sprintf(combatStats, "HP: %d/%d  |  Mana: %d/%d  |  ATK: %d", game->combat.player.hp, game->combat.player.max_hp, game->combat.player.mana, game->combat.player.max_mana, game->combat.player.atk);
    DrawTextEx(game->uiFont, combatStats, (Vector2){leftWidth + 20, 40}, 20, 1, (game->combat.player.hp < 20) ? RED : GREEN);

    // --- PANNEAU GAUCHE : RESSOURCES ET CARTE ---
    DrawTextEx(game->uiFont, T("UI_RESOURCES_TITLE"), (Vector2){20, 20}, 24, 1, LIGHTGRAY);
    DrawTextEx(game->uiFont, TextFormat(T("RES_IRON"), game->clicker.inventory.fer), (Vector2){20, 60}, 20, 1, GRAY);
    DrawTextEx(game->uiFont, TextFormat(T("RES_GOLD"), game->clicker.inventory.or), (Vector2){20, 90}, 20, 1, YELLOW);
    DrawTextEx(game->uiFont, TextFormat(T("RES_CRYSTALS"), game->clicker.inventory.cristaux), (Vector2){20, 120}, 20, 1, PURPLE);
    DrawTextEx(game->uiFont, TextFormat(T("RES_HERBS"), game->clicker.inventory.herbes), (Vector2){20, 150}, 20, 1, GREEN);
    DrawTextEx(game->uiFont, TextFormat(T("RES_WOOD"), game->clicker.inventory.bois), (Vector2){20, 180}, 20, 1, BROWN);
    DrawTextEx(game->uiFont, TextFormat(T("RES_MEAT"), game->clicker.inventory.viande), (Vector2){20, 210}, 20, 1, RED);

    DrawTextEx(game->uiFont, T("MAP_TITLE"), (Vector2){20, mapStartY + 15}, 24, 1, LIGHTGRAY);

    if (game->currentState == STATE_DUNGEON && dungeon != NULL)
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
                if (dungeon->explored[y][x])
                {
                    char    tile = dungeon->map[y][x];
                    Vector2 pos  = {mapStartX + (x * cell_size), mapRenderStartY + (y * cell_size)};

                    if (x == dungeon->playerX && y == dungeon->playerY)
                    {
                        char* dirStr = "^";
                        if (dungeon->playerDir == DIR_EAST)
                            dirStr = ">";
                        else if (dungeon->playerDir == DIR_SOUTH)
                            dirStr = "v";
                        else if (dungeon->playerDir == DIR_WEST)
                            dirStr = "<";
                        DrawTextEx(game->dungeonFont, dirStr, pos, cell_size, spacing, mapPlayerColor);
                    }
                    else if (tile == '#')
                        DrawTextEx(game->dungeonFont, "#", pos, cell_size, spacing, mapWallColor);
                    else if (tile == '.')
                        DrawTextEx(game->dungeonFont, ".", pos, cell_size, spacing, mapFloorColor);
                    else if (tile == '>')
                        DrawTextEx(game->dungeonFont, ">", pos, cell_size, spacing, mapStairsColor);
                    else if (tile == 'E' || tile == 'B')
                        DrawTextEx(game->dungeonFont, "?", pos, cell_size, spacing, mapSpecialColor);
                }
            }
        }
    }

    // --- PANNEAU DROIT : COMBAT ET EQUIPEMENT ---
    int rightX = w - rightWidth + 20;

    DrawTextEx(game->uiFont, T("UI_LOG_TITLE"), (Vector2){rightX, 20}, 24, 1, LIGHTGRAY);
    for (int i = 0; i < 5; i++)
    {
        DrawTextEx(game->uiFont, game->combat.battle_log[i], (Vector2){rightX, 60 + (i * 25)}, 20, 1, (i == 0) ? WHITE : GRAY);
    }

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
            OwnedItem*    item = &game->combat.player.inventory[inv_idx];
            ItemTemplate* t    = &g_itemDB[item->template_idx];
            sprintf(eq_text, "%s: %s%s (+%d)", slot_names[i], g_isEnglish ? t->name_en : t->name_fr, GetEffectString(item->effect), item->level);
            DrawTextEx(game->uiFont, eq_text, (Vector2){rightX, equipStartY + 45 + (i * 20)}, 20, 1, WHITE);
        }
    }

    float magicStartY = equipStartY + 195;
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

    float spellStartY = magicStartY + 115;
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