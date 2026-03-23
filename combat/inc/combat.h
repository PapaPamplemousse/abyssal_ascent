#ifndef COMBAT_H
#define COMBAT_H

#include "raylib.h"
#include <stdbool.h>

// Statistiques de combat du joueur
typedef struct {
    int level;
    int xp, max_xp;
    int hp, max_hp;
    int mana, max_mana;
    int atk;
    float spd;
    int potions_hp;
    int potions_mana;
    
    // Stats permanentes
    int base_max_hp;
    int base_max_mana;
    int base_atk;
    float base_spd;

    // -Niveaux d'équipement ---
    int eq_epee;
    int eq_armure;
    int eq_casque;
    int eq_jambieres;
    int eq_gants;

    // Sorts débloqués ---
    bool spell_fireball;
    bool spell_heal;
} PlayerStats;



// Types de monstres
typedef enum { MONSTER_RAT, MONSTER_SKELETON, MONSTER_ZOMBIE, MONSTER_BOSS_SKELETON_KING } MonsterType;

// Structure d'un ennemi
typedef struct {
    char name[32];
    int hp, max_hp;
    int atk;
    float spd;
    int xp_yield; // XP donné à la mort
    
    // Pour le QTE (Point Faible)
    bool qte_active;
    float qte_timer;
    Vector2 qte_pos; // Position du [X] à l'écran
} Enemy;

// Contexte global du combat
typedef struct {
    PlayerStats player;
    Enemy current_enemy;
    bool is_active;
    
    // Timers d'auto-battle (quand ils atteignent 1.0, le personnage attaque)
    float player_attack_timer;
    float enemy_attack_timer;
    
    // Journal de combat (historique des 5 dernières actions)
    char battle_log[5][64];
    int log_index;
} CombatContext;

void Combat_Init(CombatContext* combat);
void Combat_StartEncounter(CombatContext* combat, MonsterType type);
void Combat_Update(CombatContext* combat, float deltaTime, int centerX, int centerY);
void Combat_RenderCenter(CombatContext* combat, Font font, int centerX, int centerY);
void Combat_AddLog(CombatContext* combat, const char* msg);
void Combat_ResetRun(CombatContext* combat);
void Combat_RecalculateStats(CombatContext* combat); 
#endif // COMBAT_H