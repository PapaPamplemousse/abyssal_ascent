#ifndef LANG_H
#define LANG_H

#include <stdbool.h>

extern bool g_isEnglish; // Variable globale pour changer la langue en plein jeu

void Lang_Init(const char* filepath);
void Lang_Close(void);

// La fonction magique "T" (pour Translate)
const char* T(const char* key);

#endif // LANG_H