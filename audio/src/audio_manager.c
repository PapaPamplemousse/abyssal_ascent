#include "audio_manager.h"

static Music tracks[MAX_MUSIC];
static Sound sfx[MAX_SOUNDS];

void Audio_Init(void)
{
    // Chargement des musiques (Ambiance)
    tracks[MUS_CAMP]    = LoadMusicStream("assets/audio/MUS_CAMP.wav");
    tracks[MUS_DUNGEON] = LoadMusicStream("assets/audio/MUS_DUNGEON.wav");
    tracks[MUS_FIRE]    = LoadMusicStream("assets/audio/MUS_FIRE.flac"); 
    tracks[MUS_BOSS]    = LoadMusicStream("assets/audio/MUS_BOSS.wav");

    // Le feu et les ambiances doivent boucler à l'infini
    tracks[MUS_CAMP].looping = true;
    tracks[MUS_DUNGEON].looping = true;
    tracks[MUS_FIRE].looping = true;
    tracks[MUS_BOSS].looping = true;

    // Chargement des bruitages courts (.wav recommandé)
    sfx[SFX_CLICK]    = LoadSound("assets/audio/SFX_CLICK.wav");
    sfx[SFX_CLICK_BUY] = LoadSound("assets/audio/SFX_CLICK_BUY.wav");
    sfx[SFX_ATTACK]   = LoadSound("assets/audio/SFX_ATTACK.wav");
    sfx[SFX_QTE_OK]   = LoadSound("assets/audio/SFX_QTE_OK.wav");
    sfx[SFX_QTE_FAIL] = LoadSound("assets/audio/SFX_QTE_FAIL.wav");
    sfx[SFX_POTION]   = LoadSound("assets/audio/SFX_POTION.wav");

    // Ajustement des volumes (le feu ne doit pas couvrir la musique)
    SetMusicVolume(tracks[MUS_FIRE], 0.6f);
    SetMusicVolume(tracks[MUS_CAMP], 0.5f);
    SetMusicVolume(tracks[MUS_DUNGEON], 0.5f);
}

// Fonction vitale : doit être appelée à chaque frame pour faire avancer les musiques
void Audio_Update(void)
{
    for (int i = 0; i < MAX_MUSIC; i++) {
        if (IsMusicStreamPlaying(tracks[i])) {
            UpdateMusicStream(tracks[i]);
        }
    }
}

void Audio_PlayBGM(MusicID id) {
    if (!IsMusicStreamPlaying(tracks[id])) PlayMusicStream(tracks[id]);
}

void Audio_StopBGM(MusicID id) {
    if (IsMusicStreamPlaying(tracks[id])) StopMusicStream(tracks[id]);
}

void Audio_PlaySFX(SoundID id) {
    PlaySound(sfx[id]);
}

void Audio_Unload(void)
{
    for (int i = 0; i < MAX_MUSIC; i++) UnloadMusicStream(tracks[i]);
    for (int i = 0; i < MAX_SOUNDS; i++) UnloadSound(sfx[i]);
}