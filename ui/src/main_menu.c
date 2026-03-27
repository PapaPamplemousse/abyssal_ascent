#include "main_menu.h"
#include "ui.h"
#include "lang.h"
#include "raylib.h"
#include "audio_manager.h"
#include <stdio.h>

extern void Game_ResetState(GameContext* game);
extern bool g_isEnglish;

typedef enum {
    PAGE_MAIN,
    PAGE_SETTINGS
} MenuPage;

static MenuPage currentPage = PAGE_MAIN;
static float masterVolume = 1.0f;

void MainMenu_Render(GameContext* game, int w, int h)
{
    int cx = w / 2;
    int cy = h / 2;

    // --- ASCII ART DU TITRE ---
    const char* titleAscii[] = {
        "  ___  _                     _    ___                   _  ",
        " / _ \\| |                   | |  / _ \\                 | | ",
        "/ /_\\ \\ |__  _   _ ___ ___  | | / /_\\ \\___  ___ ___ _ _| |_",
        "|  _  | '_ \\| | | / __/ __| | | |  _  / __|/ __/ _ \\ '_| __|",
        "| | | | |_) | |_| \\__ \\__ \\ | | | | | \\__ \\ (_|  __/ | | |_",
        "\\_| |_/_.__/ \\__, |___/___/ |_| \\_| |_/___/\\___\\___|_|  \\__|",
        "              __/ |                                        ",
        "             |___/                                         "
    };
    
    int titleLines = 8;
    int startY = cy - 250;
    for (int i = 0; i < titleLines; i++) {
        Vector2 tSize = MeasureTextEx(game->dungeonFont, titleAscii[i], 20, 1);
        DrawTextEx(game->dungeonFont, titleAscii[i], (Vector2){cx - (tSize.x / 2), startY + (i * 20)}, 20, 1, PURPLE);
    }

    // ==========================================
    // PAGE D'ACCUEIL
    // ==========================================
    if (currentPage == PAGE_MAIN)
    {
        // On vérifie si une sauvegarde existe pour activer/griser "Continuer"
        bool hasSave = FileExists("save.json");

        const char* btnContinue = g_isEnglish ? "[ CONTINUE ]" : "[ CONTINUER ]";
        if (DoShopButton(game->uiFont, btnContinue, cx - 100, cy - 50, 24, hasSave)) {
            game->currentState = STATE_CAMP;
        }

        const char* btnNew = g_isEnglish ? "[ NEW GAME ]" : "[ NOUVELLE PARTIE ]";
        if (DoShopButton(game->uiFont, btnNew, cx - 120, cy + 10, 24, true)) {
            Game_ResetState(game); // Détruit la save et remet tout à zéro !
            game->currentState = STATE_CAMP;
        }

        const char* btnSettings = g_isEnglish ? "[ SETTINGS ]" : "[ PARAMETRES ]";
        if (DoShopButton(game->uiFont, btnSettings, cx - 100, cy + 70, 24, true)) {
            currentPage = PAGE_SETTINGS;
        }

        const char* btnQuit = g_isEnglish ? "[ QUIT ]" : "[ QUITTER ]";
        if (DoShopButton(game->uiFont, btnQuit, cx - 80, cy + 130, 24, true)) {
            game->isRunning = false;
        }
    }
    // ==========================================
    // PAGE PARAMETRES
    // ==========================================
    else if (currentPage == PAGE_SETTINGS)
    {
        const char* titleSettings = g_isEnglish ? "--- SETTINGS ---" : "--- PARAMETRES ---";
        Vector2 sSize = MeasureTextEx(game->uiFont, titleSettings, 30, 1);
        DrawTextEx(game->uiFont, titleSettings, (Vector2){cx - (sSize.x / 2), cy - 100}, 30, 1, LIGHTGRAY);

        // Changer la langue
        const char* langText = g_isEnglish ? "LANGUAGE : ENGLISH" : "LANGUE : FRANCAIS";
        if (DoShopButton(game->uiFont, langText, cx - 130, cy - 30, 24, true)) {
            g_isEnglish = !g_isEnglish;
        }

        // Changer le volume (Boucle 100% -> 75% -> 50% -> 25% -> 0%)
        char volText[64];
        sprintf(volText, g_isEnglish ? "MASTER VOLUME : %d%%" : "VOLUME GLOBAL : %d%%", (int)(masterVolume * 100));
        if (DoShopButton(game->uiFont, volText, cx - 150, cy + 30, 24, true)) {
            masterVolume -= 0.25f;
            if (masterVolume < 0.0f) masterVolume = 1.0f;
            SetMasterVolume(masterVolume);
            Audio_PlaySFX(SFX_CLICK);  
        }

        // Retour
        const char* btnBack = g_isEnglish ? "[ BACK ]" : "[ RETOUR ]";
        if (DoShopButton(game->uiFont, btnBack, cx - 80, cy + 110, 24, true)) {
            currentPage = PAGE_MAIN;
        }
    }
}