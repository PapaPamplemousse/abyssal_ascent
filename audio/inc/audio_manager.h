#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "raylib.h"

// --- LES PISTES D'AMBIANCE (Music) ---
typedef enum {
    MUS_CAMP = 0,
    MUS_DUNGEON,
    MUS_FIRE,      // Le crépitement du feu (traité comme une musique pour boucler)
    MUS_BOSS,
    MAX_MUSIC
} MusicID;

// --- LES BRUITAGES (Sound) ---
typedef enum {
    SFX_CLICK = 0, //  Mine, Forêt...
    SFX_CLICK_BUY, // Achat
    SFX_ATTACK,    // Coup d'épée
    SFX_QTE_OK,    // Touche réussie !
    SFX_QTE_FAIL,  // Mauvaise touche !
    SFX_POTION,
    MAX_SOUNDS
} SoundID;

void Audio_Init(void);
void Audio_Update(void);
void Audio_Unload(void);

void Audio_PlayBGM(MusicID id);
void Audio_StopBGM(MusicID id);
void Audio_PlaySFX(SoundID id);

#endif // AUDIO_MANAGER_H