#ifndef COMBAT_H
#define COMBAT_H

#include "raylib.h"
#include <stdbool.h>

typedef enum
{
    SLOT_HELMET   = 0,
    SLOT_ARMOR    = 1,
    SLOT_GLOVES   = 2,
    SLOT_LEGGINGS = 3,
    SLOT_BOOTS    = 4,
    SLOT_HAND_1   = 5,
    SLOT_HAND_2   = 6,
    SLOT_NONE     = -1
} EquipSlot;

#ifndef MAX_ASCII_LINES
#define MAX_ASCII_LINES 60
#endif
#define MAX_MONSTERS_DB 150

#define MAX_ITEMS_DB 150
#define MAX_INVENTORY 50
#define MAX_ITEM_ASCII_LINES 20
#define MAX_SLOTS (SLOT_HAND_2 + 1)

#define MAX_SPELLS_DB 20
#define MAX_POTIONS_DB 20

typedef enum { SPELL_DAMAGE, SPELL_HEAL, SPELL_POISON, SPELL_FREEZE, SPELL_VAMPIRISM, SPELL_STUN } EffectType;

typedef enum {
    ITEM_EFFECT_NONE = 0,
    ITEM_EFFECT_FIRE = 1,   // +5 ATK
    ITEM_EFFECT_POISON = 2, // Applique du poison à l'ennemi
    ITEM_EFFECT_VAMP = 3,   // Soigne de 2 HP par frappe
    ITEM_EFFECT_SPEED = 4   // +0.3 Vitesse d'attaque
} ItemEffect;

typedef struct {
    char id[32]; char name_en[32]; char name_fr[32];
    EffectType type;
    int base_val, inc_val; float base_dur, inc_dur;
    int mana_cost;
    int learn_gold, learn_crystal;
    int upg_gold_base, upg_gold_inc;
    int prep_crystal; // Coût pour l'équiper
} SpellTemplate;

typedef struct {
    char id[32]; char name_en[32]; char name_fr[32];
    EffectType type;
    int base_val, inc_val;
    int learn_gold;
    int upg_gold_base, upg_gold_inc;
    int craft_herbs_base, craft_herbs_inc;
} PotionTemplate;

// Structure chargée depuis items.json
typedef struct
{
    char id[32];
    char name_en[32];
    char name_fr[32];
    char type[32]; // "HELMET", "ARMOR", "HAND_1", "HAND_2", "HAND_2H", etc.

    int   hp, atk, mana, fog;
    float spd; // Base
    int   inc_hp, inc_atk, inc_mana, inc_fog;
    float inc_spd; // Par niveau

    // Coûts d'amélioration { base, incrément }
    int cost_fer_base, cost_fer_inc;
    int cost_bois_base, cost_bois_inc;
    int cost_or_base, cost_or_inc;
    int cost_viande_base, cost_viande_inc;

    char ascii[MAX_ITEM_ASCII_LINES][128];
    int  ascii_line_count;

} ItemTemplate;

typedef enum {
    RARITY_COMMON = 0,   // Blanc (x1.0 stats & coût)
    RARITY_RARE = 1,     // Bleu (x1.2 stats & coût)
    RARITY_EPIC = 2,     // Violet (x1.5 stats & coût)
    RARITY_LEGENDARY = 3 // Orange (x2.0 stats & coût)
} ItemRarity;

// L'objet physique dans l'inventaire du joueur
typedef struct
{
    int template_idx; // L'index dans la base de données
    int level;        // Niveau actuel de l'objet (0 = base)
    ItemEffect effect; // Effet magique
    ItemRarity rarity; // La Rareté !
} OwnedItem;

// Statistiques de combat du joueur
typedef struct
{
    int   level;
    int   xp, max_xp;
    int   hp, max_hp;
    int   mana, max_mana;
    int   atk;
    float spd;
    // Stats permanentes
    int   base_max_hp;
    int   base_max_mana;
    int   base_atk;
    float base_spd;

    int boss_souls;         // Monnaie persistante
    int passive_hp_level;   // +10% HP par niveau
    int passive_atk_level;  // +10% ATK par niveau
    int passive_mana_level; // +10% Mana par niveau
    int passive_loot_level; // +2% chance de rareté par niveau

    // -Niveaux d'équipement ---
    OwnedItem inventory[MAX_INVENTORY];
    int       inventory_count;

    // --- NOUVEAU : SAUVEGARDE DU LOOT ---
    int       inventory_safe_count; 
    bool      has_vamp_weapon;
    bool      has_poison_weapon;

    // Contient l'index de l'objet dans 'inventory', ou -1 si vide
    int equipped[MAX_SLOTS];

    int acquisition_order[MAX_INVENTORY];

    int fog_bonus; // Vision supplémentaire calculée

    // Sorts débloqués ---
    bool spell_unlocked[MAX_SPELLS_DB];
    int spell_level[MAX_SPELLS_DB];
    int equipped_spells[3]; // Contient l'ID, -1 si vide
    
    bool potion_unlocked[MAX_POTIONS_DB];
    int potion_level[MAX_POTIONS_DB];
    int potion_qty[MAX_POTIONS_DB];
    int equipped_potions[3];

    bool is_freezing;
} PlayerStats;

typedef struct
{
    char id[32];
    char name_en[32];
    char name_fr[32];
    char flavor_en[128];
    char flavor_fr[128];

    bool is_boss;
    int  min_floor, max_floor;
    int  boss_floor;

    int   hp, atk, xp;
    float spd;

    Color base_color;

    char image_path[128];
} MonsterTemplate;

// Structure d'un ennemi
typedef struct
{
    char  name[32];
    char  flavor[128];
    int   hp, max_hp;
    int   atk;
    float spd;
    int   xp_yield;

    bool  is_boss;

    Color base_color;

    Texture2D sprite;

    float poison_timer;
    int poison_dmg; float poison_tick; // timer interne pour faire des dégâts chaque seconde

    float freeze_timer; float freeze_slow_factor; // ex: 30 = -30% de vitesse
    float stun_timer;

    bool    qte_active;
    float   qte_timer;
    int     qte_key_required;
    Vector2 qte_pos;
} Enemy;

// Contexte global du combat
typedef struct
{
    PlayerStats player;
    Enemy       current_enemy;
    bool        is_active;

    // Timers d'auto-battle (quand ils atteignent 1.0, le personnage attaque)
    float player_attack_timer;
    float enemy_attack_timer;

    // Journal de combat (historique des 5 dernières actions)
    char battle_log[5][64];
    int  log_index;

    //Effets Visuels
    Color screen_flash_color;
    float screen_flash_timer;

    // Effets d'épée 
    float slash_timer;     // Durée de l'animation du coup
    int   slash_direction; // 0 = Gauche à Droite, 1 = Droite à Gauche
    Color slash_color;     // Blanc (Normal) ou Jaune (QTE)

    //Screen Shake
    float screen_shake_timer;
    float screen_shake_magnitude;

    /* number of monster killed */
    int monsters_killed;
} CombatContext;

extern ItemTemplate g_itemDB[MAX_ITEMS_DB];
extern int g_itemCount;

extern SpellTemplate g_spellDB[MAX_SPELLS_DB];
extern int g_spellCount;

extern PotionTemplate g_potionDB[MAX_POTIONS_DB];
extern int g_potionCount;

extern MonsterTemplate g_monsterDB[MAX_MONSTERS_DB];
extern int             g_monsterCount;

void Combat_Init(CombatContext* combat);
void Combat_StartEncounter(CombatContext* combat, int current_floor, bool is_boss_room);
void Combat_Update(CombatContext* combat, float deltaTime, int centerX, int centerY);
void Combat_RenderCenter(CombatContext* combat, Font font, int centerX, int centerY);
void Combat_AddLog(CombatContext* combat, const char* msg);
void Combat_ResetRun(CombatContext* combat);
void Combat_RecalculateStats(CombatContext* combat);
void Inventory_Add(CombatContext* combat, const char* item_id);
void Inventory_Equip(CombatContext* combat, int inv_idx);
void Combat_TryUsePotion(CombatContext* combat, int slot_index);
void Inventory_GetSortedIndices(CombatContext* combat, int* indices);
void Inventory_AddLoot(CombatContext* combat, int template_idx, int level, ItemEffect effect, ItemRarity rarity);
#endif // COMBAT_H