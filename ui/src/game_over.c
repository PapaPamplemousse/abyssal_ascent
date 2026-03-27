#include "game_over.h"
#include "ui.h"
#include "lang.h"
#include "save.h"
#include "combat.h"
#include "audio_manager.h"
#include <stdio.h>

extern bool g_isEnglish;

void GameOver_Render(GameContext* game, DungeonContext* dungeon, int w, int h)
{
    int cx = w / 2;
    int cy = h / 2;

    // --- 1. LE TITRE  ---
    const char* title = g_isEnglish ? "Y O U   D I E D" : "V O U S   E T E S   M O R T";
    Vector2 titleSize = MeasureTextEx(game->uiFont, title, 50, 1);
    DrawTextEx(game->uiFont, title, (Vector2){cx - (titleSize.x / 2), cy - 250}, 50, 1, RED);

    // --- 2. LE CRÂNE  ---
    const char* skull[] = {
        "                 uuuuuuu",
        "             uu$$$$$$$$$$$uu",
        "          uu$$$$$$$$$$$$$$$$$uu",
        "         u$$$$$$$$$$$$$$$$$$$$$u",
        "        u$$$$$$$$$$$$$$$$$$$$$$$u",
        "       u$$$$$$$$$$$$$$$$$$$$$$$$$u",
        "       u$$$$$$$$$$$$$$$$$$$$$$$$$u",
        "       u$$$$$$\"   \"$$$\"   \"$$$$$$u",
        "       \"$$$$\"      u$u       $$$$\"",
        "        $$$u       u$u       u$$$",
        "        $$$u      u$$$u      u$$$",
        "         \"$$$$uu$$$   $$$uu$$$$\"",
        "          \"$$$$$$$\"   \"$$$$$$$\"",
        "            u$$$$$$$u$$$$$$$u",
        "             u$\"$\"$\"$\"$\"$\"$u",
        "  uuu        $$u$ $ $ $ $u$$       uuu",
        " u$$$$        $$$$$u$u$u$$$       u$$$$",
        "  $$$$$uu      \"$$$$$$$$$\"     uu$$$$$$",
        "u$$$$$$$$$$$uu    \"\"\"\"\"    uuuu$$$$$$$$$$",
        "$$$$\"\"\"!!\"\"$$$$$$$$$$$$$$$$$$$$$$$$$$$$\"",
        " \"\"\"      \"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\""
    };

    int lines = 21;
    int startY = cy - 170;
    for(int i = 0; i < lines; i++) {
        Vector2 tSize = MeasureTextEx(game->dungeonFont, skull[i], 16, 1);
        DrawTextEx(game->dungeonFont, skull[i], (Vector2){cx - (tSize.x / 2), startY + (i * 16)}, 16, 1, DARKGRAY);
    }

    // --- 3. LES STATISTIQUES DE LA RUN ---
    char stats[256];
    if (g_isEnglish) {
        sprintf(stats, "Floor Reached: %d\nMonsters Killed: %d\nLevel Reached: %d", dungeon->highest_floor, game->combat.monsters_killed, game->combat.player.level);
    } else {
        sprintf(stats, "Etage atteint : %d\nMonstres vaincus : %d\nNiveau atteint : %d", dungeon->highest_floor, game->combat.monsters_killed, game->combat.player.level);
    }
    
    // On centre manuellement le texte multilingue
    Vector2 statsSize = MeasureTextEx(game->uiFont, stats, 24, 1);
    DrawTextEx(game->uiFont, stats, (Vector2){cx - (statsSize.x / 2), startY + (lines * 16) + 30}, 24, 1, LIGHTGRAY);

    // --- 4. LE BOUTON DE RETOUR ---
    const char* btnText = g_isEnglish ? "[ RETURN TO CAMP ]" : "[ RETOUR AU CAMP ]";
    Vector2 btnSize = MeasureTextEx(game->uiFont, btnText, 24, 1);
    
    // On peut cliquer sur le bouton OU appuyer sur Espace/Entrée
    if (DoShopButton(game->uiFont, btnText, cx - (btnSize.x / 2), h - 80, 24, true) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) 
    {
        Audio_PlaySFX(SFX_CLICK);
        Combat_ResetRun(&game->combat);
        game->combat.player.hp = 1;          // Renaissance avec 1 PV
        game->currentState = STATE_CAMP;
        SaveGame(game, dungeon);             // On sauvegarde le retour au camp !
    }
}