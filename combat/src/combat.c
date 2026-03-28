#include "combat.h"
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "lang.h"
#include "audio_manager.h"
#include "math.h"
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

static void Combat_AddDamageText(CombatContext* combat, int x, int y, const char* text, Color color);


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
            combat->player.inventory[combat->player.inventory_count].level        = 0;                // Niveau de base
            combat->player.inventory[combat->player.inventory_count].effect       = ITEM_EFFECT_NONE; // Sécurité
            combat->player.inventory_count++;

            char log[64];
            sprintf(log, T("LOG_GAIN_ITEM"), g_isEnglish ? g_itemDB[i].name_en : g_itemDB[i].name_fr);
            Combat_AddLog(combat, log);
            return;
        }
    }
}

// fonction gatcha pour les coffres : ajoute un objet aléatoire de la base de données, avec un niveau et un effet aléatoires
void Inventory_AddLoot(CombatContext* combat, int template_idx, int level, ItemEffect effect, ItemRarity rarity)
{
    if (combat->player.inventory_count >= MAX_INVENTORY)
    {
        Combat_AddLog(combat, "Inventaire plein !");
        return;
    }
    OwnedItem* item    = &combat->player.inventory[combat->player.inventory_count];
    item->template_idx = template_idx;
    item->level        = level;
    item->effect       = effect;
    item->rarity       = rarity; // On sauvegarde la rareté
    combat->player.inventory_count++;

    ItemTemplate* t = &g_itemDB[template_idx];
    char          log[64];
    sprintf(log, "Loot : %s Niv %d", g_isEnglish ? t->name_en : t->name_fr, level);
    Combat_AddLog(combat, log);
}

void Combat_Init(CombatContext* combat)
{
    combat->player.inventory_count      = 0;
    combat->player.inventory_safe_count = 0;

    for(int i = 0; i < MAX_DMG_TEXTS; i++) combat->dmg_texts[i].active = false;

    for (int i = 0; i < MAX_SLOTS; i++)
    {
        combat->player.equipped[i] = -1;
    }

    combat->magic_proj.active = false;
    combat->magic_proj.exploding = false;

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
    // Si on avait équipé un objet trouvé dans le donjon avant de mourir, on le déséquipe ( à priori pas possible mais on sait jamais )!
    for (int i = 0; i < MAX_SLOTS; i++)
    {
        if (combat->player.equipped[i] >= combat->player.inventory_safe_count)
        {
            combat->player.equipped[i] = -1;
        }
    }

    combat->magic_proj.active = false;
    combat->magic_proj.exploding = false;

    for(int i = 0; i < MAX_DMG_TEXTS; i++) combat->dmg_texts[i].active = false;

    if (combat->current_enemy.sprite.id != 0)
    {
        UnloadTexture(combat->current_enemy.sprite);
        combat->current_enemy.sprite.id = 0; // Sécurité
    }

    combat->player.inventory_count = combat->player.inventory_safe_count;

    combat->player.level  = 1;
    combat->player.xp     = 0;
    combat->player.max_xp = 100;

    combat->player.max_hp   = combat->player.base_max_hp;
    combat->player.max_mana = combat->player.base_max_mana;
    combat->player.atk      = combat->player.base_atk;
    combat->player.spd      = combat->player.base_spd;

    combat->monsters_killed = 0;

    combat->is_active = false;
    Combat_AddLog(combat, T("NEW_RUN"));
    Combat_RecalculateStats(combat);
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
    printf("\n=== DEBUT START ENCOUNTER ===\n");
    printf("[ENCOUNTER] Etage du joueur : %d | Salle de Boss : %d\n", current_floor, is_boss_room);
    printf("[ENCOUNTER] Monstres dispo en DB : %d\n", g_monsterCount);

    combat->player_attack_timer = 0.0f;
    combat->enemy_attack_timer  = 0.0f;

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
            // LE PRINTF POUR COMPRENDRE LE REJET :
            if (!t->is_boss && current_floor >= t->min_floor && current_floor <= t->max_floor)
            {
                valid_candidates[candidate_count++] = t;
                printf("  -> [RETENU] %s (min: %d, max: %d)\n", t->id, t->min_floor, t->max_floor);
            }
        }
    }

    printf("[ENCOUNTER] Total candidats trouves : %d\n", candidate_count);

    if (candidate_count == 0)
    {
        printf("[ERREUR CRITIQUE] Aucun monstre valide pour l'etage %d !\n", current_floor);
        Combat_AddLog(combat, "Erreur : Aucun monstre a cet etage !");
        printf("=== FIN START ENCOUNTER (ECHEC) ===\n\n");
        return;
    }

    combat->is_active = true;

    MonsterTemplate* chosen = valid_candidates[GetRandomValue(0, candidate_count - 1)];
    printf("[ENCOUNTER] Monstre choisi : %s\n", chosen->id);

    strcpy(combat->current_enemy.name, g_isEnglish ? chosen->name_en : chosen->name_fr);
    strcpy(combat->current_enemy.flavor, g_isEnglish ? chosen->flavor_en : chosen->flavor_fr);

    combat->current_enemy.max_hp       = chosen->hp;
    combat->current_enemy.hp           = chosen->hp;
    combat->current_enemy.atk          = chosen->atk;
    combat->current_enemy.spd          = chosen->spd;
    combat->current_enemy.xp_yield     = chosen->xp;
    combat->current_enemy.base_color   = chosen->base_color;
    combat->current_enemy.is_boss      = chosen->is_boss;
    combat->current_enemy.sprite       = LoadTexture(chosen->image_path);
    combat->current_enemy.qte_active   = false;
    combat->current_enemy.qte_timer    = 2.0f;
    combat->current_enemy.poison_timer = 0.0f;
    combat->current_enemy.freeze_timer = 0.0f;
    combat->current_enemy.stun_timer   = 0.0f;

    Combat_AddLog(combat, combat->current_enemy.flavor);
    printf("=== FIN START ENCOUNTER (SUCCES) ===\n\n");
}
extern SpellTemplate  g_spellDB[MAX_SPELLS_DB];
extern PotionTemplate g_potionDB[MAX_POTIONS_DB];

void Combat_Update(CombatContext* combat, float deltaTime, int centerX, int centerY)
{
    static int debug_tick = 0;
    if (debug_tick < 3)
    {
        printf("[RADAR COMBAT] Update en cours ! is_active = %d | HP Ennemi = %d\n", combat->is_active, combat->current_enemy.hp);
        debug_tick++;
    }

    if (!combat->is_active)
        return;

    //  GESTION DES TEXTES FLOTTANTS 
    for (int i = 0; i < MAX_DMG_TEXTS; i++)
    {
        if (combat->dmg_texts[i].active)
        {
            combat->dmg_texts[i].timer -= deltaTime;
            combat->dmg_texts[i].y -= 40.0f * deltaTime; // Le texte monte de 40 pixels par seconde
            if (combat->dmg_texts[i].timer <= 0.0f)
            {
                combat->dmg_texts[i].active = false;
            }
        }
    }

    // --- GESTION DU PROJECTILE MAGIQUE ---
    if (combat->magic_proj.active) {
        if (!combat->magic_proj.exploding) {
            // La boule avance vers sa cible
            combat->magic_proj.progress += deltaTime / combat->magic_proj.speed;
            if (combat->magic_proj.progress >= 1.0f) {
                combat->magic_proj.progress = 1.0f;
                combat->magic_proj.exploding = true;
                combat->magic_proj.explosion_timer = combat->magic_proj.explosion_max_time;
                // On peut même jouer un son d'impact ici si tu en as un !
            }
            // Calcul de la position actuelle (Interpolation Linéaire)
            combat->magic_proj.current.x = combat->magic_proj.start.x + (combat->magic_proj.target.x - combat->magic_proj.start.x) * combat->magic_proj.progress;
            combat->magic_proj.current.y = combat->magic_proj.start.y + (combat->magic_proj.target.y - combat->magic_proj.start.y) * combat->magic_proj.progress;
        } else {
            // L'explosion se dissipe
            combat->magic_proj.explosion_timer -= deltaTime;
            if (combat->magic_proj.explosion_timer <= 0.0f) {
                combat->magic_proj.active = false;
            }
        }
    }

    if (combat->screen_flash_timer > 0.0f)
    {
        combat->screen_flash_timer -= deltaTime;
    }

    if (combat->screen_shake_timer > 0.0f) {
        combat->screen_shake_timer -= deltaTime;
    }

    if (combat->slash_timer > 0.0f)
    {
        combat->slash_timer -= deltaTime;
    }

    // --- 1. GESTION DES ALTÉRATIONS D'ÉTAT (Monstre) ---
    if (combat->current_enemy.poison_timer > 0)
    {
        combat->current_enemy.poison_timer -= deltaTime;
        combat->current_enemy.poison_tick -= deltaTime;
        if (combat->current_enemy.poison_tick <= 0)
        {
            combat->current_enemy.hp -= combat->current_enemy.poison_dmg;
            char dmgStr[32];
            sprintf(dmgStr, "-%d", combat->current_enemy.poison_dmg);
            Combat_AddDamageText(combat, centerX, centerY, dmgStr, LIME);
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
            Combat_TryUsePotion(combat, i);
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

                    // : CRÉATION DE LA BOULE
                    Color magicColors[] = { MAGENTA, BLUE, VIOLET, PURPLE, GOLD, PINK, LIME };
                    combat->magic_proj.active = true;
                    combat->magic_proj.exploding = false;
                    // La boule part du coin inférieur gauche (le joueur fictif)
                    combat->magic_proj.start = (Vector2){ centerX - 250, centerY + 150 }; 
                    combat->magic_proj.target = (Vector2){ centerX, centerY }; // Vise le monstre
                    combat->magic_proj.progress = 0.0f;
                    combat->magic_proj.speed = 0.15f; // Très rapide (150ms)
                    combat->magic_proj.explosion_max_time = 0.3f; // L'explosion dure 300ms
                    combat->magic_proj.color = magicColors[GetRandomValue(0, 6)];
                    int   lvl = combat->player.spell_level[s_idx];
                    int   val = t->base_val + (lvl * t->inc_val);
                    float dur = t->base_dur + (lvl * t->inc_dur);

                    if (t->type == SPELL_DAMAGE)
                    {
                        char dmgStr[32];
                        sprintf(dmgStr, "-%d", val);
                        Combat_AddDamageText(combat, centerX, centerY, dmgStr, BLUE);
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
        Audio_PlaySFX(SFX_ATTACK);

        // SYSTEME DE COUP CRITIQUE (15% de chance) ---
        bool is_crit = (GetRandomValue(1, 100) <= 15);
        int final_dmg = combat->player.atk;
        
        if (is_crit) {
            final_dmg *= 2; // Dégâts doublés !
            combat->screen_shake_timer = 0.2f;      // Tremblement de l'écran
            combat->screen_shake_magnitude = 8.0f;
        }

        combat->current_enemy.hp -= final_dmg;


        char dmgStr[32];
        if (is_crit) sprintf(dmgStr, "-%d CRIT!", final_dmg);
        else sprintf(dmgStr, "-%d", final_dmg);
        
        // Texte jaune si critique, blanc sinon
        Combat_AddDamageText(combat, centerX, centerY, dmgStr, is_crit ? YELLOW : WHITE);

        // --- DECLENCHEMENT DU SLASH NORMAL ---
        combat->slash_timer = 0.15f; // Durée très courte et nerveuse (0.15 sec)
        combat->slash_direction = GetRandomValue(0, 1); // 50/50
        combat->slash_color = is_crit ? ORANGE : WHITE;

        char log[64];
        if (is_crit) sprintf(log, "Coup Critique ! (%d degats)", final_dmg);
        else sprintf(log, T("LOG_HIT_ENEMY"), final_dmg);
        Combat_AddLog(combat, log);

        // Effets Magiques à l'impact
        if (combat->player.has_vamp_weapon)
        {
            combat->player.hp += 2;
            if (combat->player.hp > combat->player.max_hp)
                combat->player.hp = combat->player.max_hp;
        }
        if (combat->player.has_poison_weapon)
        {
            combat->current_enemy.poison_timer = 3.0f;
            combat->current_enemy.poison_dmg   = 2;
            combat->current_enemy.poison_tick  = 1.0f;
        }

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
            // --- NOUVEAU : SYSTEME D'ESQUIVE (10% de chance) ---
            bool is_dodge = (GetRandomValue(1, 100) <= 10);
            
            if (is_dodge) 
            {
                // Pas de dégâts, juste un texte flottant stylé !
                Combat_AddDamageText(combat, centerX - 250, centerY, "ESQUIVE !", LIGHTGRAY);
                Combat_AddLog(combat, "Vous avez esquive l'attaque !");
            } 
            else 
            {
                combat->player.hp -= combat->current_enemy.atk;
                char dmgStr[32];
                sprintf(dmgStr, "-%d", combat->current_enemy.atk);
                Combat_AddDamageText(combat, centerX - 250, centerY, dmgStr, RED);
                
                char log[64];
                sprintf(log, "%s frappe (%d degats)", combat->current_enemy.name, combat->current_enemy.atk);
                Combat_AddLog(combat, log);
            }
            
            combat->enemy_attack_timer -= 1.0f;
        }
    }

    // --- 6. GESTION DU POINT FAIBLE (QTE AU CLAVIER) ---
    combat->current_enemy.qte_timer -= deltaTime;
    if (combat->current_enemy.qte_timer <= 0.0f)
    {
        if (!combat->current_enemy.qte_active)
        {
            combat->current_enemy.qte_active = true;
            combat->current_enemy.qte_pos.x  = centerX - 50 + (GetRandomValue(0, 100));
            combat->current_enemy.qte_pos.y  = centerY - 50 + (GetRandomValue(0, 100));
            combat->current_enemy.qte_timer  = 1.5f; // Temps pour réagir

            // On choisit une touche au hasard parmi X, N, Y, P
            int keys[]                             = {KEY_X, KEY_N, KEY_Y, KEY_P};
            combat->current_enemy.qte_key_required = keys[GetRandomValue(0, 3)];
        }
        else
        {
            // Temps écoulé, le QTE disparait
            combat->current_enemy.qte_active = false;
            combat->current_enemy.qte_timer  = GetRandomValue(3, 6);
        }
    }

    // --- VERIFICATION DE LA TOUCHE ---
    if (combat->current_enemy.qte_active)
    {
        bool wrong_key_pressed = false;
        bool right_key_pressed = false;

        int possible_keys[] = {KEY_X, KEY_N, KEY_Y, KEY_P};

        // On vérifie si l'une des 4 touches a été pressée cette frame
        for (int i = 0; i < 4; i++)
        {
            if (IsKeyPressed(possible_keys[i]))
            {
                if (possible_keys[i] == combat->current_enemy.qte_key_required)
                {
                    right_key_pressed = true;
                }
                else
                {
                    wrong_key_pressed = true;
                }
            }
        }

        if (right_key_pressed)
        {
            Audio_PlaySFX(SFX_QTE_OK);
            int crit_dmg = combat->player.atk * 2;
            combat->current_enemy.hp -= crit_dmg;
            char dmgStr[32];
            sprintf(dmgStr, "-%d CRIT!", crit_dmg);
            Combat_AddDamageText(combat, centerX, centerY - 50, dmgStr, YELLOW);
            // --- DECLENCHEMENT DU SLASH CRITIQUE ---
            combat->slash_timer = 0.20f;
            combat->slash_direction = GetRandomValue(0, 1);
            combat->slash_color = YELLOW;
            // --- DECLENCHEMENT DU SCREEN SHAKE ---
            combat->screen_shake_timer = 0.25f;     // Dure un quart de seconde
            combat->screen_shake_magnitude = 15.0f; // Tremblement violent (15 pixels)


            combat->current_enemy.qte_active = false;
            combat->current_enemy.qte_timer  = GetRandomValue(3, 6);
            Combat_AddLog(combat, T("WEAK_POINT"));                  // "Point faible frappé !"
            combat->screen_flash_color = (Color){255, 255, 255, 80}; // Flash blanc de réussite
            combat->screen_flash_timer = 0.1f;
        }
        else if (wrong_key_pressed)
        {
            // PUNITIF : Mauvaise touche = QTE annulé !
            combat->current_enemy.qte_active = false;
            combat->current_enemy.qte_timer  = GetRandomValue(3, 6);
            Combat_AddLog(combat, T("WEAK_POINT_MISS"));
            Audio_PlaySFX(SFX_QTE_FAIL);
        }
    }

    // --- 7. CONDITIONS DE FIN DE COMBAT ---
    if (combat->current_enemy.hp <= 0)
    {
        combat->is_active = false;
        combat->monsters_killed++;
        Combat_AddLog(combat, T("ENEMY_DEFEATED"));
        if (combat->current_enemy.sprite.id != 0)
        {
            UnloadTexture(combat->current_enemy.sprite);
            combat->current_enemy.sprite.id = 0; // Sécurité
        }

        if (combat->current_enemy.is_boss)
        {
            combat->player.boss_souls++;
            Combat_AddLog(combat, "=> AME DE BOSS OBTENUE !");
        }

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
    if (!combat->is_active)
        return;

    // --- 1. DESSIN DU FLASH D'ÉCRAN ---
    if (combat->screen_flash_timer > 0.0f)
    {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), combat->screen_flash_color);
    }

    // --- 2. COULEUR DYNAMIQUE DU MONSTRE ---
    Color mColor = combat->current_enemy.base_color;

    if (combat->current_enemy.freeze_timer > 0)
        mColor = SKYBLUE;
    else if (combat->current_enemy.poison_timer > 0)
        mColor = LIME;
    else if (combat->current_enemy.stun_timer > 0)
        mColor = YELLOW;

    // ==========================================
    // APPLICATION DU SCREEN SHAKE 
    // ==========================================
    if (combat->screen_shake_timer > 0.0f) {
        // L'intensité diminue progressivement avec le temps pour un effet naturel
        float intensity = combat->screen_shake_magnitude * (combat->screen_shake_timer / 0.25f);
        
        // On modifie virtuellement le centre de l'écran pour cette frame !
        centerX += GetRandomValue(-(int)intensity, (int)intensity);
        centerY += GetRandomValue(-(int)intensity, (int)intensity);
    }

    // --- CALCUL AVEC MISE À L'ÉCHELLE ---
    float scale = 5.0f;

    int scaledWidth  = (int)(combat->current_enemy.sprite.width * scale);
    int scaledHeight = (int)(combat->current_enemy.sprite.height * scale);

    // On centre l'image parfaitement avec ses nouvelles dimensions
    int imgX = centerX - (scaledWidth / 2);
    int imgY = centerY - (scaledHeight / 2);

    // --- 3. TAGS DE STATUT AU-DESSUS DU MONSTRE ---
    int tagY = imgY - 30;

    if (combat->current_enemy.stun_timer > 0)
    {
        const char* txt   = g_isEnglish ? "[ STUNNED ]" : "[ ETOURDI ]";
        Vector2     tSize = MeasureTextEx(font, txt, 20, 1);
        DrawTextEx(font, txt, (Vector2){centerX - (tSize.x / 2), tagY}, 20, 1, YELLOW);
        tagY -= 25;
    }
    if (combat->current_enemy.freeze_timer > 0)
    {
        const char* txt   = g_isEnglish ? "[ FROZEN ]" : "[ GELE ]";
        Vector2     tSize = MeasureTextEx(font, txt, 20, 1);
        DrawTextEx(font, txt, (Vector2){centerX - (tSize.x / 2), tagY}, 20, 1, SKYBLUE);
        tagY -= 25;
    }
    if (combat->current_enemy.poison_timer > 0)
    {
        const char* txt   = g_isEnglish ? "[ POISONED ]" : "[ EMPOISONNE ]";
        Vector2     tSize = MeasureTextEx(font, txt, 20, 1);
        DrawTextEx(font, txt, (Vector2){centerX - (tSize.x / 2), tagY}, 20, 1, LIME);
        tagY -= 25;
    }

    // --- 4. DESSIN DU MONSTRE (AGRANDI) ---
    DrawTextureEx(combat->current_enemy.sprite, (Vector2){(float)imgX, (float)imgY}, 0.0f, scale, mColor);

    // ==========================================
    // DESSIN DU SLASH D'ÉPÉE 
    // ==========================================
    if (combat->slash_timer > 0.0f)
    {
        int slashSize = 250; // La taille de la balafre
        
        // On calcule un "progress" de 0.0 à 1.0 pour animer le mouvement
        float progress = 1.0f - (combat->slash_timer / 0.15f); 
        float offset = progress * 80; // La lame "glisse" de 80 pixels pendant l'animation
        
        Vector2 startPos, endPos;

        if (combat->slash_direction == 0) {
            // De Haut-Gauche vers Bas-Droite
            startPos = (Vector2){ centerX - slashSize/2 + offset, centerY - slashSize/2 + offset };
            endPos   = (Vector2){ centerX + slashSize/2 + offset, centerY + slashSize/2 + offset };
        } else {
            // De Haut-Droite vers Bas-Gauche
            startPos = (Vector2){ centerX + slashSize/2 - offset, centerY - slashSize/2 + offset };
            endPos   = (Vector2){ centerX - slashSize/2 - offset, centerY + slashSize/2 + offset };
        }

        // Plus le temps passe, plus la ligne devient fine et transparente
        float thickness = 20.0f * (combat->slash_timer / 0.15f);
        if (thickness < 1.0f) thickness = 1.0f; // Sécurité

        DrawLineEx(startPos, endPos, thickness, combat->slash_color);
    }
    // --- 5. QTE ET POINTS DE VIE ---
    if (combat->current_enemy.qte_active)
    {
        DrawRectangleLines(combat->current_enemy.qte_pos.x, combat->current_enemy.qte_pos.y, 40, 40, YELLOW);

        char qte_char = 'X';
        if (combat->current_enemy.qte_key_required == KEY_N)
            qte_char = 'N';
        else if (combat->current_enemy.qte_key_required == KEY_Y)
            qte_char = 'Y';
        else if (combat->current_enemy.qte_key_required == KEY_P)
            qte_char = 'P';

        char qte_str[8];
        sprintf(qte_str, "[%c]", qte_char);

        DrawTextEx(font, qte_str, (Vector2){combat->current_enemy.qte_pos.x + 5, combat->current_enemy.qte_pos.y + 10}, 24, 1, YELLOW);
    }

    char hpText[64];
    sprintf(hpText, "[ %s : %d / %d HP ]", combat->current_enemy.name, combat->current_enemy.hp, combat->current_enemy.max_hp);
    Vector2 tSize = MeasureTextEx(font, hpText, 24, 1);

    // Le texte des PV se place 10 pixels SOUS l'image agrandie
    int textY = imgY + scaledHeight + 40;
    DrawTextEx(font, hpText, (Vector2){centerX - (tSize.x / 2), textY}, 24, 1, RED);

    // ==========================================
    // --- 6. DESSIN DES DEGATS FLOTTANTS ---
    // ==========================================
    for (int i = 0; i < MAX_DMG_TEXTS; i++)
    {
        if (combat->dmg_texts[i].active)
        {
            // On calcule l'opacité (Alpha) pour créer un fondu sur la fin
            float alpha = combat->dmg_texts[i].timer / 1.0f; 
            Color fadeColor = combat->dmg_texts[i].color;
            fadeColor.a = (unsigned char)(255.0f * alpha);

            Vector2 tSize = MeasureTextEx(font, combat->dmg_texts[i].text, 24, 1);
            DrawTextEx(font, combat->dmg_texts[i].text, 
                      (Vector2){combat->dmg_texts[i].x - (tSize.x / 2), combat->dmg_texts[i].y}, 
                      30, 1, fadeColor); // Taille 30 pour être bien lisible
        }
    }

    // ==========================================
    // --- 7. DESSIN DE LA MAGIE ---
    // ==========================================
    if (combat->magic_proj.active)
    {
        if (!combat->magic_proj.exploding)
        {
            // Boule d'énergie principale
            DrawCircleV(combat->magic_proj.current, 15.0f, combat->magic_proj.color);
            
            // Traînée visuelle (un cercle plus petit et transparent derrière)
            Vector2 trailPos = { 
                combat->magic_proj.current.x - (combat->magic_proj.target.x - combat->magic_proj.start.x) * 0.05f, 
                combat->magic_proj.current.y - (combat->magic_proj.target.y - combat->magic_proj.start.y) * 0.05f 
            };
            Color trailColor = combat->magic_proj.color;
            trailColor.a = 120; // Semi-transparent
            DrawCircleV(trailPos, 10.0f, trailColor);
        }
        else
        {
            // L'Explosion finale !
            float life_ratio = combat->magic_proj.explosion_timer / combat->magic_proj.explosion_max_time; // Passe de 1.0 à 0.0
            
            // Le rayon s'agrandit brutalement de 20 à 100 pixels
            float radius = 20.0f + ((1.0f - life_ratio) * 80.0f); 
            
            Color expColor = combat->magic_proj.color;
            expColor.a = (unsigned char)(255 * life_ratio); // S'estompe progressivement
            
            // Dessin du noyau et de l'onde de choc
            DrawCircleV(combat->magic_proj.target, radius * 0.8f, expColor);
            DrawCircleLines(combat->magic_proj.target.x, combat->magic_proj.target.y, radius, expColor);
        }
    }

    // ==========================================
    // --- 8. EFFET DANGER DE MORT (HP FAIBLES) ---
    // ==========================================
    float hp_percent = (float)combat->player.hp / (float)combat->player.max_hp;
    
    // Si on a moins de 25% de vie et qu'on est pas encore mort
    if (hp_percent <= 0.25f && combat->player.hp > 0)
    {
        // On crée une pulsation agressive avec un sinus très rapide
        float pulse = (sinf(GetTime() * 15.0f) + 1.0f) / 2.0f; 
        
        // Un voile rouge translucide qui palpite sur TOUT l'écran
        Color dangerColor = (Color){ 200, 0, 0, (unsigned char)(60 * pulse) };
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), dangerColor);
        
        // Un texte d'alerte clignotant
        Color textDanger = (Color){ 255, 0, 0, (unsigned char)(255 * pulse) };
        Vector2 tSize = MeasureTextEx(font, "!!! SANTE CRITIQUE !!!", 30, 1);
        DrawTextEx(font, "!!! SANTE CRITIQUE !!!", (Vector2){centerX - (tSize.x / 2), GetScreenHeight() - 150}, 30, 1, textDanger);
    }
}

void Combat_RecalculateStats(CombatContext* combat)
{
    combat->player.base_max_hp   = 50;
    combat->player.base_atk      = 5;
    combat->player.base_max_mana = 20;
    combat->player.base_spd      = 0.8f;
    combat->player.fog_bonus     = 0;

    combat->player.has_vamp_weapon   = false;
    combat->player.has_poison_weapon = false;

    for (int i = 0; i < MAX_SLOTS; i++)
    {
        int inv_idx = combat->player.equipped[i];
        if (inv_idx != -1)
        {
            OwnedItem*    item = &combat->player.inventory[inv_idx];
            ItemTemplate* t    = &g_itemDB[item->template_idx];

            // --- NOUVEAU : Calcul du multiplicateur de Rareté ---
            float r_mult = 1.0f;
            if (item->rarity == RARITY_RARE)
                r_mult = 1.2f;
            else if (item->rarity == RARITY_EPIC)
                r_mult = 1.5f;
            else if (item->rarity == RARITY_LEGENDARY)
                r_mult = 2.0f;

            // On applique le multiplicateur aux stats de l'objet !
            combat->player.base_max_hp += (int)((t->hp + (item->level * t->inc_hp)) * r_mult);
            combat->player.base_atk += (int)((t->atk + (item->level * t->inc_atk)) * r_mult);
            combat->player.base_max_mana += (int)((t->mana + (item->level * t->inc_mana)) * r_mult);
            combat->player.base_spd += (t->spd + (item->level * t->inc_spd)); // La vitesse n'est pas multipliée pour éviter les abus
            combat->player.fog_bonus += t->fog + (item->level * t->inc_fog);  // Le fog non plus

            if (item->effect == ITEM_EFFECT_FIRE)
                combat->player.base_atk += 5;
            if (item->effect == ITEM_EFFECT_SPEED)
                combat->player.base_spd += 0.3f;
            if (item->effect == ITEM_EFFECT_VAMP)
                combat->player.has_vamp_weapon = true;
            if (item->effect == ITEM_EFFECT_POISON)
                combat->player.has_poison_weapon = true;
        }
    }

    // Stats de renaissance
    float hp_mult   = 1.0f + (combat->player.passive_hp_level * 0.10f);   // +10% par niveau
    float atk_mult  = 1.0f + (combat->player.passive_atk_level * 0.10f);  // +10% par niveau1
    float mana_mult = 1.0f + (combat->player.passive_mana_level * 0.10f); // +10% par niveau1

    combat->player.base_max_hp   = (int)(combat->player.base_max_hp * hp_mult);
    combat->player.base_atk      = (int)(combat->player.base_atk * atk_mult);
    combat->player.base_max_mana = (int)(combat->player.base_max_mana * mana_mult);

    combat->player.max_hp   = combat->player.base_max_hp;
    combat->player.max_mana = combat->player.base_max_mana;
    combat->player.atk      = combat->player.base_atk;
    combat->player.spd      = combat->player.base_spd;

    // --- MALUS DE FROID SUR L'ATTAQUE ---
    if (combat->player.is_freezing)
    {
        combat->player.base_atk /= 2;
        // On s'assure qu'il fait au moins 1 de dégât pour ne pas soft-lock le combat
        if (combat->player.base_atk <= 0)
            combat->player.base_atk = 1;
    }

    combat->player.atk      = combat->player.base_atk;
    combat->player.max_hp   = combat->player.base_max_hp;
    combat->player.max_mana = combat->player.base_max_mana;
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

void Combat_TryUsePotion(CombatContext* combat, int slot_index)
{
    int p_idx = combat->player.equipped_potions[slot_index];
    if (p_idx != -1 && combat->player.potion_qty[p_idx] > 0)
    {
        Audio_PlaySFX(SFX_POTION);
        combat->player.potion_qty[p_idx]--;
        PotionTemplate* t   = &g_potionDB[p_idx];
        int             val = t->base_val + (combat->player.potion_level[p_idx] * t->inc_val);

        if (t->type == SPELL_HEAL)
        {
            combat->player.hp += val;
            combat->screen_flash_color = (Color){255, 0, 0, 60}; // Flash Rouge
        }
        else if(t->type == SPELL_MANA)
        {
            combat->player.mana += val;
            combat->screen_flash_color = (Color){0, 150, 255, 60}; // Flash Bleu
        }
        else
        {
            combat->screen_flash_color = GRAY;
        }

        combat->screen_flash_timer = 0.15f; // Durée du flash

        // Sécurité pour ne pas dépasser le max
        if (combat->player.hp > combat->player.max_hp)
            combat->player.hp = combat->player.max_hp;
        if (combat->player.mana > combat->player.max_mana)
            combat->player.mana = combat->player.max_mana;

        // Mise à jour du journal
        char log[64];
        sprintf(log, "> %s : %s", T("WORD_UTILISE"), g_isEnglish ? t->name_en : t->name_fr);
        Combat_AddLog(combat, log);
    }
}

// Cherche une case libre et crée un texte flottant
static void Combat_AddDamageText(CombatContext* combat, int x, int y, const char* text, Color color)
{
    for (int i = 0; i < MAX_DMG_TEXTS; i++)
    {
        if (!combat->dmg_texts[i].active)
        {
            // On ajoute un petit décalage aléatoire pour que les textes ne se superposent pas
            combat->dmg_texts[i].x = x + GetRandomValue(-20, 20);
            combat->dmg_texts[i].y = y + GetRandomValue(-10, 10);
            strcpy(combat->dmg_texts[i].text, text);
            combat->dmg_texts[i].color = color;
            combat->dmg_texts[i].timer = 1.0f; // Disparaît après 1 seconde
            combat->dmg_texts[i].active = true;
            break;
        }
    }
}