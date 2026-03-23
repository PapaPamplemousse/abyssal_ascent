#include "clicker.h"
#include <stdio.h> // Pour sprintf
#include "../../utils/inc/lang.h"

void Clicker_Init(ClickerContext* clicker)
{
    clicker->inventory = (PlayerResources){0}; // Met tout à zéro
    clicker->autoTimer = 0.0f;
}

// Fonction appelée à chaque frame pour générer les ressources des auto-clickers
void Clicker_ProcessAuto(ClickerContext* clicker, float deltaTime)
{
    clicker->autoTimer += deltaTime;
    if (clicker->autoTimer >= 1.0f)
    { // Toutes les 1 seconde
        clicker->inventory.fer += clicker->inventory.auto_fer;
        clicker->inventory.or += clicker->inventory.auto_or;
        clicker->inventory.cristaux += clicker->inventory.auto_cristaux;
        clicker->inventory.bois += clicker->inventory.auto_bois;
        clicker->inventory.viande += clicker->inventory.auto_viande;
        clicker->inventory.herbes += clicker->inventory.auto_herbes;
        clicker->autoTimer -= 1.0f;
    }
}

// --- UTILITAIRES D'INTERFACE ---

// Fonction interne pour dessiner un texte centré et savoir s'il est cliqué
bool DrawAndCheckButtonCentered(Font font, const char* text, int centerX, int y, int fontSize, Color baseColor)
{
    Vector2   textSize = MeasureTextEx(font, text, fontSize, 1);
    Rectangle hitbox   = {centerX - (textSize.x / 2), y, textSize.x, textSize.y};

    bool  isHovered = CheckCollisionPointRec(GetMousePosition(), hitbox);
    Color drawColor = isHovered ? WHITE : baseColor;

    DrawTextEx(font, text, (Vector2){hitbox.x, hitbox.y}, fontSize, 1, drawColor);

    return isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// --- LA MINE ---

void Clicker_UpdateMine(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font)
{
    int colWidth   = viewWidth / 3;
    int col1Center = viewStartX + (colWidth / 2);
    int col2Center = viewStartX + colWidth + (colWidth / 2);
    int col3Center = viewStartX + (colWidth * 2) + (colWidth / 2);

    int artY = screenHeight * 0.3f; // Position du dessin ASCII
    int buyY = screenHeight * 0.6f; // Position du bouton d'achat

    // --- COLONNE 1 : FER ---
    if (DrawAndCheckButtonCentered(font, T("ART_IRON"), col1Center, artY, 30, LIGHTGRAY))
        clicker->inventory.fer++;

    // Coût du mineur de fer : 10 Fer
    int  costFer = 10 + (clicker->inventory.auto_fer * 5);
    char buyFerText[50];
    sprintf(buyFerText, T("BUY_MINER"), costFer);
    if (DrawAndCheckButtonCentered(font, buyFerText, col1Center, buyY, 20, GRAY))
    {
        if (clicker->inventory.fer >= costFer)
        {
            clicker->inventory.fer -= costFer;
            clicker->inventory.auto_fer++;
        }
    }

    // --- COLONNE 2 : OR ---
    if (DrawAndCheckButtonCentered(font, T("ART_GOLD"), col2Center, artY, 30, GOLD))
        clicker->inventory.or ++;

    // Coût du chercheur d'or : 50 Fer + 10 Or
    int  costOr = 10 + (clicker->inventory.auto_or * 10);
    char buyOrText[50];
    sprintf(buyOrText, T("BUY_PROSPECTOR"), costOr);
    if (DrawAndCheckButtonCentered(font, buyOrText, col2Center, buyY, 20, GRAY))
    {
        if (clicker->inventory.or >= costOr)
        {
            clicker->inventory.or -= costOr;
            clicker->inventory.auto_or++;
        }
    }

    // --- COLONNE 3 : CRISTAUX ---
    if (DrawAndCheckButtonCentered(font, T("ART_CRYSTAL"), col3Center, artY, 30, PURPLE))
        clicker->inventory.cristaux++;

    int  costCristal = 5 + (clicker->inventory.auto_cristaux * 5);
    char buyCristalText[50];
    sprintf(buyCristalText, T("BUY_EXTRACTOR"), costCristal);
    if (DrawAndCheckButtonCentered(font, buyCristalText, col3Center, buyY, 20, GRAY))
    {
        if (clicker->inventory.cristaux >= costCristal)
        {
            clicker->inventory.cristaux -= costCristal;
            clicker->inventory.auto_cristaux++;
        }
    }
}

void Clicker_RenderMine(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font)
{
    int colWidth = viewWidth / 3;

    // Titre
    DrawAndCheckButtonCentered(font, T("MINE_TITLE"), viewStartX + (viewWidth / 2), 120, 40, LIGHTGRAY);

    // Ligne horizontale sous le titre
    DrawLine(viewStartX, 180, viewStartX + viewWidth, 180, DARKGRAY);

    // Lignes verticales de séparation
    DrawLine(viewStartX + colWidth, 180, viewStartX + colWidth, screenHeight, DARKGRAY);
    DrawLine(viewStartX + (colWidth * 2), 180, viewStartX + (colWidth * 2), screenHeight, DARKGRAY);

    // Affichage des statistiques d'auto-click
    int  statsY = screenHeight * 0.8f;
    char statText[50];

    sprintf(statText, T("STAT_MINER"), clicker->inventory.auto_fer, clicker->inventory.auto_fer);
    // sprintf(statText, T("STAT_MINER"), clicker->inventory.auto_fer, clicker->inventory.auto_fer);
    DrawAndCheckButtonCentered(font, statText, viewStartX + (colWidth / 2), statsY, 20, DARKGRAY);

    sprintf(statText, T("STAT_PROSPECTOR"), clicker->inventory.auto_or, clicker->inventory.auto_or);
    DrawAndCheckButtonCentered(font, statText, viewStartX + colWidth + (colWidth / 2), statsY, 20, DARKGRAY);

    sprintf(statText, T("STAT_EXTRACTOR"), clicker->inventory.auto_cristaux, clicker->inventory.auto_cristaux);
    DrawAndCheckButtonCentered(font, statText, viewStartX + (colWidth * 2) + (colWidth / 2), statsY, 20, DARKGRAY);
}

// --- LA FORÊT SOMBRE ---

void Clicker_UpdateForest(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font)
{
    int colWidth   = viewWidth / 3;
    int col1Center = viewStartX + (colWidth / 2);
    int col2Center = viewStartX + colWidth + (colWidth / 2);
    int col3Center = viewStartX + (colWidth * 2) + (colWidth / 2);

    int artY = screenHeight * 0.3f; // Position Y des gros dessins ASCII
    int buyY = screenHeight * 0.6f; // Position Y des boutons d'achat

    // --- COLONNE 1 : HERBES ---
    // Gros ASCII art cliquable pour ramasser à la main
    if (DrawAndCheckButtonCentered(font, T("ART_HERBS"), col1Center, artY, 30, GREEN))
        clicker->inventory.herbes++;

    // Coût de l'Herboriste : 10 Herbes
    int  costHerboriste = 10 + (clicker->inventory.auto_herbes * 5);
    char buyHerbText[50];
    sprintf(buyHerbText, T("BUY_HERBALIST"), costHerboriste);
    if (DrawAndCheckButtonCentered(font, buyHerbText, col1Center, buyY, 20, GRAY))
    {
        if (clicker->inventory.herbes >= costHerboriste)
        {
            clicker->inventory.herbes -= costHerboriste;
            clicker->inventory.auto_herbes++;
        }
    }

    // --- COLONNE 2 : BOIS ---
    if (DrawAndCheckButtonCentered(font, T("ART_WOOD"), col2Center, artY, 30, BROWN))
        clicker->inventory.bois++;

    // Coût du Bûcheron : 10 Bois + 5 Herbes
    int  costBucheronBois = 10 + (clicker->inventory.auto_bois * 8);
    int  costBucheronHerb = 5 + (clicker->inventory.auto_bois * 2);
    char buyBoisText[60];
    sprintf(buyBoisText, T("BUY_LUMBERJACK"), costBucheronBois, costBucheronHerb);

    if (DrawAndCheckButtonCentered(font, buyBoisText, col2Center, buyY, 18, GRAY))
    {
        if (clicker->inventory.bois >= costBucheronBois && clicker->inventory.herbes >= costBucheronHerb)
        {
            clicker->inventory.bois -= costBucheronBois;
            clicker->inventory.herbes -= costBucheronHerb;
            clicker->inventory.auto_bois++;
        }
    }

    // --- COLONNE 3 : VIANDE ---
    if (DrawAndCheckButtonCentered(font, T("ART_MEAT"), col3Center, artY, 30, RED))
        clicker->inventory.viande++;

    // Coût du Chasseur : 20 Bois (pour arcs/pièges)
    int  costChasseurBois = 20 + (clicker->inventory.auto_viande * 10);
    char buyViandeText[50];
    sprintf(buyViandeText, T("BUY_HUNTER"), costChasseurBois);

    if (DrawAndCheckButtonCentered(font, buyViandeText, col3Center, buyY, 20, GRAY))
    {
        if (clicker->inventory.bois >= costChasseurBois)
        {
            clicker->inventory.bois -= costChasseurBois;
            clicker->inventory.auto_viande++;
        }
    }
}

void Clicker_RenderForest(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font)
{
    int colWidth = viewWidth / 3;

    // Titre de la zone
    DrawAndCheckButtonCentered(font, T("FOREST_TITLE"), viewStartX + (viewWidth / 2), 120, 40, GREEN);

    // Ligne horizontale sous le titre
    DrawLine(viewStartX, 180, viewStartX + viewWidth, 180, DARKGRAY);

    // Lignes verticales de séparation des 3 colonnes
    DrawLine(viewStartX + colWidth, 180, viewStartX + colWidth, screenHeight, DARKGRAY);
    DrawLine(viewStartX + (colWidth * 2), 180, viewStartX + (colWidth * 2), screenHeight, DARKGRAY);

    // Affichage des statistiques d'auto-click en bas
    int  statsY = screenHeight * 0.8f;
    char statText[50];

    sprintf(statText, T("STAT_HERBALIST"), clicker->inventory.auto_herbes, clicker->inventory.auto_herbes);
    DrawAndCheckButtonCentered(font, statText, viewStartX + (colWidth / 2), statsY, 20, DARKGRAY);

    sprintf(statText, T("STAT_LUMBERJACK"), clicker->inventory.auto_bois, clicker->inventory.auto_bois);
    DrawAndCheckButtonCentered(font, statText, viewStartX + colWidth + (colWidth / 2), statsY, 20, DARKGRAY);

    sprintf(statText, T("STAT_HUNTER"), clicker->inventory.auto_viande, clicker->inventory.auto_viande);
    DrawAndCheckButtonCentered(font, statText, viewStartX + (colWidth * 2) + (colWidth / 2), statsY, 20, DARKGRAY);
}