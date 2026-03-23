#ifndef CLICKER_H
#define CLICKER_H

#include "raylib.h"

typedef struct
{
    // Ressources brutes
    int herbes, fer, viande, or, bois, cristaux;
    // Niveaux des auto-clickers (Ex: Mineurs, Bûcherons)
    int auto_herbes, auto_fer, auto_viande, auto_or, auto_bois, auto_cristaux;
} PlayerResources;

typedef struct
{
    PlayerResources inventory;
    float           autoTimer; // Chronomètre pour générer les ressources automatiques
} ClickerContext;

void Clicker_Init(ClickerContext* clicker);
void Clicker_ProcessAuto(ClickerContext* clicker, float deltaTime);

void Clicker_UpdateMine(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font);
void Clicker_RenderMine(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font);

void Clicker_UpdateForest(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font);
void Clicker_RenderForest(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font);
#endif // CLICKER_H