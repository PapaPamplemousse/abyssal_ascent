#ifndef CLICKER_H
#define CLICKER_H

#include "raylib.h"

typedef struct
{
    // Ressources brutes
    int herbes, fer, viande, or, bois, cristaux;

    // Déblocages (Fer et Herbes sont débloqués par défaut)
    bool unlock_or;
    bool unlock_bois;
    bool unlock_cristaux;
    bool unlock_viande;

    // Tableaux des auto-clickers (4 Tiers par ressource)
    // [0] = +1/s, [1] = +10/s, [2] = +100/s, [3] = +1000/s
    int b_fer[4];
    int b_or[4];
    int b_cristaux[4];
    
    int b_herbes[4];
    int b_bois[4];
    int b_viande[4];

} PlayerResources;

typedef struct
{
    PlayerResources inventory;
    float           autoTimer; 
} ClickerContext;

void Clicker_Init(ClickerContext* clicker);
void Clicker_ProcessAuto(ClickerContext* clicker, float deltaTime);

void Clicker_UpdateMine(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font);
void Clicker_RenderMine(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font);

void Clicker_UpdateForest(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font);
void Clicker_RenderForest(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font);

#endif // CLICKER_H