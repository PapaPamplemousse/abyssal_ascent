#include "combat.h"
#include <stdio.h>
#include <string.h>
#include "../../utils/inc/cJSON.h"
#include "../../utils/inc/lang.h"

// 1. Monstres
MonsterTemplate g_monsterDB[MAX_MONSTERS_DB];
int             g_monsterCount = 0;

// 2. Objets (Équipement)
ItemTemplate g_itemDB[MAX_ITEMS_DB];
int          g_itemCount = 0;

// 3. Magie et Potions
SpellTemplate g_spellDB[MAX_SPELLS_DB];
int           g_spellCount = 0;

PotionTemplate g_potionDB[MAX_POTIONS_DB];
int            g_potionCount = 0;

EffectType ParseEffectType(const char* str)
{
    if (strstr(str, "DAMAGE"))
        return SPELL_DAMAGE;
    if (strstr(str, "HEAL"))
        return SPELL_HEAL;
    if (strstr(str, "POISON"))
        return SPELL_POISON;
    if (strstr(str, "FREEZE"))
        return SPELL_FREEZE;
    if (strstr(str, "VAMPIRISM"))
        return SPELL_VAMPIRISM;
    if (strstr(str, "STUN"))
        return SPELL_STUN;
    if (strstr(str, "MANA"))
        return SPELL_HEAL; // Simplification
    return SPELL_DAMAGE;
}

// --- LE CHARGEUR JSON ---
void LoadMonstersDB(const char* filepath)
{
    // 1. Lire le fichier
    char* buffer = LoadFileText(filepath); // Fonction Raylib hyper pratique
    if (!buffer)
    {
        printf("Erreur: Impossible de charger %s\n", filepath);
        return;
    }

    // 2. Parser avec cJSON
    cJSON* json = cJSON_Parse(buffer);
    if (!json)
    {
        printf("Erreur de syntaxe JSON.\n");
        UnloadFileText(buffer);
        return;
    }

    // 3. Récupérer le tableau "monsters"
    cJSON* monstersArray = cJSON_GetObjectItemCaseSensitive(json, "monsters");
    cJSON* monsterNode   = NULL;

    g_monsterCount = 0;

    // 4. Boucler sur chaque monstre
    cJSON_ArrayForEach(monsterNode, monstersArray)
    {
        if (g_monsterCount >= MAX_MONSTERS_DB)
            break;
        MonsterTemplate* t = &g_monsterDB[g_monsterCount];

        strcpy(t->id, cJSON_GetObjectItem(monsterNode, "id")->valuestring);
        strcpy(t->name_en, cJSON_GetObjectItem(monsterNode, "name_en")->valuestring);
        strcpy(t->name_fr, cJSON_GetObjectItem(monsterNode, "name_fr")->valuestring);
        strcpy(t->flavor_en, cJSON_GetObjectItem(monsterNode, "flavor_en")->valuestring);
        strcpy(t->flavor_fr, cJSON_GetObjectItem(monsterNode, "flavor_fr")->valuestring);

        t->is_boss = cJSON_GetObjectItem(monsterNode, "is_boss")->valueint; // cJSON utilise valueint pour les booleens (0/1)
        if (t->is_boss)
        {
            t->boss_floor = cJSON_GetObjectItem(monsterNode, "boss_floor")->valueint;
        }
        else
        {
            t->min_floor = cJSON_GetObjectItem(monsterNode, "min_floor")->valueint;
            t->max_floor = cJSON_GetObjectItem(monsterNode, "max_floor")->valueint;
        }

        cJSON* stats = cJSON_GetObjectItem(monsterNode, "stats");
        t->hp        = cJSON_GetObjectItem(stats, "hp")->valueint;
        t->atk       = cJSON_GetObjectItem(stats, "atk")->valueint;
        t->spd       = cJSON_GetObjectItem(stats, "spd")->valuedouble;
        t->xp        = cJSON_GetObjectItem(stats, "xp")->valueint;

        cJSON* asciiArray   = cJSON_GetObjectItem(monsterNode, "ascii");
        cJSON* line         = NULL;
        t->ascii_line_count = 0;
        cJSON_ArrayForEach(line, asciiArray)
        {
            if (t->ascii_line_count < MAX_ASCII_LINES)
            {
                strcpy(t->ascii[t->ascii_line_count], line->valuestring);
                t->ascii_line_count++;
            }
        }
        g_monsterCount++;
    }

    cJSON_Delete(json);     // Libérer la mémoire du parseur
    UnloadFileText(buffer); // Libérer le fichier
    printf("--- CHARGEMENT OK : %d monstres charges depuis le JSON ---\n", g_monsterCount);
}

void LoadItemsDB(const char* filepath)
{
    char* buffer = LoadFileText(filepath);
    if (!buffer)
        return;

    cJSON* json = cJSON_Parse(buffer);
    if (!json)
    {
        UnloadFileText(buffer);
        return;
    }

    cJSON* itemsArray = cJSON_GetObjectItemCaseSensitive(json, "items");
    cJSON* itemNode   = NULL;
    g_itemCount       = 0;

    cJSON_ArrayForEach(itemNode, itemsArray)
    {
        if (g_itemCount >= MAX_ITEMS_DB)
            break;
        ItemTemplate* t = &g_itemDB[g_itemCount];

        strcpy(t->id, cJSON_GetObjectItem(itemNode, "id")->valuestring);
        strcpy(t->name_en, cJSON_GetObjectItem(itemNode, "name_en")->valuestring);
        strcpy(t->name_fr, cJSON_GetObjectItem(itemNode, "name_fr")->valuestring);
        strcpy(t->type, cJSON_GetObjectItem(itemNode, "type")->valuestring);

        cJSON* stats = cJSON_GetObjectItem(itemNode, "stats");
        t->hp        = cJSON_GetObjectItem(stats, "hp")->valueint;
        t->atk       = cJSON_GetObjectItem(stats, "atk")->valueint;
        t->mana      = cJSON_GetObjectItem(stats, "mana")->valueint;
        t->spd       = cJSON_GetObjectItem(stats, "spd")->valuedouble;
        t->fog       = cJSON_GetObjectItem(stats, "fog")->valueint;

        cJSON* stat_inc = cJSON_GetObjectItem(itemNode, "stat_inc");
        t->inc_hp       = cJSON_GetObjectItem(stat_inc, "hp")->valueint;
        t->inc_atk      = cJSON_GetObjectItem(stat_inc, "atk")->valueint;
        t->inc_mana     = cJSON_GetObjectItem(stat_inc, "mana")->valueint;
        t->inc_spd      = cJSON_GetObjectItem(stat_inc, "spd")->valuedouble;
        t->inc_fog      = cJSON_GetObjectItem(stat_inc, "fog")->valueint;

        cJSON* upgrade   = cJSON_GetObjectItem(itemNode, "upgrade");
        cJSON* ferNode   = cJSON_GetObjectItem(upgrade, "fer");
        t->cost_fer_base = ferNode ? cJSON_GetObjectItem(ferNode, "base")->valueint : 0;
        t->cost_fer_inc  = ferNode ? cJSON_GetObjectItem(ferNode, "inc")->valueint : 0;

        cJSON* boisNode   = cJSON_GetObjectItem(upgrade, "bois");
        t->cost_bois_base = boisNode ? cJSON_GetObjectItem(boisNode, "base")->valueint : 0;
        t->cost_bois_inc  = boisNode ? cJSON_GetObjectItem(boisNode, "inc")->valueint : 0;

        // --- LA CORRECTION DE L'ASCII EST ICI ---
        cJSON* asciiArray   = cJSON_GetObjectItem(itemNode, "ascii");
        cJSON* line         = NULL;
        t->ascii_line_count = 0; // TRÈS IMPORTANT : Initialiser à 0 !
        if (asciiArray)
        {
            cJSON_ArrayForEach(line, asciiArray)
            {
                if (t->ascii_line_count < MAX_ITEM_ASCII_LINES)
                {
                    strcpy(t->ascii[t->ascii_line_count], line->valuestring);
                    t->ascii_line_count++;
                }
            }
        }
        g_itemCount++;
    }
    cJSON_Delete(json);
    UnloadFileText(buffer);
}

void LoadMagicDB(const char* spell_path, const char* potion_path)
{
    // --- 1. CHARGEMENT DES SORTS ---
    char* s_buf = LoadFileText(spell_path);
    if (s_buf)
    {
        cJSON* json = cJSON_Parse(s_buf);
        if (json)
        {
            cJSON* arr = cJSON_GetObjectItemCaseSensitive(json, "spells");
            cJSON* item;
            g_spellCount = 0;
            cJSON_ArrayForEach(item, arr)
            {
                if (g_spellCount >= MAX_SPELLS_DB)
                    break;
                SpellTemplate* t = &g_spellDB[g_spellCount];

                strcpy(t->id, cJSON_GetObjectItem(item, "id")->valuestring);
                strcpy(t->name_en, cJSON_GetObjectItem(item, "name_en")->valuestring);
                strcpy(t->name_fr, cJSON_GetObjectItem(item, "name_fr")->valuestring);
                t->type = ParseEffectType(cJSON_GetObjectItem(item, "type")->valuestring);

                t->base_val = cJSON_GetObjectItem(item, "base_val")->valueint;
                t->inc_val  = cJSON_GetObjectItem(item, "inc_val")->valueint;

                // base_dur et inc_dur peuvent ne pas exister pour les sorts de dégâts directs
                cJSON* bd   = cJSON_GetObjectItem(item, "base_dur");
                t->base_dur = bd ? bd->valuedouble : 0.0f;
                cJSON* idur = cJSON_GetObjectItem(item, "inc_dur");
                t->inc_dur  = idur ? idur->valuedouble : 0.0f;

                t->mana_cost     = cJSON_GetObjectItem(item, "mana_cost")->valueint;
                t->learn_gold    = cJSON_GetObjectItem(item, "learn_gold")->valueint;
                t->learn_crystal = cJSON_GetObjectItem(item, "learn_crystal")->valueint;
                t->upg_gold_base = cJSON_GetObjectItem(item, "upg_gold_base")->valueint;
                t->upg_gold_inc  = cJSON_GetObjectItem(item, "upg_gold_inc")->valueint;
                t->prep_crystal  = cJSON_GetObjectItem(item, "prep_crystal")->valueint;

                g_spellCount++;
            }
            cJSON_Delete(json);
        }
        UnloadFileText(s_buf);
    }

    // --- 2. CHARGEMENT DES POTIONS ---
    char* p_buf = LoadFileText(potion_path);
    if (p_buf)
    {
        cJSON* json = cJSON_Parse(p_buf);
        if (json)
        {
            cJSON* arr = cJSON_GetObjectItemCaseSensitive(json, "potions");
            cJSON* item;
            g_potionCount = 0;
            cJSON_ArrayForEach(item, arr)
            {
                if (g_potionCount >= MAX_POTIONS_DB)
                    break;
                PotionTemplate* t = &g_potionDB[g_potionCount];

                strcpy(t->id, cJSON_GetObjectItem(item, "id")->valuestring);
                strcpy(t->name_en, cJSON_GetObjectItem(item, "name_en")->valuestring);
                strcpy(t->name_fr, cJSON_GetObjectItem(item, "name_fr")->valuestring);
                t->type = ParseEffectType(cJSON_GetObjectItem(item, "type")->valuestring);

                t->base_val = cJSON_GetObjectItem(item, "base_val")->valueint;
                t->inc_val  = cJSON_GetObjectItem(item, "inc_val")->valueint;

                t->learn_gold       = cJSON_GetObjectItem(item, "learn_gold")->valueint;
                t->upg_gold_base    = cJSON_GetObjectItem(item, "upg_gold_base")->valueint;
                t->upg_gold_inc     = cJSON_GetObjectItem(item, "upg_gold_inc")->valueint;
                t->craft_herbs_base = cJSON_GetObjectItem(item, "craft_herbs_base")->valueint;
                t->craft_herbs_inc  = cJSON_GetObjectItem(item, "craft_herbs_inc")->valueint;

                g_potionCount++;
            }
            cJSON_Delete(json);
        }
        UnloadFileText(p_buf);
    }
    printf("--- MAGIE CHARGEE : %d sorts, %d potions ---\n", g_spellCount, g_potionCount);
}

void Inventory_Add(CombatContext* combat, const char* item_id)
{
    if (combat->player.inventory_count >= MAX_INVENTORY)
        return; // Inventaire plein

    // On cherche l'objet dans la base de données via son ID ("rusty_sword", "torch"...)
    for (int i = 0; i < g_itemCount; i++)
    {
        if (strcmp(g_itemDB[i].id, item_id) == 0)
        {
            combat->player.inventory[combat->player.inventory_count].template_idx = i;
            combat->player.inventory[combat->player.inventory_count].level        = 0; // Niveau de base
            combat->player.inventory_count++;

            char log[64];
            sprintf(log, T("LOG_GAIN_ITEM"), g_isEnglish ? g_itemDB[i].name_en : g_itemDB[i].name_fr);
            Combat_AddLog(combat, log);
            return;
        }
    }
}

void Combat_Init(CombatContext* combat)
{
    combat->player.inventory_count = 0;

    for (int i = 0; i < MAX_SLOTS; i++)
    {
        combat->player.equipped[i] = -1;
    }

    for (int i = 0; i < MAX_SPELLS_DB; i++)
    {
        combat->player.spell_unlocked[i] = false;
        combat->player.spell_level[i]    = 0;
    }
    for (int i = 0; i < 3; i++)
    {
        combat->player.equipped_spells[i]  = -1;
        combat->player.equipped_potions[i] = -1;
    }

    Combat_RecalculateStats(combat);
    combat->is_active = false;
    for (int i = 0; i < 5; i++)
        strcpy(combat->battle_log[i], "");
    combat->log_index = 0;

    Combat_ResetRun(combat);
    combat->player.hp   = combat->player.max_hp;
    combat->player.mana = combat->player.max_mana;
}

void Combat_ResetRun(CombatContext* combat)
{
    combat->player.level  = 1;
    combat->player.xp     = 0;
    combat->player.max_xp = 100;

    combat->player.max_hp   = combat->player.base_max_hp;
    combat->player.max_mana = combat->player.base_max_mana;
    combat->player.atk      = combat->player.base_atk;
    combat->player.spd      = combat->player.base_spd;

    combat->is_active = false;
    Combat_AddLog(combat, T("NEW_RUN"));
}

void Combat_AddLog(CombatContext* combat, const char* msg)
{
    // Fait remonter les anciens messages
    for (int i = 4; i > 0; i--)
    {
        strcpy(combat->battle_log[i], combat->battle_log[i - 1]);
    }
    strcpy(combat->battle_log[0], msg);
}

void Combat_StartEncounter(CombatContext* combat, int current_floor, bool is_boss_room)
{
    combat->is_active           = true;
    combat->player_attack_timer = 0.0f;
    combat->enemy_attack_timer  = 0.0f;

    // 1. Trouver une liste de candidats valides pour cet étage
    MonsterTemplate* valid_candidates[MAX_MONSTERS_DB];
    int              candidate_count = 0;

    for (int i = 0; i < g_monsterCount; i++)
    {
        MonsterTemplate* t = &g_monsterDB[i];

        if (is_boss_room)
        {
            if (t->is_boss && t->boss_floor == current_floor)
            {
                valid_candidates[candidate_count++] = t;
            }
        }
        else
        {
            if (!t->is_boss && current_floor >= t->min_floor && current_floor <= t->max_floor)
            {
                valid_candidates[candidate_count++] = t;
            }
        }
    }

    // Sécurité anti-crash au cas où le JSON est vide
    if (candidate_count == 0)
        return;

    // 2. Tirer un monstre au hasard parmi les candidats valides
    MonsterTemplate* chosen = valid_candidates[GetRandomValue(0, candidate_count - 1)];

    // 3. Copier les données du Template vers l'Ennemi Actuel
    strcpy(combat->current_enemy.name, g_isEnglish ? chosen->name_en : chosen->name_fr);
    strcpy(combat->current_enemy.flavor, g_isEnglish ? chosen->flavor_en : chosen->flavor_fr);

    combat->current_enemy.max_hp   = chosen->hp;
    combat->current_enemy.hp       = chosen->hp;
    combat->current_enemy.atk      = chosen->atk;
    combat->current_enemy.spd      = chosen->spd;
    combat->current_enemy.xp_yield = chosen->xp;

    combat->current_enemy.ascii_line_count = chosen->ascii_line_count;
    for (int i = 0; i < chosen->ascii_line_count; i++)
    {
        strcpy(combat->current_enemy.ascii[i], chosen->ascii[i]);
    }

    combat->current_enemy.qte_active = false;
    combat->current_enemy.qte_timer  = 2.0f;

    combat->current_enemy.poison_timer = 0.0f;
    combat->current_enemy.freeze_timer = 0.0f;
    combat->current_enemy.stun_timer   = 0.0f;

    // Message d'ambiance !
    Combat_AddLog(combat, combat->current_enemy.flavor);
}

extern SpellTemplate  g_spellDB[MAX_SPELLS_DB];
extern PotionTemplate g_potionDB[MAX_POTIONS_DB];

void Combat_Update(CombatContext* combat, float deltaTime, int centerX, int centerY)
{
    if (!combat->is_active)
        return;

    if (combat->screen_flash_timer > 0.0f)
    {
        combat->screen_flash_timer -= deltaTime;
    }

    // --- 1. GESTION DES ALTÉRATIONS D'ÉTAT (Monstre) ---
    if (combat->current_enemy.poison_timer > 0)
    {
        combat->current_enemy.poison_timer -= deltaTime;
        combat->current_enemy.poison_tick -= deltaTime;
        if (combat->current_enemy.poison_tick <= 0)
        {
            combat->current_enemy.hp -= combat->current_enemy.poison_dmg;
            Combat_AddLog(combat, T("LOG_POISON_DAMAGE"));
            combat->current_enemy.poison_tick = 1.0f; // Dégâts toutes les secondes
        }
    }
    if (combat->current_enemy.freeze_timer > 0)
        combat->current_enemy.freeze_timer -= deltaTime;
    if (combat->current_enemy.stun_timer > 0)
        combat->current_enemy.stun_timer -= deltaTime;

    for (int i = 0; i < 3; i++)
    {
        if (IsKeyPressed(KEY_ONE + i) || IsKeyPressed(KEY_KP_1 + i))
        {
            int p_idx = combat->player.equipped_potions[i];
            if (p_idx != -1 && combat->player.potion_qty[p_idx] > 0)
            {
                combat->player.potion_qty[p_idx]--;
                PotionTemplate* t   = &g_potionDB[p_idx];
                int             val = t->base_val + (combat->player.potion_level[p_idx] * t->inc_val);

                if (t->type == SPELL_HEAL)
                {
                    combat->player.hp += val;
                    combat->screen_flash_color = (Color){255, 0, 0, 60}; // Flash Rouge
                }
                else
                {
                    combat->player.mana += val;
                    combat->screen_flash_color = (Color){0, 150, 255, 60}; // Flash Bleu
                }
                combat->screen_flash_timer = 0.15f; // Durée du flash

                if (combat->player.hp > combat->player.max_hp)
                    combat->player.hp = combat->player.max_hp;
                if (combat->player.mana > combat->player.max_mana)
                    combat->player.mana = combat->player.max_mana;

                char log[64];
                sprintf(log, "> %s : %s", T("WORD_UTILISE"), g_isEnglish ? t->name_en : t->name_fr);
                Combat_AddLog(combat, log);
            }
        }
    }

    // --- 3. UTILISATION DES SORTS ---
    for (int i = 0; i < 3; i++)
    {
        if (IsKeyPressed(KEY_FOUR + i) || IsKeyPressed(KEY_KP_4 + i))
        {
            int s_idx = combat->player.equipped_spells[i];
            if (s_idx != -1)
            {
                SpellTemplate* t = &g_spellDB[s_idx];
                if (combat->player.mana >= t->mana_cost)
                {
                    combat->player.mana -= t->mana_cost;
                    int   lvl = combat->player.spell_level[s_idx];
                    int   val = t->base_val + (lvl * t->inc_val);
                    float dur = t->base_dur + (lvl * t->inc_dur);

                    if (t->type == SPELL_DAMAGE)
                    {
                        combat->current_enemy.hp -= val;
                        combat->screen_flash_color = (Color){255, 100, 0, 60}; // Flash Orange
                    }
                    else if (t->type == SPELL_POISON)
                    {
                        combat->current_enemy.poison_timer = dur;
                        combat->current_enemy.poison_dmg   = val;
                        combat->current_enemy.poison_tick  = 1.0f;
                        combat->screen_flash_color         = (Color){150, 255, 50, 60}; // Flash Vert Toxique
                    }
                    else if (t->type == SPELL_FREEZE)
                    {
                        combat->current_enemy.freeze_timer       = dur;
                        combat->current_enemy.freeze_slow_factor = val;
                        combat->screen_flash_color               = (Color){50, 200, 255, 60}; // Flash Cyan
                    }
                    else if (t->type == SPELL_VAMPIRISM)
                    {
                        combat->current_enemy.hp -= val;
                        combat->player.hp += val;
                        if (combat->player.hp > combat->player.max_hp)
                            combat->player.hp = combat->player.max_hp;
                        combat->screen_flash_color = (Color){200, 0, 50, 60}; // Flash Rouge Sang
                    }
                    else if (t->type == SPELL_STUN)
                    {
                        combat->current_enemy.stun_timer = dur;
                        combat->screen_flash_color       = (Color){255, 255, 0, 60}; // Flash Jaune
                    }

                    combat->screen_flash_timer = 0.15f; // Durée du flash
                    char log[64];
                    sprintf(log, "> Sort : %s", g_isEnglish ? t->name_en : t->name_fr);
                    Combat_AddLog(combat, log);
                }
                else
                {
                    Combat_AddLog(combat, "Pas assez de Mana !");
                }
            }
        }
    }

    // --- 4. AUTO-ATTAQUE DU JOUEUR ---
    combat->player_attack_timer += deltaTime * combat->player.spd;
    if (combat->player_attack_timer >= 1.0f)
    {
        combat->current_enemy.hp -= combat->player.atk;
        char log[64];
        sprintf(log, "Vous frappez (%d degats)", combat->player.atk);
        Combat_AddLog(combat, log);
        combat->player_attack_timer -= 1.0f;
    }

    // --- 5. AUTO-ATTAQUE DE L'ENNEMI ---
    // L'ennemi n'attaque PAS s'il est étourdi (Stun)
    if (combat->current_enemy.stun_timer <= 0.0f)
    {
        float current_spd = combat->current_enemy.spd;

        // S'il est gelé, sa vitesse est réduite ! (Ex: ralentit de 30%)
        if (combat->current_enemy.freeze_timer > 0.0f)
        {
            current_spd *= (1.0f - (combat->current_enemy.freeze_slow_factor / 100.0f));
        }

        combat->enemy_attack_timer += deltaTime * current_spd;
        if (combat->enemy_attack_timer >= 1.0f)
        {
            combat->player.hp -= combat->current_enemy.atk;
            char log[64];
            sprintf(log, "%s frappe (%d degats)", combat->current_enemy.name, combat->current_enemy.atk);
            Combat_AddLog(combat, log);
            combat->enemy_attack_timer -= 1.0f;
        }
    }

    // --- 6. GESTION DU POINT FAIBLE (QTE) ---
    combat->current_enemy.qte_timer -= deltaTime;
    if (combat->current_enemy.qte_timer <= 0.0f)
    {
        if (!combat->current_enemy.qte_active)
        {
            combat->current_enemy.qte_active = true;
            combat->current_enemy.qte_pos.x  = centerX - 50 + (GetRandomValue(0, 100));
            combat->current_enemy.qte_pos.y  = centerY - 50 + (GetRandomValue(0, 100));
            combat->current_enemy.qte_timer  = 1.5f;
        }
        else
        {
            combat->current_enemy.qte_active = false;
            combat->current_enemy.qte_timer  = GetRandomValue(3, 6);
        }
    }

    if (combat->current_enemy.qte_active)
    {
        Rectangle qte_rect = {combat->current_enemy.qte_pos.x, combat->current_enemy.qte_pos.y, 40, 40};
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), qte_rect))
        {
            int crit_dmg = combat->player.atk * 2;
            combat->current_enemy.hp -= crit_dmg;
            combat->current_enemy.qte_active = false;
            combat->current_enemy.qte_timer  = GetRandomValue(3, 6);
            Combat_AddLog(combat, T("WEAK_POINT"));
        }
    }

    // --- 7. CONDITIONS DE FIN DE COMBAT ---
    if (combat->current_enemy.hp <= 0)
    {
        combat->is_active = false;
        Combat_AddLog(combat, T("ENEMY_DEFEATED"));

        combat->player.xp += combat->current_enemy.xp_yield;
        if (combat->player.xp >= combat->player.max_xp)
        {
            combat->player.level++;
            combat->player.xp -= combat->player.max_xp;
            combat->player.max_xp = (int)(combat->player.max_xp * 1.5);
            combat->player.max_hp += 10;
            combat->player.hp = combat->player.max_hp;
            combat->player.atk += 2;
            Combat_AddLog(combat, T("LEVEL_UP"));
        }
    }
}

// --- FONCTION DE TRI D'INVENTAIRE ---
// Tri de type "Insertion" simple et stable.
void Inventory_GetSortedIndices(CombatContext* combat, int* indices)
{
    int count = combat->player.inventory_count;
    for (int i = 0; i < count; i++)
        indices[i] = i;

    for (int i = 1; i < count; i++)
    {
        int key_idx = indices[i];
        int j       = i - 1;

        while (j >= 0)
        {
            bool swap = false;

            // CORRECTION : On cherche sur MAX_SLOTS
            bool is_equipped_i = false;
            for (int s = 0; s < MAX_SLOTS; s++)
                if (combat->player.equipped[s] == key_idx)
                    is_equipped_i = true;
            bool is_equipped_j = false;
            for (int s = 0; s < MAX_SLOTS; s++)
                if (combat->player.equipped[s] == indices[j])
                    is_equipped_j = true;

            if (is_equipped_i && !is_equipped_j)
                swap = true;
            else if (is_equipped_i == is_equipped_j)
            {
                if (key_idx > indices[j])
                    swap = true;
            }

            if (swap)
            {
                indices[j + 1] = indices[j];
                j              = j - 1;
            }
            else
                break;
        }
        indices[j + 1] = key_idx;
    }
}

void Combat_RenderCenter(CombatContext* combat, Font font, int centerX, int centerY)
{
    if (!combat->is_active) return;

    // --- 1. DESSIN DU FLASH D'ÉCRAN ---
    if (combat->screen_flash_timer > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), combat->screen_flash_color);
    }

    // --- 2. COULEUR DYNAMIQUE DU MONSTRE ---
    Color mColor = LIGHTGRAY;
    if (combat->current_enemy.freeze_timer > 0) mColor = SKYBLUE;
    else if (combat->current_enemy.poison_timer > 0) mColor = LIME;
    else if (combat->current_enemy.stun_timer > 0) mColor = YELLOW;

    int line_height = 20;
    int startY = centerY - ((combat->current_enemy.ascii_line_count * line_height) / 2);

    // --- 3. TAGS DE STATUT AU-DESSUS DU MONSTRE ---
    int tagY = startY - 30; // On commence à dessiner au-dessus
    if (combat->current_enemy.stun_timer > 0) {
        const char* txt = g_isEnglish ? "[ STUNNED ]" : "[ ETOURDI ]";
        Vector2 tSize = MeasureTextEx(font, txt, 20, 1);
        DrawTextEx(font, txt, (Vector2){centerX - (tSize.x / 2), tagY}, 20, 1, YELLOW);
        tagY -= 25;
    }
    if (combat->current_enemy.freeze_timer > 0) {
        const char* txt = g_isEnglish ? "[ FROZEN ]" : "[ GELE ]";
        Vector2 tSize = MeasureTextEx(font, txt, 20, 1);
        DrawTextEx(font, txt, (Vector2){centerX - (tSize.x / 2), tagY}, 20, 1, SKYBLUE);
        tagY -= 25;
    }
    if (combat->current_enemy.poison_timer > 0) {
        const char* txt = g_isEnglish ? "[ POISONED ]" : "[ EMPOISONNE ]";
        Vector2 tSize = MeasureTextEx(font, txt, 20, 1);
        DrawTextEx(font, txt, (Vector2){centerX - (tSize.x / 2), tagY}, 20, 1, LIME);
        tagY -= 25;
    }

    // --- 4. DESSIN DU MONSTRE ---
    for (int i = 0; i < combat->current_enemy.ascii_line_count; i++)
    {
        DrawTextEx(font, combat->current_enemy.ascii[i], (Vector2){centerX - 250, startY + (i * line_height)}, 20, 1, mColor);
    }

    // --- 5. QTE ET POINTS DE VIE ---
    if (combat->current_enemy.qte_active)
    {
        DrawRectangleLines(combat->current_enemy.qte_pos.x, combat->current_enemy.qte_pos.y, 40, 40, YELLOW);
        DrawTextEx(font, "[X]", (Vector2){combat->current_enemy.qte_pos.x + 5, combat->current_enemy.qte_pos.y + 10}, 24, 1, YELLOW);
    }

    char hpText[64];
    sprintf(hpText, "[ %s : %d / %d HP ]", combat->current_enemy.name, combat->current_enemy.hp, combat->current_enemy.max_hp);
    Vector2 tSize = MeasureTextEx(font, hpText, 24, 1);
    
    int asciiHeight = combat->current_enemy.ascii_line_count * line_height;
    int textY       = startY + asciiHeight + 10; 
    DrawTextEx(font, hpText, (Vector2){centerX - (tSize.x / 2), textY}, 24, 1, RED);
}

void Combat_RecalculateStats(CombatContext* combat)
{
    combat->player.base_max_hp   = 50;
    combat->player.base_atk      = 5;
    combat->player.base_max_mana = 20;
    combat->player.base_spd      = 0.8f;
    combat->player.fog_bonus     = 0;

    // CORRECTION : On boucle sur MAX_SLOTS pour inclure HAND_2 et LEGGINGS
    for (int i = 0; i < MAX_SLOTS; i++)
    {
        int inv_idx = combat->player.equipped[i];
        if (inv_idx != -1)
        {
            OwnedItem*    item = &combat->player.inventory[inv_idx];
            ItemTemplate* t    = &g_itemDB[item->template_idx];

            combat->player.base_max_hp += t->hp + (item->level * t->inc_hp);
            combat->player.base_atk += t->atk + (item->level * t->inc_atk);
            combat->player.base_max_mana += t->mana + (item->level * t->inc_mana);
            combat->player.base_spd += t->spd + (item->level * t->inc_spd);
            combat->player.fog_bonus += t->fog + (item->level * t->inc_fog);
        }
    }

    combat->player.max_hp   = combat->player.base_max_hp;
    combat->player.max_mana = combat->player.base_max_mana;
    combat->player.atk      = combat->player.base_atk;
    combat->player.spd      = combat->player.base_spd;
}

void Inventory_Equip(CombatContext* combat, int inv_idx)
{
    if (inv_idx < 0 || inv_idx >= combat->player.inventory_count)
        return;
    ItemTemplate* t = &g_itemDB[combat->player.inventory[inv_idx].template_idx];

    EquipSlot target_slot = SLOT_NONE;

    // On utilise strstr pour éviter les bugs d'espaces invisibles dans le JSON !
    if (strstr(t->type, "HELMET"))
        target_slot = SLOT_HELMET;
    else if (strstr(t->type, "ARMOR"))
        target_slot = SLOT_ARMOR;
    else if (strstr(t->type, "GLOVES"))
        target_slot = SLOT_GLOVES;
    else if (strstr(t->type, "LEGGINGS"))
        target_slot = SLOT_LEGGINGS;
    else if (strstr(t->type, "BOOTS"))
        target_slot = SLOT_BOOTS;
    else if (strstr(t->type, "HAND_2H"))
        target_slot = SLOT_HAND_1; // Doit être avant HAND_2
    else if (strstr(t->type, "HAND_1"))
        target_slot = SLOT_HAND_1;
    else if (strstr(t->type, "HAND_2"))
        target_slot = SLOT_HAND_2;

    if (target_slot != SLOT_NONE)
    {
        // Logique stricte pour les armes à 2 mains
        if (strstr(t->type, "HAND_2H"))
        {
            combat->player.equipped[SLOT_HAND_2] = -1; // Déséquipe la main gauche
        }
        else if (target_slot == SLOT_HAND_2)
        {
            int main1_idx = combat->player.equipped[SLOT_HAND_1];
            if (main1_idx != -1 && strstr(g_itemDB[combat->player.inventory[main1_idx].template_idx].type, "HAND_2H"))
            {
                combat->player.equipped[SLOT_HAND_1] = -1;
            }
        }

        // On équipe enfin l'objet à la bonne place
        combat->player.equipped[target_slot] = inv_idx;
        Combat_RecalculateStats(combat);
    }
}
void Inventory_Unequip(CombatContext* combat, EquipSlot slot)
{
    // 1. Validation de l'emplacement (Safety check)
    if (slot < 0 || slot >= SLOT_HAND_2)
    {
        return;
    }

    // 2. Vérifier s'il y a effectivement quelque chose à déséquiper
    if (combat->player.equipped[slot] == -1)
    {
        // Déjà vide, rien à faire
        return;
    }

    // 3. Déséquipement
    // On remet l'index d'inventaire à -1 pour cet emplacement
    combat->player.equipped[slot] = -1;

    // 4. Mise à jour des stats
    // Crucial pour retirer les bonus de défense/attaque de l'objet
    Combat_RecalculateStats(combat);
}