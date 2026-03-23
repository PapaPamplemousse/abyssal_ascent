#include "lang.h"
#include "cJSON.h"
#include "raylib.h"
#include <stdio.h>

cJSON* g_langDB    = NULL;
bool   g_isEnglish = true; // On commence en Français par défaut

void Lang_Init(const char* filepath)
{
    char* buffer = LoadFileText(filepath);
    if (buffer)
    {
        g_langDB = cJSON_Parse(buffer);
        if (!g_langDB)
        {
            printf("ERREUR LANGUE : Le fichier '%s' a ete trouve, mais le JSON est invalide (virgule en trop, guillemets manquants ?) !\n", filepath);
        }
        else
        {
            printf("SUCCES LANGUE : Fichier '%s' charge avec succes.\n", filepath);
        }
        UnloadFileText(buffer);
    }
    else
    {
        printf("ERREUR LANGUE : Impossible de trouver le fichier '%s'. Verifie tes dossiers !\n", filepath);
    }
}

void Lang_Close(void)
{
    if (g_langDB)
        cJSON_Delete(g_langDB);
}

const char* T(const char* key)
{
    if (!g_langDB)
        return key; // Si le JSON a planté, on affiche la clé

    // On cherche la clé (ex: "CAMP_TITLE")
    cJSON* item = cJSON_GetObjectItemCaseSensitive(g_langDB, key);
    if (!item)
        return key; // Clé non trouvée

    // On cherche la sous-clé ("en" ou "fr")
    cJSON* val = cJSON_GetObjectItemCaseSensitive(item, g_isEnglish ? "en" : "fr");
    return val ? val->valuestring : key;
}