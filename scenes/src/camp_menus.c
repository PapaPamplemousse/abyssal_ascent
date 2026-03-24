#include "camp_menus.h"
#include "ui.h"
#include "lang.h"
#include <stdio.h>

// Déclarations externes pour les bases de données
extern ItemTemplate   g_itemDB[];
extern SpellTemplate  g_spellDB[MAX_SPELLS_DB];
extern PotionTemplate g_potionDB[MAX_POTIONS_DB];
extern int            g_spellCount;
extern int            g_potionCount;
extern bool           g_isEnglish;

// Variables d'état des menus (anciennement dans game.c)
static int selectedForgeIdx  = -1;
static int selectedSpellIdx  = -1;
static int selectedPotionIdx = -1;

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
    int cell_h = 90;
    int cx     = startX + 20;
    int cy     = 135;

    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_HELMET], T("SLOT_HELMET"), cx + cell_w + 10, cy, cell_w, cell_h);
    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_HAND_1], T("SLOT_HAND_1"), cx, cy + cell_h + 5, cell_w, cell_h);
    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_ARMOR], T("SLOT_ARMOR"), cx + cell_w + 10, cy + cell_h + 5, cell_w, cell_h);
    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_HAND_2], T("SLOT_HAND_2"), cx + (cell_w * 2) + 20, cy + cell_h + 5, cell_w, cell_h);
    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_GLOVES], T("SLOT_GLOVES"), cx + cell_w + 10, cy + (cell_h * 2) + 10, cell_w, cell_h);
    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_LEGGINGS], T("SLOT_LEGGINGS"), cx + cell_w + 10, cy + (cell_h * 3) + 15, cell_w, cell_h);
    DrawEquipSlotGrid(game, game->combat.player.equipped[SLOT_BOOTS], T("SLOT_BOOTS"), cx + cell_w + 10, cy + (cell_h * 4) + 20, cell_w, cell_h);

    // --- ZONE DROITE : LISTE INVENTAIRE ---
    int listX = startX + visualWidth + 20;
    DrawTextEx(game->uiFont, T("UI_INV_LIST"), (Vector2){listX, 100}, 24, 1, WHITE);

    int sorted_indices[MAX_INVENTORY];
    Inventory_GetSortedIndices(&game->combat, sorted_indices);

    for (int i = 0; i < game->combat.player.inventory_count; i++)
    {
        int           inv_idx = sorted_indices[i];
        OwnedItem* item    = &game->combat.player.inventory[inv_idx];
        ItemTemplate* t       = &g_itemDB[item->template_idx];

        bool is_eq = false;
        for (int j = 0; j < MAX_SLOTS; j++) {
            if (game->combat.player.equipped[j] == inv_idx) is_eq = true;
        }

        // CORRECTION 2 : Buffer plus grand + Suppression du DoShopButton !
        char btn_text[256];
        sprintf(btn_text, "%s %s%s%s (Niv %d)", is_eq ? "[ EQUIPE ]" : "[ ]", GetRarityName(item->rarity), g_isEnglish ? t->name_en : t->name_fr, GetEffectString(item->effect), item->level);

        int startY = 150;
        Vector2 tSize = MeasureTextEx(game->uiFont, btn_text, 20, 1);
        Rectangle hitbox = {listX, startY + (i * 30), tSize.x, tSize.y}; // Aligné sur listX proprement
        bool isHovered = CheckCollisionPointRec(GetMousePosition(), hitbox);
        
        Color drawColor = GetRarityColor(item->rarity); 
        if (is_eq) drawColor = YELLOW; 
        if (isHovered) drawColor = WHITE; 

        // On dessine UNE SEULE FOIS, proprement.
        DrawTextEx(game->uiFont, btn_text, (Vector2){hitbox.x, hitbox.y}, 20, 1, drawColor);

        // La logique de clic remplace le "DoShopButton"
        if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (is_eq) {
                for (int j = 0; j < MAX_SLOTS; j++) {
                    if (game->combat.player.equipped[j] == inv_idx) {
                        game->combat.player.equipped[j] = -1;
                    }
                }
            } else {
                Inventory_Equip(&game->combat, inv_idx);
            }
            Combat_RecalculateStats(&game->combat);
        }
    }
}

void Game_RenderForge(GameContext* game, int w, int h)
{
    int startX      = (w * 0.25f) + 30;
    int centerWidth = w * 0.55f;

    DrawLine(startX, 80, startX + centerWidth, 80, DARKGRAY);

    int listWidth = centerWidth * 0.45f;
    int shopWidth = centerWidth * 0.55f;

    DrawLine(startX + listWidth, 80, startX + listWidth, h, DARKGRAY);

    // --- ZONE GAUCHE : LISTE ÉQUIPEMENT ---
    int listX = startX + 20;
    DrawTextEx(game->uiFont, T("UI_FORGE_LIST"), (Vector2){listX, 100}, 24, 1, WHITE);

    int sorted_indices[MAX_INVENTORY];
    Inventory_GetSortedIndices(&game->combat, sorted_indices);

    for (int i = 0; i < game->combat.player.inventory_count; i++)
    {
        int           inv_idx = sorted_indices[i];
        OwnedItem* item    = &game->combat.player.inventory[inv_idx];
        ItemTemplate* t       = &g_itemDB[item->template_idx];

        bool is_eq = false;
        for (int j = 0; j < MAX_SLOTS; j++) {
            if (game->combat.player.equipped[j] == inv_idx) is_eq = true;
        }

        char itemText[256];
        sprintf(itemText, "%s %s%s%s (Niv %d)", is_eq ? "[E]" : "[ ]", GetRarityName(item->rarity), g_isEnglish ? t->name_en : t->name_fr, GetEffectString(item->effect), item->level);

        // CORRECTION 3 : Suppression de DoShopButton pour laisser les couleurs s'afficher !
        int startY = 150;
        Vector2 tSize = MeasureTextEx(game->uiFont, itemText, 20, 1);
        Rectangle hitbox = {listX, startY + (i * 30), tSize.x, tSize.y};
        bool isHovered = CheckCollisionPointRec(GetMousePosition(), hitbox);

        Color drawColor = GetRarityColor(item->rarity);
        if (selectedForgeIdx == inv_idx) drawColor = YELLOW; // L'objet sélectionné clignote en jaune
        else if (isHovered) drawColor = WHITE;

        DrawTextEx(game->uiFont, itemText, (Vector2){hitbox.x, hitbox.y}, 20, 1, drawColor);

        if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            selectedForgeIdx = inv_idx;
        }
    }

    // --- ZONE DROITE : ATELIER ---
    int shopX = startX + listWidth + 20;
    DrawTextEx(game->uiFont, T("UI_FORGE_CENTER"), (Vector2){shopX, 100}, 24, 1, WHITE);

    if (selectedForgeIdx == -1)
    {
        DrawTextCentered(game->uiFont, T("UI_EQUIP_TITLE"), shopX + (shopWidth / 2) - 20, h / 2, 20, 1, DARKGRAY);
    }
    else
    {
        OwnedItem* item = &game->combat.player.inventory[selectedForgeIdx];
        ItemTemplate* t    = &g_itemDB[item->template_idx];

        int line_height = 20;
        int asciiStartY = 150;
        for (int i = 0; i < t->ascii_line_count; i++)
        {
            Vector2 tSize = MeasureTextEx(game->dungeonFont, t->ascii[i], 20, 1);
            DrawTextEx(game->dungeonFont, t->ascii[i], (Vector2){shopX + (shopWidth / 2) - (tSize.x / 2) - 20, asciiStartY + (i * line_height)}, 20, 1, GetRarityColor(item->rarity));
        }

        int  infoY = asciiStartY + (t->ascii_line_count * line_height) + 30;
        char statsText[256];
        sprintf(statsText, "%s (Niv %d -> %d)", g_isEnglish ? t->name_en : t->name_fr, item->level, item->level + 1);
        DrawTextEx(game->uiFont, statsText, (Vector2){shopX, infoY}, 20, 1, WHITE);

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

        int costY = infoY + 70;
        DrawTextEx(game->uiFont, "[ COUTS AMELIORATION ]", (Vector2){shopX, costY}, 20, 1, LIGHTGRAY);

        float r_mult = 1.0f;
        if (item->rarity == 1) r_mult = 1.2f;
        else if (item->rarity == 2) r_mult = 1.5f;
        else if (item->rarity == 3) r_mult = 2.0f;

        int cur_cost_fer = (int)((t->cost_fer_base + (item->level * t->cost_fer_inc)) * r_mult);
        int cur_cost_bois = (int)((t->cost_bois_base + (item->level * t->cost_bois_inc)) * r_mult);
        bool canAfford     = true;

        int costLineY = costY + 30;
        if (cur_cost_fer > 0)
        {
            bool hasFer = game->clicker.inventory.fer >= cur_cost_fer;
            if (!hasFer) canAfford = false;
            DrawTextEx(game->uiFont, TextFormat("Fer: %d", cur_cost_fer), (Vector2){shopX, costLineY}, 20, 1, hasFer ? GRAY : RED);
            costLineY += 25;
        }
        if (cur_cost_bois > 0)
        {
            bool hasBois = game->clicker.inventory.bois >= cur_cost_bois;
            if (!hasBois) canAfford = false;
            DrawTextEx(game->uiFont, TextFormat("Bois: %d", cur_cost_bois), (Vector2){shopX, costLineY}, 20, 1, hasBois ? BROWN : RED);
            costLineY += 25;
        }

        Rectangle upgradeRect  = {shopX + (shopWidth / 2) - 100, costLineY + 30, 200, 50};
        bool      hoverUpgrade = CheckCollisionPointRec(GetMousePosition(), upgradeRect);

        if (canAfford)
        {
            DrawRectangleRec(upgradeRect, hoverUpgrade ? WHITE : GREEN);
            DrawTextCentered(game->uiFont, T("BTN_UPGRADE"), upgradeRect.x + 100, upgradeRect.y + 15, 24, 1, BLACK);

            if (hoverUpgrade && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                game->clicker.inventory.fer -= cur_cost_fer;
                game->clicker.inventory.bois -= cur_cost_bois;
                item->level++;
                Combat_RecalculateStats(&game->combat);
                Combat_AddLog(&game->combat, T("LOG_UPGRADE_SUCCESS"));
            }
        }
        else
        {
            DrawRectangleRec(upgradeRect, DARKGRAY);
            DrawTextCentered(game->uiFont, T("BTN_UPGRADE"), upgradeRect.x + 100, upgradeRect.y + 15, 24, 1, GRAY);
        }
    }
}

void Game_RenderArchiforge(GameContext* game, int w, int h)
{
    (void)h;
    int startX      = (w * 0.25f) + 30;
    int centerWidth = w * 0.55f;
    int listWidth   = centerWidth * 0.45f;
    int shopX       = startX + listWidth + 20;

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

    if (selectedSpellIdx != -1)
    {
        SpellTemplate* t        = &g_spellDB[selectedSpellIdx];
        bool           unlocked = game->combat.player.spell_unlocked[selectedSpellIdx];

        DrawTextEx(game->uiFont, g_isEnglish ? t->name_en : t->name_fr, (Vector2){shopX, 150}, 30, 1, WHITE);

        if (!unlocked)
        {
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
            int  lvl      = game->combat.player.spell_level[selectedSpellIdx];
            int  upg_cost = t->upg_gold_base + (lvl * t->upg_gold_inc);
            bool canUpg   = (game->clicker.inventory.or >= upg_cost && lvl < 10);

            if (DoShopButton(game->uiFont, TextFormat("[ AMELIORER (-%d Or) ]", upg_cost), shopX, 250, 24, canUpg))
            {
                game->clicker.inventory.or -= upg_cost;
                game->combat.player.spell_level[selectedSpellIdx]++;
            }

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

void Game_RenderAlchemist(GameContext* game, int w, int h)
{
    int startX      = (w * 0.25f) + 30;
    int centerWidth = w * 0.55f;
    int listWidth   = centerWidth * 0.45f;
    int shopX       = startX + listWidth + 20;

    DrawLine(startX, 80, startX + centerWidth, 80, DARKGRAY);
    DrawLine(startX + listWidth, 80, startX + listWidth, h, DARKGRAY);

    DrawTextEx(game->uiFont, "=== L'ALCHIMISTE ===", (Vector2){startX + 20, 100}, 24, 1, PURPLE);

    for (int i = 0; i < g_potionCount; i++)
    {
        PotionTemplate* t        = &g_potionDB[i];
        bool            unlocked = game->combat.player.potion_unlocked[i];
        char            text[128];

        if (unlocked)
        {
            sprintf(text, "%s (Niv %d) x%d", g_isEnglish ? t->name_en : t->name_fr, game->combat.player.potion_level[i], game->combat.player.potion_qty[i]);
        }
        else
        {
            sprintf(text, "[LOCKED] %s", g_isEnglish ? t->name_en : t->name_fr);
        }

        if (DoShopButton(game->uiFont, text, startX + 20, 150 + (i * 30), 20, true))
        {
            selectedPotionIdx = i;
        }
    }

    if (selectedPotionIdx != -1)
    {
        PotionTemplate* t        = &g_potionDB[selectedPotionIdx];
        bool            unlocked = game->combat.player.potion_unlocked[selectedPotionIdx];

        DrawTextEx(game->uiFont, g_isEnglish ? t->name_en : t->name_fr, (Vector2){shopX, 150}, 30, 1, WHITE);

        if (!unlocked)
        {
            char costText[64];
            sprintf(costText, "Cout: %d Or", t->learn_gold);
            DrawTextEx(game->uiFont, costText, (Vector2){shopX, 200}, 20, 1, GRAY);

            bool canAfford = (game->clicker.inventory.or >= t->learn_gold);
            if (DoShopButton(game->uiFont, "[ APPRENDRE ]", shopX, 250, 24, canAfford))
            {
                game->clicker.inventory.or -= t->learn_gold;
                game->combat.player.potion_unlocked[selectedPotionIdx] = true;
            }
        }
        else
        {
            int lvl        = game->combat.player.potion_level[selectedPotionIdx];
            int upg_cost   = t->upg_gold_base + (lvl * t->upg_gold_inc);
            int craft_cost = t->craft_herbs_base + (lvl * t->craft_herbs_inc);

            bool canUpg = (game->clicker.inventory.or >= upg_cost && lvl < 10);
            if (DoShopButton(game->uiFont, TextFormat("[ AMELIORER (-%d Or) ]", upg_cost), shopX, 200, 20, canUpg))
            {
                game->clicker.inventory.or -= upg_cost;
                game->combat.player.potion_level[selectedPotionIdx]++;
            }

            bool canCraft = (game->clicker.inventory.herbes >= craft_cost);
            if (DoShopButton(game->uiFont, TextFormat("[ CRAFTER (-%d Herbes) ]", craft_cost), shopX, 250, 24, canCraft))
            {
                game->clicker.inventory.herbes -= craft_cost;
                game->combat.player.potion_qty[selectedPotionIdx]++;
            }

            DrawTextEx(game->uiFont, "Equiper dans le slot (Touches 1, 2, 3):", (Vector2){shopX, 320}, 20, 1, LIGHTGRAY);
            for (int slot = 0; slot < 3; slot++)
            {
                bool        is_eq    = (game->combat.player.equipped_potions[slot] == selectedPotionIdx);
                const char* btnLabel = is_eq ? TextFormat("[E] Slot %d", slot + 1) : TextFormat("[ ] Slot %d", slot + 1);

                if (DoShopButton(game->uiFont, btnLabel, shopX + (slot * 110), 360, 20, true))
                {
                    if (is_eq)
                    {
                        game->combat.player.equipped_potions[slot] = -1;
                    }
                    else
                    {
                        game->combat.player.equipped_potions[slot] = selectedPotionIdx;
                    }
                }
            }
        }
    }
}