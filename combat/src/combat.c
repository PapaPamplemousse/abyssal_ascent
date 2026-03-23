#include "combat.h"
#include <stdio.h>
#include <string.h>

void Combat_RecalculateStats(CombatContext* combat) {
    // 1. Stats de base initiales (LA VITESSE EST DE RETOUR !)
    combat->player.base_max_hp = 50;
    combat->player.base_atk = 5;
    combat->player.base_max_mana = 20;
    combat->player.base_spd = 0.8f; // <--- C'était ça le coupable

    // 2. Application des bonus d'équipement (Forge)
    combat->player.base_atk += (combat->player.eq_epee * 3) + (combat->player.eq_gants * 1);
    combat->player.base_max_hp += (combat->player.eq_armure * 15) + (combat->player.eq_casque * 10) + (combat->player.eq_jambieres * 10);
    
    // 3. On met à jour les stats du combat ACTUEL avec nos nouveaux calculs
    combat->player.max_hp = combat->player.base_max_hp;
    combat->player.max_mana = combat->player.base_max_mana;
    combat->player.atk = combat->player.base_atk;
    combat->player.spd = combat->player.base_spd;
}

void Combat_Init(CombatContext* combat) {
    // Initialisation de l'équipement
    combat->player.eq_epee = 0;
    combat->player.eq_armure = 0;
    combat->player.eq_casque = 0;
    combat->player.eq_jambieres = 0;
    combat->player.eq_gants = 0;
    
    combat->player.spell_fireball = false;
    combat->player.spell_heal = false;
    
    combat->player.potions_hp = 0;
    combat->player.potions_mana = 0;

    Combat_RecalculateStats(combat);

    combat->is_active = false;
    for(int i=0; i<5; i++) strcpy(combat->battle_log[i], "");
    combat->log_index = 0;

    Combat_ResetRun(combat);
    combat->player.hp = combat->player.max_hp; 
    combat->player.mana = combat->player.max_mana; 
}

void Combat_ResetRun(CombatContext* combat) {
    combat->player.level = 1;
    combat->player.xp = 0;
    combat->player.max_xp = 100;
    
    combat->player.max_hp = combat->player.base_max_hp;
    combat->player.max_mana = combat->player.base_max_mana;
    combat->player.atk = combat->player.base_atk;
    combat->player.spd = combat->player.base_spd;
    
    combat->is_active = false;
    Combat_AddLog(combat, "--- NOUVELLE RUN ---");
}

void Combat_AddLog(CombatContext* combat, const char* msg) {
    // Fait remonter les anciens messages
    for (int i = 4; i > 0; i--) {
        strcpy(combat->battle_log[i], combat->battle_log[i-1]);
    }
    strcpy(combat->battle_log[0], msg);
}

void Combat_StartEncounter(CombatContext* combat, MonsterType type) {
    combat->is_active = true;
    combat->player_attack_timer = 0.0f;
    combat->enemy_attack_timer = 0.0f;
    
    combat->current_enemy.qte_active = false;
    combat->current_enemy.qte_timer = 2.0f; // Le point faible apparait dans 2 sec

    if (type == MONSTER_RAT) {
        strcpy(combat->current_enemy.name, "Rat Geant");
        combat->current_enemy.max_hp = 20;
        combat->current_enemy.atk = 2;
        combat->current_enemy.spd = 1.2f; // Rapide
        combat->current_enemy.xp_yield = 15;
    } 
    else if (type == MONSTER_SKELETON) {
        strcpy(combat->current_enemy.name, "Squelette");
        combat->current_enemy.max_hp = 45;
        combat->current_enemy.atk = 6;
        combat->current_enemy.spd = 0.6f; // Lent
        combat->current_enemy.xp_yield = 35;
    }
    else if (type == MONSTER_ZOMBIE) {
        strcpy(combat->current_enemy.name, "Zombie Putride");
        combat->current_enemy.max_hp = 60;
        combat->current_enemy.atk = 8;
        combat->current_enemy.spd = 0.4f;
        combat->current_enemy.xp_yield = 50;
    }
    else if (type == MONSTER_BOSS_SKELETON_KING) { 
        strcpy(combat->current_enemy.name, "ROI SQUELETTE");
        combat->current_enemy.max_hp = 250;
        combat->current_enemy.atk = 15;
        combat->current_enemy.spd = 0.7f;
        combat->current_enemy.xp_yield = 300;
    }
    
    combat->current_enemy.hp = combat->current_enemy.max_hp;
    Combat_AddLog(combat, "Un monstre apparait !");
}

void Combat_Update(CombatContext* combat, float deltaTime, int centerX, int centerY) {
    if (!combat->is_active) return;

    // --- 1. ACTIONS MANUELLES (Consommables et Sorts) ---
    if (IsKeyPressed(KEY_H) && combat->player.potions_hp > 0) {
        combat->player.potions_hp--;
        combat->player.hp += 30; 
        if (combat->player.hp > combat->player.max_hp) combat->player.hp = combat->player.max_hp;
        Combat_AddLog(combat, "> Vous buvez une Potion de Soin !");
    }
    
    if (IsKeyPressed(KEY_M) && combat->player.potions_mana > 0) {
        combat->player.potions_mana--;
        combat->player.mana += 20; 
        if (combat->player.mana > combat->player.max_mana) combat->player.mana = combat->player.max_mana;
        Combat_AddLog(combat, "> Vous buvez une Potion de Mana !");
    }

    if (IsKeyPressed(KEY_F) && combat->player.spell_fireball && combat->player.mana >= 10) {
        combat->player.mana -= 10;
        int damage = 25 + (combat->player.atk); 
        combat->current_enemy.hp -= damage;
        char log[64];
        sprintf(log, "> BOULE DE FEU ! (%d degats)", damage);
        Combat_AddLog(combat, log);
    }

    if (IsKeyPressed(KEY_S) && combat->player.spell_heal && combat->player.mana >= 15) {
        combat->player.mana -= 15;
        combat->player.hp += 40;
        if (combat->player.hp > combat->player.max_hp) combat->player.hp = combat->player.max_hp;
        Combat_AddLog(combat, "> Vous lancez SOIN !");
    }

    // --- 2. AUTO-ATTAQUE DU JOUEUR ---
    combat->player_attack_timer += deltaTime * combat->player.spd;
    if (combat->player_attack_timer >= 1.0f) {
        combat->current_enemy.hp -= combat->player.atk;
        char log[64];
        sprintf(log, "Vous frappez (%d degats)", combat->player.atk);
        Combat_AddLog(combat, log);
        combat->player_attack_timer -= 1.0f; // On reset le timer !
    }

    // --- 3. AUTO-ATTAQUE DE L'ENNEMI ---
    combat->enemy_attack_timer += deltaTime * combat->current_enemy.spd;
    if (combat->enemy_attack_timer >= 1.0f) {
        combat->player.hp -= combat->current_enemy.atk;
        char log[64];
        sprintf(log, "%s frappe (%d degats)", combat->current_enemy.name, combat->current_enemy.atk);
        Combat_AddLog(combat, log);
        combat->enemy_attack_timer -= 1.0f;
    }

    // --- 4. GESTION DU POINT FAIBLE (QTE) ---
    combat->current_enemy.qte_timer -= deltaTime;
    if (combat->current_enemy.qte_timer <= 0.0f) {
        if (!combat->current_enemy.qte_active) {
            combat->current_enemy.qte_active = true;
            combat->current_enemy.qte_pos.x = centerX - 50 + (GetRandomValue(0, 100));
            combat->current_enemy.qte_pos.y = centerY - 50 + (GetRandomValue(0, 100));
            combat->current_enemy.qte_timer = 1.5f;
        } else {
            combat->current_enemy.qte_active = false;
            combat->current_enemy.qte_timer = GetRandomValue(3, 6); 
        }
    }

    if (combat->current_enemy.qte_active) {
        Rectangle qte_rect = { combat->current_enemy.qte_pos.x, combat->current_enemy.qte_pos.y, 40, 40 };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), qte_rect)) {
            int crit_dmg = combat->player.atk * 2;
            combat->current_enemy.hp -= crit_dmg;
            combat->current_enemy.qte_active = false;
            combat->current_enemy.qte_timer = GetRandomValue(3, 6);
            Combat_AddLog(combat, "*** POINT FAIBLE ! ***");
        }
    }

    // --- 5. CONDITIONS DE FIN DE COMBAT ---
    if (combat->current_enemy.hp <= 0) {
        combat->is_active = false;
        Combat_AddLog(combat, "Ennemi vaincu !");
        
        combat->player.xp += combat->current_enemy.xp_yield;
        if (combat->player.xp >= combat->player.max_xp) {
            combat->player.level++;
            combat->player.xp -= combat->player.max_xp;
            combat->player.max_xp = (int)(combat->player.max_xp * 1.5);
            combat->player.max_hp += 10;
            combat->player.hp = combat->player.max_hp; 
            combat->player.atk += 2;
            Combat_AddLog(combat, "NIVEAU SUPERIEUR !");
        }
    }
}

// void Combat_Update(CombatContext* combat, float deltaTime, int centerX, int centerY) {
//     if (!combat->is_active) return;

//     if (IsKeyPressed(KEY_H) && combat->player.potions_hp > 0) {
//         combat->player.potions_hp--;
//         combat->player.hp += 30; // Rend 30 HP
//         if (combat->player.hp > combat->player.max_hp) combat->player.hp = combat->player.max_hp;
//         Combat_AddLog(combat, "> Vous buvez une Potion de Soin !");
//     }
    
//     if (IsKeyPressed(KEY_M) && combat->player.potions_mana > 0) {
//         combat->player.potions_mana--;
//         combat->player.mana += 20; // Rend 20 Mana
//         if (combat->player.mana > combat->player.max_mana) combat->player.mana = combat->player.max_mana;
//         Combat_AddLog(combat, "> Vous buvez une Potion de Mana !");
//     }

//     if (IsKeyPressed(KEY_F) && combat->player.spell_fireball && combat->player.mana >= 10) {
//         combat->player.mana -= 10;
//         int damage = 25 + (combat->player.atk); // Le sort scale avec l'attaque
//         combat->current_enemy.hp -= damage;
//         char log[64];
//         sprintf(log, "> BOULE DE FEU ! (%d degats)", damage);
//         Combat_AddLog(combat, log);
//     }

//     if (IsKeyPressed(KEY_S) && combat->player.spell_heal && combat->player.mana >= 15) {
//         combat->player.mana -= 15;
//         combat->player.hp += 40;
//         if (combat->player.hp > combat->player.max_hp) combat->player.hp = combat->player.max_hp;
//         Combat_AddLog(combat, "> Vous lancez SOIN !");
//     }

//     // 1. Auto-attaque du Joueur
//     combat->player_attack_timer += deltaTime * combat->player.spd;
//     if (combat->player_attack_timer >= 1.0f) {
//         combat->current_enemy.hp -= combat->player.atk;
//         char log[64];
//         sprintf(log, "Vous frappez (%d degats)", combat->player.atk);
//         Combat_AddLog(combat, log);
//         combat->player_attack_timer -= 1.0f;
//     }

//     // 2. Auto-attaque de l'Ennemi
//     combat->enemy_attack_timer += deltaTime * combat->current_enemy.spd;
//     if (combat->enemy_attack_timer >= 1.0f) {
//         combat->player.hp -= combat->current_enemy.atk;
//         char log[64];
//         sprintf(log, "%s frappe (%d degats)", combat->current_enemy.name, combat->current_enemy.atk);
//         Combat_AddLog(combat, log);
//         combat->enemy_attack_timer -= 1.0f;
//     }

//     // 3. Gestion du Point Faible (QTE)
//     combat->current_enemy.qte_timer -= deltaTime;
//     if (combat->current_enemy.qte_timer <= 0.0f) {
//         if (!combat->current_enemy.qte_active) {
//             // Fait apparaitre le point faible à une position aléatoire autour du centre
//             combat->current_enemy.qte_active = true;
//             combat->current_enemy.qte_pos.x = centerX - 50 + (GetRandomValue(0, 100));
//             combat->current_enemy.qte_pos.y = centerY - 50 + (GetRandomValue(0, 100));
//             combat->current_enemy.qte_timer = 1.5f; // Reste actif 1.5 secondes
//         } else {
//             // Le point faible disparait
//             combat->current_enemy.qte_active = false;
//             combat->current_enemy.qte_timer = GetRandomValue(3, 6); // Revient dans 3 à 6 secondes
//         }
//     }

//     // Clic sur le point faible
//     if (combat->current_enemy.qte_active) {
//         Rectangle qte_rect = { combat->current_enemy.qte_pos.x, combat->current_enemy.qte_pos.y, 40, 40 };
//         if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), qte_rect)) {
//             // Attaque critique bonus !
//             int crit_dmg = combat->player.atk * 2;
//             combat->current_enemy.hp -= crit_dmg;
//             combat->current_enemy.qte_active = false;
//             combat->current_enemy.qte_timer = GetRandomValue(3, 6);
//             Combat_AddLog(combat, "*** POINT FAIBLE ! ***");
//         }
//     }

//     // 4. Conditions de fin de combat
//     if (combat->current_enemy.hp <= 0) {
//         combat->is_active = false;
//         Combat_AddLog(combat, "Ennemi vaincu !");
        
//         // Gain d'XP
//         combat->player.xp += combat->current_enemy.xp_yield;
//         if (combat->player.xp >= combat->player.max_xp) {
//             // Level Up !
//             combat->player.level++;
//             combat->player.xp -= combat->player.max_xp;
//             combat->player.max_xp = (int)(combat->player.max_xp * 1.5);
//             combat->player.max_hp += 10;
//             combat->player.hp = combat->player.max_hp; // Soin total
//             combat->player.atk += 2;
//             Combat_AddLog(combat, "NIVEAU SUPERIEUR !");
//         }
//     }
// }

void Combat_RenderCenter(CombatContext* combat, Font font, int centerX, int centerY) {
    if (!combat->is_active) return;

    // Nom et HP de l'ennemi
    char hpText[64];
    sprintf(hpText, "[ %s : %d / %d HP ]", combat->current_enemy.name, combat->current_enemy.hp, combat->current_enemy.max_hp);
    
    // Fonction magique pour centrer (on l'avait créée avant)
    Vector2 tSize = MeasureTextEx(font, hpText, 24, 1);
    DrawTextEx(font, hpText, (Vector2){centerX - (tSize.x/2), centerY - 150}, 24, 1, RED);

    // Dessin du monstre ASCII (on simplifie pour l'exemple)
    Color mColor = LIGHTGRAY;
    if (strcmp(combat->current_enemy.name, "Squelette") == 0) {
        DrawTextEx(font, "   .-.   ", (Vector2){centerX - 40, centerY - 80}, 24, 1, mColor);
        DrawTextEx(font, "  (o.o)  ", (Vector2){centerX - 40, centerY - 60}, 24, 1, mColor);
        DrawTextEx(font, "  | O |  ", (Vector2){centerX - 40, centerY - 40}, 24, 1, mColor);
        DrawTextEx(font, "  |   |  ", (Vector2){centerX - 40, centerY - 20}, 24, 1, mColor);
        DrawTextEx(font, " '~~~' ", (Vector2){centerX - 40, centerY}, 24, 1, mColor);
    } else {
        DrawTextEx(font, "   /\\_\\  ", (Vector2){centerX - 40, centerY - 50}, 24, 1, mColor);
        DrawTextEx(font, "  ( o.o) ", (Vector2){centerX - 40, centerY - 30}, 24, 1, mColor);
        DrawTextEx(font, "   > ^ < ", (Vector2){centerX - 40, centerY - 10}, 24, 1, mColor);
    }

    // Dessin du Point Faible [X]
    if (combat->current_enemy.qte_active) {
        DrawRectangleLines(combat->current_enemy.qte_pos.x, combat->current_enemy.qte_pos.y, 40, 40, YELLOW);
        DrawTextEx(font, "[X]", (Vector2){combat->current_enemy.qte_pos.x + 5, combat->current_enemy.qte_pos.y + 10}, 24, 1, YELLOW);
    }
}