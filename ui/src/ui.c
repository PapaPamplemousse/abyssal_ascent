#include "ui.h"
#include "lang.h"
#include <stdio.h>
#include <string.h>
#include "audio_manager.h"
#include "combat.h"

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

Color GetRarityColor(int rarity) {
    if (rarity == 1) return SKYBLUE;    // Rare
    if (rarity == 2) return PURPLE;     // Épique
    if (rarity == 3) return ORANGE;     // Légendaire
    return WHITE;                       // Commun
}

const char* GetRarityName(int rarity) {
    if (rarity == 1) return " [Rare]";
    if (rarity == 2) return " [Epique]";
    if (rarity == 3) return " [Lgd]";
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

    bool isClicked = isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && canAfford;

    if (isClicked)
    {
        Audio_PlaySFX(SFX_CLICK_BUY);
    }

    return isClicked;
}

void DrawEquipSlotGrid(GameContext* game, int inv_idx, const char* slot_label, int x, int y, int width, int height)
{
    Color borderColor = DARKGRAY;

    DrawRectangleLinesEx((Rectangle){x, y, width, height}, 2, borderColor);
    DrawTextEx(game->uiFont, slot_label, (Vector2){x + 5, y + 5}, 20, 1, GRAY);

    if (inv_idx == -1)
    {
        Vector2 tSize = MeasureTextEx(game->uiFont, "[ Vide ]", 20, 1);
        DrawTextEx(game->uiFont, "[ Vide ]", (Vector2){x + (width / 2) - (tSize.x / 2), y + (height / 2) - (tSize.y / 2)}, 20, 1, DARKGRAY);
    }
    else
    {
        OwnedItem* item = &game->combat.player.inventory[inv_idx];
        ItemTemplate* t    = &g_itemDB[item->template_idx];

        //  DESSIN DU SPRITE DANS LA CASE 
        if (t->sprite.id != 0) 
        {
            // On calcule l'échelle pour que l'image rentre bien dans la case (qui fait 'height' pixels de haut)
            // On vise une hauteur de 50 pixels pour l'image
            float scale = 50.0f / (float)t->sprite.height; 
            
            float scaledWidth = t->sprite.width * scale;
            float scaledHeight = t->sprite.height * scale;

            // On centre l'image dans la case, avec un petit décalage vers le bas (+10) pour le texte
            int imgX = x + (width / 2) - (scaledWidth / 2);
            int imgY = y + (height / 2) - (scaledHeight / 2) + 10;

            // On dessine l'image en appliquant la couleur de sa rareté !!
            DrawTextureEx(t->sprite, (Vector2){(float)imgX, (float)imgY}, 0.0f, scale, GetRarityColor(item->rarity));
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

    // --- PREPARATION DES TEXTES FORMATES ---
    char fmt_fer[32], fmt_or[32], fmt_crys[32], fmt_herb[32], fmt_bois[32], fmt_viande[32];

    FormatNumber(game->clicker.inventory.fer, fmt_fer);
    FormatNumber(game->clicker.inventory.or, fmt_or);
    FormatNumber(game->clicker.inventory.cristaux, fmt_crys);
    FormatNumber(game->clicker.inventory.herbes, fmt_herb);
    FormatNumber(game->clicker.inventory.bois, fmt_bois);
    FormatNumber(game->clicker.inventory.viande, fmt_viande);

    // --- PANNEAU GAUCHE : RESSOURCES ET CARTE ---
    DrawTextEx(game->uiFont, T("UI_RESOURCES_TITLE"), (Vector2){20, 20}, 24, 1, LIGHTGRAY);
    DrawTextEx(game->uiFont, TextFormat(T("RES_IRON"), fmt_fer), (Vector2){20, 60}, 20, 1, GRAY);
    DrawTextEx(game->uiFont, TextFormat(T("RES_GOLD"), fmt_or), (Vector2){20, 90}, 20, 1, YELLOW);
    DrawTextEx(game->uiFont, TextFormat(T("RES_CRYSTALS"), fmt_crys), (Vector2){20, 120}, 20, 1, PURPLE);
    DrawTextEx(game->uiFont, TextFormat(T("RES_HERBS"), fmt_herb), (Vector2){20, 150}, 20, 1, GREEN);
    DrawTextEx(game->uiFont, TextFormat(T("RES_WOOD"), fmt_bois), (Vector2){20, 180}, 20, 1, BROWN);
    DrawTextEx(game->uiFont, TextFormat(T("RES_MEAT"), fmt_viande), (Vector2){20, 210}, 20, 1, RED);

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
                        if (dungeon->playerDir == DIR_EAST) dirStr = ">";
                        else if (dungeon->playerDir == DIR_SOUTH) dirStr = "v";
                        else if (dungeon->playerDir == DIR_WEST) dirStr = "<";
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

    // --- PANNEAU DROIT : COMBAT ET EQUIPEMENT ---
    int rightX = w - rightWidth + 20;

    DrawTextEx(game->uiFont, T("UI_LOG_TITLE"), (Vector2){rightX, 20}, 24, 1, LIGHTGRAY);
    for (int i = 0; i < 5; i++)
    {
        DrawTextEx(game->uiFont, game->combat.battle_log[i], (Vector2){rightX, 60 + (i * 25)}, 20, 1, (i == 0) ? WHITE : GRAY);
    }

    // --- JOURNAL DES QUETES ---
    Quest_CheckInitial(&game->combat); // S'assure qu'on a nos 3 quêtes de base !
    
    float questStartY = 200; 
    DrawLine(w - rightWidth, questStartY, w, questStartY, uiBorder);
    DrawTextEx(game->uiFont, "[ QUETES ACTIVES ]", (Vector2){rightX, questStartY + 10}, 24, 1, LIGHTGRAY);

    int active_count = 0;
    for (int i = 0; i < MAX_ACTIVE_QUESTS; i++) 
    {
        if (game->combat.player.active_quests[i].is_active) 
        {
            Quest* q = &game->combat.player.active_quests[i];
            
            Color qColor = q->is_completed ? GREEN : WHITE;
            DrawTextEx(game->uiFont, q->title, (Vector2){rightX, questStartY + 45 + (active_count * 45)}, 20, 1, qColor);
            
            char progress[128];
            if (q->is_completed) {
                sprintf(progress, "-> Termine ! (Cliquez pour valider)");
                
                // --- LOGIQUE DE CLIC POUR RÉCOMPENSE ---
                Rectangle qRect = { rightX, questStartY + 40 + (active_count * 45), 300, 45 };
                if (CheckCollisionPointRec(GetMousePosition(), qRect)) {
                    DrawRectangleLinesEx(qRect, 1, YELLOW); // Surbrillance au survol
                    
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        Audio_PlaySFX(SFX_CLICK_BUY);
                        // On utilise highest_floor_curr si on est dans le donjon, sinon fallback
                        int current_floor = (dungeon != NULL) ? dungeon->highest_floor_curr : game->combat.player.level;
                        Quest_ClaimReward(&game->combat, i, current_floor);
                    }
                }
            } else if (q->type == QUEST_STORY) {
                sprintf(progress, "-> %s", q->desc);
            } else {
                sprintf(progress, "-> %s : %d / %d", q->desc, q->current_val, q->target_val);
            }
            
            DrawTextEx(game->uiFont, progress, (Vector2){rightX + 10, questStartY + 65 + (active_count * 45)}, 16, 1, GRAY);
            active_count++;
        }
    }
    
    if (active_count == 0) {
        DrawTextEx(game->uiFont, "Aucune quete en cours...", (Vector2){rightX, questStartY + 50}, 18, 1, DARKGRAY);
    }

    // --- MISE À JOUR DE LA POSITION DE L'ÉQUIPEMENT ---
    // On descend un peu le point de départ de l'équipement pour laisser la place aux quêtes !
    float equipStartY = h * 0.48f; // (Avant: 0.35f)
    DrawLine(w - rightWidth, equipStartY, w, equipStartY, uiBorder);
    DrawTextEx(game->uiFont, T("UI_EQUIP_VISUAL"), (Vector2){rightX, equipStartY + 10}, 24, 1, LIGHTGRAY);

    const char* slot_names[MAX_SLOTS] = {T("SLOT_HELMET"), T("SLOT_ARMOR"), T("SLOT_GLOVES"), T("SLOT_LEGGINGS"), T("SLOT_BOOTS"), T("SLOT_HAND_1"), T("SLOT_HAND_2")};

    for (int i = 0; i < MAX_SLOTS; i++)
    {
        int  inv_idx = game->combat.player.equipped[i];
        
        // CORRECTION 1 : Tableau beaucoup plus grand pour éviter le crash (Buffer Overflow) !
        char eq_text[256]; 
        
        if (inv_idx == -1)
        {
            sprintf(eq_text, "%s: [ Vide ]", slot_names[i]);
            DrawTextEx(game->uiFont, eq_text, (Vector2){rightX, equipStartY + 45 + (i * 20)}, 20, 1, DARKGRAY);
        }
        else
        {
            OwnedItem* item = &game->combat.player.inventory[inv_idx];
            ItemTemplate* t = &g_itemDB[item->template_idx];
            sprintf(eq_text, "%s: %s%s%s (+%d)", slot_names[i], GetRarityName(item->rarity), g_isEnglish ? t->name_en : t->name_fr, GetEffectString(item->effect), item->level);
            DrawTextEx(game->uiFont, eq_text, (Vector2){rightX, equipStartY + 45 + (i * 20)}, 20, 1, GetRarityColor(item->rarity));
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
            char pText[64];
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
            char sText[64];
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

// Transforme 1500000 en "1.50M"
void FormatNumber(long long value, char* buffer) 
{
    if (value >= 1000000000000000LL) {
        sprintf(buffer, "%.2fQ", (float)value / 1000000000000000.0f); // Quadrillions
    } else if (value >= 1000000000000LL) {
        sprintf(buffer, "%.2fT", (float)value / 1000000000000.0f);    // Trillions
    } else if (value >= 1000000000LL) {
        sprintf(buffer, "%.2fB", (float)value / 1000000000.0f);       // Billions (Milliards)
    } else if (value >= 1000000LL) {
        sprintf(buffer, "%.2fM", (float)value / 1000000.0f);          // Millions
    } else if (value >= 1000LL) {
        sprintf(buffer, "%.1fK", (float)value / 1000.0f);             // Milliers
    } else {
        sprintf(buffer, "%lld", value);                               // Normal (< 1000)
    }
}