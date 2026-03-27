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

bool  g_camp_fire_lit = false;
float g_camp_fire_timer = 0.0f;

static int compute_price(int base, int inc, int level);



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

        // --- AFFICHAGE DU SPRITE DANS LA FORGE ---
        float scale = 4.0f; // Image bien grosse pour la forge !
        int scaledWidth = t->sprite.width * scale;
        int scaledHeight = t->sprite.height * scale;
        
        int imgX = shopX + (shopWidth / 2) - (scaledWidth / 2) - 20;
        int imgY = 150;

        if (t->sprite.id != 0) {
            // Teinte colorée selon la rareté
            DrawTextureEx(t->sprite, (Vector2){(float)imgX, (float)imgY}, 0.0f, scale, GetRarityColor(item->rarity));
        }

        // On positionne le texte des stats juste en dessous de la nouvelle image
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

        int cur_cost_fer = compute_price(t->cost_fer_base, t->cost_fer_inc, item->level);
        int cur_cost_bois = compute_price(t->cost_bois_base, t->cost_bois_inc, item->level);
         
        cur_cost_fer = cur_cost_fer*r_mult;
        cur_cost_bois = cur_cost_bois * r_mult;
        
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
            int upg_cost = compute_price(t->upg_gold_base, t->upg_gold_inc, lvl);

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
            int upg_cost   = compute_price(t->upg_gold_base, t->upg_gold_inc, lvl*lvl);
            int craft_cost = t->craft_herbs_base + ((lvl*10+1)* t->craft_herbs_inc);

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
    float frameTime = 0.30f;
    int   seed      = (int)(GetTime() / frameTime);
    SetRandomSeed(seed);
    
    int   cx            = (w * 0.2f) + ((w * 0.55f) / 2);
    int   cy            = h / 2;
    float asciiFontSize = 20;
    float spacing       = 2;

    DrawTextEx(game->uiFont, T("CAMP_TITLE"), (Vector2){cx - 100, 80}, 40, 1, GREEN);

    // --- 1. DESSIN DE LA FUMÉE (Seulement si allumé) ---
    if (g_camp_fire_lit) {
        for (int s = 0; s < 3; s++) {
            int   smokeX   = cx + GetRandomValue(-40, 40);
            int   smokeY   = cy - 130 + GetRandomValue(-20, 20);
            Color smokeCol = (Color){120, 120, 120, (unsigned char)GetRandomValue(100, 180)};
            DrawTextEx(game->dungeonFont, "▒", (Vector2){(float)smokeX, (float)smokeY}, asciiFontSize, spacing, smokeCol);
        }
    }

    // --- 2. L'ASCII DU FEU ---
    const char* fireAscii[] = {
        "                       ", 
        "    ▓██▄      ▓██▄     ", "    ▀███  ███████████   ", "         █████████████  ", "   ▄████  ████████████▀  ",
        "  ████████████▓▀███▀████     ▄▄ ", "  █████████████▀   █████▄   ▄███", "  ▀██████████▀     ▀███████████", "   ████████      ▄▄   ▀███████▀",
        "    ▀▀█████████████     ██████ ",  "  ▄▄██████████████▀█▄█████████▄▄", "  █████████████████████████▀▀▀",  "   ▀███████████▀  ▀█████████▀ "
    };

    int lineCount = 13;
    for (int i = 0; i < lineCount; i++)
    {
        float flickerX = (g_camp_fire_lit && i < 11) ? (float)GetRandomValue(-1, 1) : 0;
        Vector2 textSize = MeasureTextEx(game->dungeonFont, fireAscii[i], asciiFontSize, spacing);
        Vector2 pos      = {cx - (textSize.x / 2) + flickerX, cy - 140 + (i * asciiFontSize)};

        Color col;
        int intensity = GetRandomValue(0, 40);

        if (g_camp_fire_lit) {
            if (i <= 4) col = (Color){255, 255 - intensity, intensity, 255}; // Jaune/Blanc
            else if (i <= 8) col = (Color){255, 160 - intensity, 0, 255}; // Orange
            else if (i <= 10) col = (Color){220 - intensity, 20, 0, 255}; // Rouge
            else col = (Color){100, 60, 30, 255}; // Bûches
        } else {
            // Feu éteint : braises et bois froid
            if (i <= 8) col = BLANK; // Pas de flammes hautes
            else if (i <= 10) col = (Color){80 + intensity, 20, 10, 255}; // Braises mourantes
            else col = (Color){60, 40, 20, 255}; // Bois sombre
        }

        DrawTextEx(game->dungeonFont, fireAscii[i], (Vector2){pos.x + flickerX, pos.y}, asciiFontSize, spacing, col);
    }

    // --- 3. GESTION DE LA SURVIE (Boutons au centre) ---
    int btnY = cy + 130;

    // Bouton Allumer/Eteindre
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

    // Bouton Cuire Viande (Calcul du besoin de soin)
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

                // On soigne d'abord les HP, puis le Mana
                int hp_to_heal = (missing_hp < points_to_heal) ? missing_hp : points_to_heal;
                game->combat.player.hp += hp_to_heal;
                points_to_heal -= hp_to_heal;
                if (points_to_heal > 0) game->combat.player.mana += points_to_heal;
            }
        } else {
            DrawTextEx(game->uiFont, "(Le feu doit etre allume pour cuisiner)", (Vector2){cx - 190, btnY + 45}, 18, 1, DARKGRAY);
        }
    } else {
        DrawTextEx(game->uiFont, "(Sante et Mana au maximum)", (Vector2){cx - 130, btnY + 45}, 18, 1, GRAY);
    }

    if (!g_camp_fire_lit) {
        DrawTextCentered(game->uiFont, "FROID ABYSSAL : ATK et Production reduites !", cx, btnY + 90, 20, 1, RED);
    }

    // --- 4. MENUS DU CAMP (Latéraux) ---
    // Colonne de gauche
    DrawTextEx(game->uiFont, T("CAMP_BTN_MINE"),       (Vector2){cx - 300, h - 200}, 24, 1, LIGHTGRAY);
    DrawTextEx(game->uiFont, T("CAMP_BTN_FOREST"),     (Vector2){cx - 300, h - 160}, 24, 1, GREEN);
    DrawTextEx(game->uiFont, T("CAMP_BTN_FORGE"),      (Vector2){cx - 300, h - 120}, 24, 1, ORANGE);
    DrawTextEx(game->uiFont, T("CAMP_BTN_ALCHEMIST"),  (Vector2){cx - 300, h - 80},  24, 1, PINK);

    // Colonne de droite
    DrawTextEx(game->uiFont, T("CAMP_BTN_ARCHIFORGE"), (Vector2){cx + 70,  h - 200}, 24, 1, BLUE);
    DrawTextEx(game->uiFont, T("CAMP_BTN_INVENTORY"),  (Vector2){cx + 70,  h - 160}, 24, 1, YELLOW);
    DrawTextEx(game->uiFont, T("CAMP_BTN_DUNGEON"),    (Vector2){cx + 70,  h - 120}, 24, 1, PURPLE);
    DrawTextEx(game->uiFont, T("CAMP_BTN_ALTAR"),      (Vector2){cx + 70,  h - 80},  20, 1, RED);

    // Bouton retour au centre en bas
    DrawTextEx(game->uiFont, T("CAMP_BTN_MAIN_MENU"),  (Vector2){cx - 100, h - 40},  20, 1, DARKGRAY);
}


static int compute_price(int base, int inc, int level)
{
    int price = base;

    for (int i = 0; i < level; i++)
    {
        price = price + (price * inc) / 5; 
    }

    return price;
}