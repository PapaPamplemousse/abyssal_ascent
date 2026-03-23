#include "clicker.h"
#include <stdio.h> 
#include "../../utils/inc/lang.h"

void Clicker_Init(ClickerContext* clicker)
{
    clicker->inventory = (PlayerResources){0}; 
    clicker->autoTimer = 0.0f;
}

// Calcule et ajoute la production par seconde
void Clicker_ProcessAuto(ClickerContext* clicker, float deltaTime)
{
    clicker->autoTimer += deltaTime;
    if (clicker->autoTimer >= 1.0f)
    { 
        PlayerResources* inv = &clicker->inventory;
        
        int p_fer = inv->b_fer[0]*1 + inv->b_fer[1]*10 + inv->b_fer[2]*100 + inv->b_fer[3]*1000;
        int p_or = inv->b_or[0]*1 + inv->b_or[1]*10 + inv->b_or[2]*100 + inv->b_or[3]*1000;
        int p_cris = inv->b_cristaux[0]*1 + inv->b_cristaux[1]*10 + inv->b_cristaux[2]*100 + inv->b_cristaux[3]*1000;
        
        int p_herb = inv->b_herbes[0]*1 + inv->b_herbes[1]*10 + inv->b_herbes[2]*100 + inv->b_herbes[3]*1000;
        int p_bois = inv->b_bois[0]*1 + inv->b_bois[1]*10 + inv->b_bois[2]*100 + inv->b_bois[3]*1000;
        int p_vian = inv->b_viande[0]*1 + inv->b_viande[1]*10 + inv->b_viande[2]*100 + inv->b_viande[3]*1000;

        inv->fer += p_fer;
        if (inv->unlock_or) inv->or += p_or;
        if (inv->unlock_cristaux) inv->cristaux += p_cris;
        
        inv->herbes += p_herb;
        if (inv->unlock_bois) inv->bois += p_bois;
        if (inv->unlock_viande) inv->viande += p_vian;

        clicker->autoTimer -= 1.0f;
    }
}

bool DrawAndCheckButtonCentered(Font font, const char* text, int centerX, int y, int fontSize, Color baseColor)
{
    Vector2 textSize = MeasureTextEx(font, text, fontSize, 1);
    Rectangle hitbox = {centerX - (textSize.x / 2), y, textSize.x, textSize.y};
    bool isHovered = CheckCollisionPointRec(GetMousePosition(), hitbox);
    Color drawColor = isHovered ? WHITE : baseColor;
    DrawTextEx(font, text, (Vector2){hitbox.x, hitbox.y}, fontSize, 1, drawColor);
    return isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Super fonction utilitaire pour générer les boutons d'achat proprement
void BuyBld(Font font, const char* name, int* count, int baseCost, int scale, int* res, int x, int y) {
    int cost = baseCost + (*count * scale);
    char txt[64];
    sprintf(txt, "[%d] %s (-%d)", *count, name, cost);
    if (DrawAndCheckButtonCentered(font, txt, x, y, 18, GRAY)) {
        if (*res >= cost) { *res -= cost; (*count)++; }
    }
}

// --- LA MINE ---
void Clicker_UpdateMine(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font)
{
    int colWidth = viewWidth / 3;
    int c1 = viewStartX + (colWidth / 2);
    int c2 = viewStartX + colWidth + (colWidth / 2);
    int c3 = viewStartX + (colWidth * 2) + (colWidth / 2);

    int artY = screenHeight * 0.25f; 
    int buyY = screenHeight * 0.45f; 

    PlayerResources* inv = &clicker->inventory;

    // COLONNE 1 : FER (Toujours débloqué)
    if (DrawAndCheckButtonCentered(font, T("ART_IRON"), c1, artY, 30, LIGHTGRAY)) inv->fer++;
    BuyBld(font, "Mineur", &inv->b_fer[0], 10, 5, &inv->fer, c1, buyY);
    BuyBld(font, "Foreuse", &inv->b_fer[1], 150, 50, &inv->fer, c1, buyY + 40);
    BuyBld(font, "Excavatrice", &inv->b_fer[2], 2000, 500, &inv->fer, c1, buyY + 80);
    BuyBld(font, "Faille Terrestre", &inv->b_fer[3], 25000, 5000, &inv->fer, c1, buyY + 120);

    // COLONNE 2 : OR
    if (!inv->unlock_or) {
        if (DrawAndCheckButtonCentered(font, "DEBLOQUER L'OR\n\n(-1000 Fer)", c2, screenHeight/2, 20, YELLOW)) {
            if (inv->fer >= 1000) { inv->fer -= 1000; inv->unlock_or = true; }
        }
    } else {
        if (DrawAndCheckButtonCentered(font, T("ART_GOLD"), c2, artY, 30, GOLD)) inv->or++;
        BuyBld(font, "Chercheur", &inv->b_or[0], 10, 5, &inv->or, c2, buyY);
        BuyBld(font, "Orpailleur", &inv->b_or[1], 150, 50, &inv->or, c2, buyY + 40);
        BuyBld(font, "Mine d'Or", &inv->b_or[2], 2000, 500, &inv->or, c2, buyY + 80);
        BuyBld(font, "Transmutateur", &inv->b_or[3], 25000, 5000, &inv->or, c2, buyY + 120);
    }

    // COLONNE 3 : CRISTAUX
    if (!inv->unlock_cristaux) {
        if (DrawAndCheckButtonCentered(font, "DEBLOQUER CRISTAUX\n\n(-10k Fer, -1k Or)", c3, screenHeight/2, 18, PURPLE)) {
            if (inv->fer >= 10000 && inv->or >= 1000) {
                inv->fer -= 10000; inv->or -= 1000; inv->unlock_cristaux = true;
            }
        }
    } else {
        if (DrawAndCheckButtonCentered(font, T("ART_CRYSTAL"), c3, artY, 30, PURPLE)) inv->cristaux++;
        BuyBld(font, "Extracteur", &inv->b_cristaux[0], 10, 5, &inv->cristaux, c3, buyY);
        BuyBld(font, "Resonateur", &inv->b_cristaux[1], 150, 50, &inv->cristaux, c3, buyY + 40);
        BuyBld(font, "Puits Magique", &inv->b_cristaux[2], 2000, 500, &inv->cristaux, c3, buyY + 80);
        BuyBld(font, "Monolithe", &inv->b_cristaux[3], 25000, 5000, &inv->cristaux, c3, buyY + 120);
    }
}

void Clicker_RenderMine(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font)
{
    int colWidth = viewWidth / 3;
    DrawAndCheckButtonCentered(font, T("MINE_TITLE"), viewStartX + (viewWidth / 2), 100, 50, LIGHTGRAY);
    DrawLine(viewStartX, 150, viewStartX + viewWidth, 150, DARKGRAY);
    DrawLine(viewStartX + colWidth, 150, viewStartX + colWidth, screenHeight, DARKGRAY);
    DrawLine(viewStartX + (colWidth * 2), 150, viewStartX + (colWidth * 2), screenHeight, DARKGRAY);

    // Affichage des Productions par seconde (Dynamique)
    int statsY = screenHeight * 0.85f;
    char statText[50];
    PlayerResources* inv = &clicker->inventory;

    int p_fer = inv->b_fer[0]*1 + inv->b_fer[1]*10 + inv->b_fer[2]*100 + inv->b_fer[3]*1000;
    sprintf(statText, "Prod: +%d/sec", p_fer);
    DrawAndCheckButtonCentered(font, statText, viewStartX + (colWidth / 2), statsY, 20, DARKGRAY);

    if (inv->unlock_or) {
        int p_or = inv->b_or[0]*1 + inv->b_or[1]*10 + inv->b_or[2]*100 + inv->b_or[3]*1000;
        sprintf(statText, "Prod: +%d/sec", p_or);
        DrawAndCheckButtonCentered(font, statText, viewStartX + colWidth + (colWidth / 2), statsY, 20, DARKGRAY);
    }
    if (inv->unlock_cristaux) {
        int p_cris = inv->b_cristaux[0]*1 + inv->b_cristaux[1]*10 + inv->b_cristaux[2]*100 + inv->b_cristaux[3]*1000;
        sprintf(statText, "Prod: +%d/sec", p_cris);
        DrawAndCheckButtonCentered(font, statText, viewStartX + (colWidth * 2) + (colWidth / 2), statsY, 20, DARKGRAY);
    }
}

// --- LA FORÊT SOMBRE ---
void Clicker_UpdateForest(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font)
{
    int colWidth = viewWidth / 3;
    int c1 = viewStartX + (colWidth / 2);
    int c2 = viewStartX + colWidth + (colWidth / 2);
    int c3 = viewStartX + (colWidth * 2) + (colWidth / 2);

    int artY = screenHeight * 0.25f; 
    int buyY = screenHeight * 0.45f; 

    PlayerResources* inv = &clicker->inventory;

    // COLONNE 1 : HERBES
    if (DrawAndCheckButtonCentered(font, T("ART_HERBS"), c1, artY, 30, GREEN)) inv->herbes++;
    BuyBld(font, "Herboriste", &inv->b_herbes[0], 10, 5, &inv->herbes, c1, buyY);
    BuyBld(font, "Serre", &inv->b_herbes[1], 150, 50, &inv->herbes, c1, buyY + 40);
    BuyBld(font, "Bosquet", &inv->b_herbes[2], 2000, 500, &inv->herbes, c1, buyY + 80);
    BuyBld(font, "Arbre Monde", &inv->b_herbes[3], 25000, 5000, &inv->herbes, c1, buyY + 120);

    // COLONNE 2 : BOIS
    if (!inv->unlock_bois) {
        if (DrawAndCheckButtonCentered(font, "DEBLOQUER LE BOIS\n\n(-1000 Herbes)", c2, screenHeight/2, 20, BROWN)) {
            if (inv->herbes >= 1000) { inv->herbes -= 1000; inv->unlock_bois = true; }
        }
    } else {
        if (DrawAndCheckButtonCentered(font, T("ART_WOOD"), c2, artY, 30, BROWN)) inv->bois++;
        BuyBld(font, "Bucheron", &inv->b_bois[0], 10, 5, &inv->bois, c2, buyY);
        BuyBld(font, "Scierie", &inv->b_bois[1], 150, 50, &inv->bois, c2, buyY + 40);
        BuyBld(font, "Treant", &inv->b_bois[2], 2000, 500, &inv->bois, c2, buyY + 80);
        BuyBld(font, "Esprit Foret", &inv->b_bois[3], 25000, 5000, &inv->bois, c2, buyY + 120);
    }

    // COLONNE 3 : VIANDE
    if (!inv->unlock_viande) {
        if (DrawAndCheckButtonCentered(font, "DEBLOQUER VIANDE\n\n(-10k Herb, -1k Bois)", c3, screenHeight/2, 18, RED)) {
            if (inv->herbes >= 10000 && inv->bois >= 1000) {
                inv->herbes -= 10000; inv->bois -= 1000; inv->unlock_viande = true;
            }
        }
    } else {
        if (DrawAndCheckButtonCentered(font, T("ART_MEAT"), c3, artY, 30, RED)) inv->viande++;
        BuyBld(font, "Chasseur", &inv->b_viande[0], 10, 5, &inv->viande, c3, buyY);
        BuyBld(font, "Trappeur", &inv->b_viande[1], 150, 50, &inv->viande, c3, buyY + 40);
        BuyBld(font, "Abattoir", &inv->b_viande[2], 2000, 500, &inv->viande, c3, buyY + 80);
        BuyBld(font, "Cloneur", &inv->b_viande[3], 25000, 5000, &inv->viande, c3, buyY + 120);
    }
}

void Clicker_RenderForest(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font)
{
    int colWidth = viewWidth / 3;
    DrawAndCheckButtonCentered(font, T("FOREST_TITLE"), viewStartX + (viewWidth / 2), 100, 50, GREEN);
    DrawLine(viewStartX, 150, viewStartX + viewWidth, 150, DARKGRAY);
    DrawLine(viewStartX + colWidth, 150, viewStartX + colWidth, screenHeight, DARKGRAY);
    DrawLine(viewStartX + (colWidth * 2), 150, viewStartX + (colWidth * 2), screenHeight, DARKGRAY);

    int statsY = screenHeight * 0.85f;
    char statText[50];
    PlayerResources* inv = &clicker->inventory;

    int p_herb = inv->b_herbes[0]*1 + inv->b_herbes[1]*10 + inv->b_herbes[2]*100 + inv->b_herbes[3]*1000;
    sprintf(statText, "Prod: +%d/sec", p_herb);
    DrawAndCheckButtonCentered(font, statText, viewStartX + (colWidth / 2), statsY, 20, DARKGRAY);

    if (inv->unlock_bois) {
        int p_bois = inv->b_bois[0]*1 + inv->b_bois[1]*10 + inv->b_bois[2]*100 + inv->b_bois[3]*1000;
        sprintf(statText, "Prod: +%d/sec", p_bois);
        DrawAndCheckButtonCentered(font, statText, viewStartX + colWidth + (colWidth / 2), statsY, 20, DARKGRAY);
    }
    if (inv->unlock_viande) {
        int p_vian = inv->b_viande[0]*1 + inv->b_viande[1]*10 + inv->b_viande[2]*100 + inv->b_viande[3]*1000;
        sprintf(statText, "Prod: +%d/sec", p_vian);
        DrawAndCheckButtonCentered(font, statText, viewStartX + (colWidth * 2) + (colWidth / 2), statsY, 20, DARKGRAY);
    }
}