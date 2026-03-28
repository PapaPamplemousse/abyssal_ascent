#include "camp_menus.h"
#include "ui.h"
#include "lang.h"
#include <stdio.h>
#include <math.h>
#include "ui.h"

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

bool  g_camp_fire_lit = false;
float g_camp_fire_timer = 0.0f;

static long long compute_price(int base, int inc, int level);


void Game_RenderInventory(GameContext* game, int w, int h)
{
    int startX      = (w * 0.25f) + 30;
    int centerWidth = w * 0.55f;

    DrawLine(startX, 80, startX + centerWidth, 80, DARKGRAY);

    int visualWidth = centerWidth * 0.45f;

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

        int startY = 150;
        Vector2 tSize = MeasureTextEx(game->uiFont, itemText, 20, 1);
        Rectangle hitbox = {listX, startY + (i * 30), tSize.x, tSize.y};
        bool isHovered = CheckCollisionPointRec(GetMousePosition(), hitbox);

        Color drawColor = GetRarityColor(item->rarity);
        if (selectedForgeIdx == inv_idx) drawColor = YELLOW; 
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

        // --- AFFICHAGE DU SPRITE DANS LA FORGE ---
        float scale = 4.0f; 
        int scaledWidth = t->sprite.width * scale;
        int scaledHeight = t->sprite.height * scale;
        
        int imgX = shopX + (shopWidth / 2) - (scaledWidth / 2) - 20;
        int imgY = 150;

        if (t->sprite.id != 0) {
            DrawTextureEx(t->sprite, (Vector2){(float)imgX, (float)imgY}, 0.0f, scale, GetRarityColor(item->rarity));
        }

        int  infoY = imgY + scaledHeight + 30;
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

        // --- NOUVEAU : Calcul avec long long ! ---
        long long cur_cost_fer = compute_price(t->cost_fer_base, t->cost_fer_inc, item->level);
        long long cur_cost_bois = compute_price(t->cost_bois_base, t->cost_bois_inc, item->level);
         
        cur_cost_fer = (long long)(cur_cost_fer * r_mult);
        cur_cost_bois = (long long)(cur_cost_bois * r_mult);
        
        bool canAfford     = true;
        int costLineY = costY + 30;

        if (cur_cost_fer > 0)
        {
            bool hasFer = game->clicker.inventory.fer >= cur_cost_fer;
            if (!hasFer) canAfford = false;
            
            // Formatage du prix
            char fmt_fer[32];
            FormatNumber(cur_cost_fer, fmt_fer);
            DrawTextEx(game->uiFont, TextFormat("Fer: %s", fmt_fer), (Vector2){shopX, costLineY}, 20, 1, hasFer ? GRAY : RED);
            costLineY += 25;
        }
        if (cur_cost_bois > 0)
        {
            bool hasBois = game->clicker.inventory.bois >= cur_cost_bois;
            if (!hasBois) canAfford = false;
            
            // Formatage du prix
            char fmt_bois[32];
            FormatNumber(cur_cost_bois, fmt_bois);
            DrawTextEx(game->uiFont, TextFormat("Bois: %s", fmt_bois), (Vector2){shopX, costLineY}, 20, 1, hasBois ? BROWN : RED);
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
            // Formatage des coûts de déblocage
            char fmt_gold[32], fmt_crystal[32];
            FormatNumber(t->learn_gold, fmt_gold);
            FormatNumber(t->learn_crystal, fmt_crystal);

            char costText[128];
            sprintf(costText, "Cout: %s Or, %s Cristal", fmt_gold, fmt_crystal);
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
            
            // --- NOUVEAU : Formatage du prix d'amélioration ! ---
            long long upg_cost = compute_price(t->upg_gold_base, t->upg_gold_inc, lvl);
            bool canUpg   = (game->clicker.inventory.or >= upg_cost && lvl < 10);

            char fmt_upg[32];
            FormatNumber(upg_cost, fmt_upg);

            if (DoShopButton(game->uiFont, TextFormat("[ AMELIORER (-%s Or) ]", fmt_upg), shopX, 250, 24, canUpg))
            {
                game->clicker.inventory.or -= upg_cost;
                game->combat.player.spell_level[selectedSpellIdx]++;
            }

            DrawTextEx(game->uiFont, "Equiper dans le slot :", (Vector2){shopX, 320}, 20, 1, LIGHTGRAY);
            bool canPrep = (game->clicker.inventory.cristaux >= t->prep_crystal);
            
            // Formatage du coût d'équipement
            char fmt_prep[32];
            FormatNumber(t->prep_crystal, fmt_prep);

            for (int slot = 0; slot < 3; slot++)
            {
                if (DoShopButton(game->uiFont, TextFormat("[ Slot %d (-%s Cristal) ]", slot + 1, fmt_prep), shopX + (slot * 150), 360, 20, canPrep))
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
            int lvl = game->combat.player.potion_level[selectedPotionIdx];
            
            // 1. On récupère les prix géants
            long long upg_cost   = compute_price(t->upg_gold_base, t->upg_gold_inc, lvl);
            long long craft_cost = compute_price(t->craft_herbs_base, t->craft_herbs_inc, lvl);

            int max_potions = 10;
            bool isFull = (game->combat.player.potion_qty[selectedPotionIdx] >= max_potions);
            bool canCraft = (game->clicker.inventory.herbes >= craft_cost) && !isFull;
            bool canUpg = (game->clicker.inventory.or >= upg_cost && lvl < 10);
            
            // 2. On formate le prix en "K", "M", etc.
            char formatted_upg_cost[32];
            FormatNumber(upg_cost, formatted_upg_cost);

            // 3. On affiche avec TextFormat et le %s (car c'est maintenant du texte !)
            if (DoShopButton(game->uiFont, TextFormat("[ AMELIORER (-%s Or) ]", formatted_upg_cost), shopX, 200, 20, canUpg))
            {
                game->clicker.inventory.or -= upg_cost;
                game->combat.player.potion_level[selectedPotionIdx]++;
            }
                        
            char formatted_craft_cost[32];
            FormatNumber(craft_cost, formatted_craft_cost);

            if (isFull) 
            {
                // Message rouge si le sac est plein
                DrawTextCentered(game->uiFont, "[ SACOCHE PLEINE (MAX 10) ]", shopX + 150, 250, 20, 1, RED);
            }
            else 
            {
                if (DoShopButton(game->uiFont, TextFormat("[ CRAFTER (-%s Herbes) ]", formatted_craft_cost), shopX, 250, 24, canCraft))
                {
                    game->clicker.inventory.herbes -= craft_cost;
                    game->combat.player.potion_qty[selectedPotionIdx]++;
                }
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


void Game_RenderAltar(GameContext* game, int w, int h)
{
    int centerX = w / 2;
    int centerY = h / 2;

    // =========================================================
    // PARAMÈTRES DE MISE EN PAGE (Pour ajuster facilement)
    // =========================================================
    
    // 1. Taille de l'image (Près de 80% de la hauteur de l'écran !)
    float pentagramSize = (float)h * 0.78f; 
    
    // 2. Position verticale du centre de l'image (Légèrement décalé vers le bas pour le titre)
    int altarY = centerY + 30; 
    
    // 3. Écartement des descriptions (Haut/Bas) par rapport au centre
    // On augmente cet écart pour faire "respirer" l'image géante
    int offset_y = 190; 

    // 4. Écartement horizontal (inchangé, pour les bords de l'écran)
    int offset_x = 350; 


    // =========================================================
    // 1. DESSIN DU TITRE ET ÂMES (Remontés et plus imposants)
    // =========================================================
    
    // Titre tout en haut
    DrawTextCentered(game->uiFont, "[ AUTEL DES ANCIENS ]", centerX, 30, 40, 1, PURPLE);
    
    // Âmes juste en dessous (Y=70 au lieu de 90)
    char soulText[64];
    sprintf(soulText, "Ames de Boss disponibles : %d", game->combat.player.boss_souls);
    DrawTextCentered(game->uiFont, soulText, centerX, 70, 24, 1, WHITE);


    // =========================================================
    // 2. DESSIN DU PENTAGRAMME EN PNG (GIGANTESQUE)
    // =========================================================

    if (game->tex_pentagram.id != 0) 
    {
        // Calcul du multiplicateur pour atteindre la taille cible
        float scale = pentagramSize / (float)game->tex_pentagram.height;
        
        float scaledWidth = game->tex_pentagram.width * scale;
        float scaledHeight = game->tex_pentagram.height * scale;

        // Rectangle source (toute l'image)
        Rectangle sourceRec = { 0.0f, 0.0f, (float)game->tex_pentagram.width, (float)game->tex_pentagram.height };
        
        // Rectangle de destination (centré sur altarY)
        Rectangle destRec = { (float)centerX, (float)altarY, scaledWidth, scaledHeight };
        
        // Origine au centre de l'image pour un positionnement facile
        Vector2 origin = { scaledWidth / 2.0f, scaledHeight / 2.0f };

        float rotation = 0.0f; // Toujours fixe ^^

        // Teinte violette pour l'ambiance temple
        DrawTexturePro(game->tex_pentagram, sourceRec, destRec, origin, rotation, DARKPURPLE);
    }

    // =========================================================
    // 3. BOUTONS DES PASSIFS (Placés sur les bords de l'image géante)
    // =========================================================
    
    // Calcul des hauteurs basées sur l'écartement offset_y
    int top_y = altarY - offset_y;
    int bottom_y = altarY + offset_y;

    // --- LIGNE DU HAUT ---

    // Position 1 : Haut Gauche (Vitalité Ancestrale)
    int p1_x = centerX - offset_x;
    int p1_y = top_y;
    
    char hpTxt[128];
    sprintf(hpTxt, "Vitalite Ancestrale\nNiv %d : +%d%% HP Max", game->combat.player.passive_hp_level, game->combat.player.passive_hp_level * 10);
    DrawTextCentered(game->uiFont, hpTxt, p1_x, p1_y, 20, 1, GREEN);
    if (DoShopButton(game->uiFont, "[ Ameliorer (1 Ame) ]", p1_x - 100, p1_y + 45, 20, game->combat.player.boss_souls > 0)) {
        game->combat.player.boss_souls--;
        game->combat.player.passive_hp_level++;
        Combat_RecalculateStats(&game->combat);
    }

    // Position 2 : Haut Droite (Force Titanesque)
    int p2_x = centerX + offset_x;
    int p2_y = top_y;
    
    char atkTxt[128];
    sprintf(atkTxt, "Force Titanesque\nNiv %d : +%d%% ATK", game->combat.player.passive_atk_level, game->combat.player.passive_atk_level * 10);
    DrawTextCentered(game->uiFont, atkTxt, p2_x, p2_y, 20, 1, RED);
    if (DoShopButton(game->uiFont, "[ Ameliorer (1 Ame) ]", p2_x - 100, p2_y + 45, 20, game->combat.player.boss_souls > 0)) {
        game->combat.player.boss_souls--;
        game->combat.player.passive_atk_level++;
        Combat_RecalculateStats(&game->combat);
    }

    // --- LIGNE DU BAS ---

    // Position 3 : Bas Gauche (Puits Cosmique)
    int p3_x = centerX - offset_x;
    int p3_y = bottom_y;
    
    char manaTxt[128];
    sprintf(manaTxt, "Puits Cosmique\nNiv %d : +%d%% Mana", game->combat.player.passive_mana_level, game->combat.player.passive_mana_level * 10);
    DrawTextCentered(game->uiFont, manaTxt, p3_x, p3_y, 20, 1, SKYBLUE);
    if (DoShopButton(game->uiFont, "[ Ameliorer (1 Ame) ]", p3_x - 100, p3_y + 45, 20, game->combat.player.boss_souls > 0)) {
        game->combat.player.boss_souls--;
        game->combat.player.passive_mana_level++;
        Combat_RecalculateStats(&game->combat);
    }

    // Position 4 : Bas Droite (Aura de Fortune)
    int p4_x = centerX + offset_x;
    int p4_y = bottom_y;
    
    char lootTxt[128];
    sprintf(lootTxt, "Aura de Fortune\nNiv %d : +%d%% Loot", game->combat.player.passive_loot_level, game->combat.player.passive_loot_level * 2);
    DrawTextCentered(game->uiFont, lootTxt, p4_x, p4_y, 20, 1, YELLOW);
    if (DoShopButton(game->uiFont, "[ Ameliorer (1 Ame) ]", p4_x - 100, p4_y + 45, 20, game->combat.player.boss_souls > 0)) {
        game->combat.player.boss_souls--;
        game->combat.player.passive_loot_level++;
    }
}

void Game_RenderCamp(GameContext* game, int w, int h)
{
    int cx = (w * 0.2f) + ((w * 0.55f) / 2);
    int cy = h / 2;

    // --- TITRE ET SOUS-TITRE (Plus d'ambiance) ---
    DrawTextCentered(game->uiFont, T("CAMP_TITLE"), cx, 110, 40, 1, GREEN);
    DrawTextCentered(game->uiFont, "Un bref repit dans les tenebres...", cx, 160, 20, 1, GRAY);

    // =========================================================
    // 1. DESSIN DU FEU DE CAMP (ANIMÉ + EFFETS DE LUMIÈRE)
    // =========================================================
    int fireY = cy - 60; 
    Texture2D currentTex;

    // L'ombre au sol (dessinée EN DESSOUS du feu)
    DrawEllipse(cx, fireY + 80, 100, 20, (Color){10, 10, 10, 180});

    if (g_camp_fire_lit) {
        int frameIndex = (int)(GetTime() * 8.0f) % 4; 
        currentTex = game->tex_fire_lit[frameIndex];

        // --- HALO LUMINEUX QUI PALPITE ---
        // Utilisation d'un sinus pour faire "respirer" la lumière
        float pulse = sinf(GetTime() * 5.0f) * 10.0f;
        
        // Un grand halo orange léger
        DrawCircleGradient(cx, fireY + 30, 250.0f + pulse, (Color){255, 100, 0, 30}, BLANK);
        // Un petit halo jaune plus intense au centre
        DrawCircleGradient(cx, fireY + 30, 120.0f + (pulse * 0.5f), (Color){255, 200, 0, 50}, BLANK);

        // --- BRAISES VOLANTES ---
        for (int i = 0; i < 8; i++) {
            float pTime = GetTime() + (i * 0.7f); // Décalage temporel pour chaque particule
            float pY = fireY + 60 - fmodf(pTime * 50.0f, 180.0f); // Monte vers le haut
            float pX = cx + sinf(pTime * 3.0f + i) * 40.0f;       // Vole en zigzag
            float pAlpha = 1.0f - (fmodf(pTime * 50.0f, 180.0f) / 180.0f); // Disparaît en montant
            
            DrawRectangle(pX, pY, 4, 4, Fade(YELLOW, pAlpha));
        }

    } else {
        currentTex = game->tex_fire_unlit;
    }

    // Le dessin du sprite par-dessus la lumière
    if (currentTex.id != 0) {
        float scale = 200.0f / (float)currentTex.height;
        float scaledWidth = currentTex.width * scale;
        float scaledHeight = currentTex.height * scale;

        int imgX = cx - (scaledWidth / 2);
        int imgY = fireY - (scaledHeight / 2);

        Color tint = g_camp_fire_lit ? WHITE : GRAY;
        DrawTextureEx(currentTex, (Vector2){(float)imgX, (float)imgY}, 0.0f, scale, tint);
    }

    // =========================================================
    // 2. GESTION DE LA SURVIE (Boutons au centre)
    // =========================================================
    int btnY = fireY + 130; 

    char fireBtn[64];
    if (g_camp_fire_lit) sprintf(fireBtn, "[ ETEINDRE LE FEU ]");
    else sprintf(fireBtn, "[ ALLUMER LE FEU (-25 Bois/sec) ]");

    bool can_light_fire = g_camp_fire_lit || (game->clicker.inventory.bois >= 25);
    if (DoShopButton(game->uiFont, fireBtn, cx - 180, btnY, 20, can_light_fire)) {
        g_camp_fire_lit = !g_camp_fire_lit;
        g_camp_fire_timer = 0.0f;
        game->combat.player.is_freezing = !g_camp_fire_lit;
        Combat_RecalculateStats(&game->combat);
    }

    int missing_hp = game->combat.player.max_hp - game->combat.player.hp;
    int missing_mana = game->combat.player.max_mana - game->combat.player.mana;
    int total_missing = missing_hp + missing_mana;
    int meat_cost = total_missing * 5;

    if (total_missing > 0) {
        if (g_camp_fire_lit) {
            char healBtn[128];
            int affordable_heal = game->clicker.inventory.viande / 5;
            bool can_heal = affordable_heal > 0;

            if (meat_cost > game->clicker.inventory.viande && can_heal) {
                sprintf(healBtn, "[ CUIRE VIANDE : SOIN PARTIEL (-%d Viande) ]", affordable_heal * 5);
            } else if (!can_heal) {
                 sprintf(healBtn, "[ CUIRE VIANDE (Pas assez de viande) ]");
            } else {
                sprintf(healBtn, "[ CUIRE VIANDE : SOIN MAX (-%d Viande) ]", meat_cost);
            }

            if (DoShopButton(game->uiFont, healBtn, cx - 210, btnY + 40, 20, can_heal)) {
                int points_to_heal = (meat_cost > game->clicker.inventory.viande) ? affordable_heal : total_missing;
                game->clicker.inventory.viande -= points_to_heal * 5;

                int hp_to_heal = (missing_hp < points_to_heal) ? missing_hp : points_to_heal;
                game->combat.player.hp += hp_to_heal;
                points_to_heal -= hp_to_heal;
                if (points_to_heal > 0) game->combat.player.mana += points_to_heal;
            }
        } else {
            DrawTextCentered(game->uiFont, "(Le feu doit etre allume pour cuisiner)", cx, btnY + 45, 18, 1, DARKGRAY);
        }
    } else {
        DrawTextCentered(game->uiFont, "(Sante et Mana au maximum)", cx, btnY + 45, 18, 1, GRAY);
    }

    if (!g_camp_fire_lit) {
        DrawTextCentered(game->uiFont, "FROID ABYSSAL : ATK et Production reduites !", cx, btnY + 90, 20, 1, RED);
    }

    // =========================================================
    // 3. MENUS DU CAMP (Latéraux et structurés)
    // =========================================================
    
    // Cadre décoratif pour la zone des menus en bas
    DrawLine(cx - 280, h - 230, cx + 280, h - 230, DARKGRAY);
    DrawTextCentered(game->uiFont, "[ ACTIVITES DISPONIBLES ]", cx, h - 250, 20, 1, GRAY);

    // Colonne de gauche
    DrawTextEx(game->uiFont, T("CAMP_BTN_MINE"),       (Vector2){cx - 250, h - 190}, 24, 1, LIGHTGRAY);
    DrawTextEx(game->uiFont, T("CAMP_BTN_FOREST"),     (Vector2){cx - 250, h - 150}, 24, 1, GREEN);
    DrawTextEx(game->uiFont, T("CAMP_BTN_FORGE"),      (Vector2){cx - 250, h - 110}, 24, 1, ORANGE);
    DrawTextEx(game->uiFont, T("CAMP_BTN_ALCHEMIST"),  (Vector2){cx - 250, h - 70},  24, 1, PINK);

    // Colonne de droite
    DrawTextEx(game->uiFont, T("CAMP_BTN_ARCHIFORGE"), (Vector2){cx + 50,  h - 190}, 24, 1, BLUE);
    DrawTextEx(game->uiFont, T("CAMP_BTN_INVENTORY"),  (Vector2){cx + 50,  h - 150}, 24, 1, YELLOW);
    DrawTextEx(game->uiFont, T("CAMP_BTN_DUNGEON"),    (Vector2){cx + 50,  h - 110}, 24, 1, PURPLE);
    DrawTextEx(game->uiFont, T("CAMP_BTN_ALTAR"),      (Vector2){cx + 50,  h - 70},  20, 1, RED);

    // Bouton retour tout en bas
    DrawTextCentered(game->uiFont, T("CAMP_BTN_MAIN_MENU"), cx, h - 30, 20, 1, DARKGRAY);
}

static long long compute_price(int base, int inc, int level)
{
    long long price = base; // On utilise la puissance du 64 bits !

    for (int i = 0; i < level; i++)
    {
        price = price + (price * inc) / 5; 
    }

    return price;
}