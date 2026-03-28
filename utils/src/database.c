#include "database.h"
#include "combat.h"  // Pour accéder à g_monsterDB, g_itemDB, etc.
#include "dungeon.h" // Pour accéder à g_eventDB, etc.
#include "lang.h"    // Pour Lang_Init
#include "cJSON.h"
#include <stdio.h>
#include <string.h>

static void LoadMonstersDB(const char* filepath);
static void LoadItemsDB(const char* filepath);
static void LoadMagicDB(const char* spell_path, const char* potion_path);
static void LoadDungeonDB(const char* ambiance_path, const char* rooms_path);

static EffectType ParseEffectType(const char* str);

// --- FONCTION PUBLIQUE ---
void DB_Init(const char* monsters_path, const char* items_path, const char* spells_path, const char* potions_path, const char* ambiance_path, const char* rooms_path, const char* lang_path)
{
    printf("=== DEMARRAGE DU CHARGEMENT DES BASES DE DONNEES ===\n");

    Lang_Init(lang_path); // On charge la langue en premier (pratique pour les logs)
    LoadMonstersDB(monsters_path);
    LoadItemsDB(items_path);
    LoadMagicDB(spells_path, potions_path);
    LoadDungeonDB(ambiance_path, rooms_path);

    printf("=== FIN DU CHARGEMENT DES BASES DE DONNEES ===\n");
}

// --- FONCTIONS PRIVÉES (Uniquement utilisées dans ce fichier) ---

/**
 * @brief Charge la base de données des monstres depuis un fichier JSON.
 * @details Remplit g_monsterDB et g_monsterCount.
 * @param[in] filepath Chemin vers le fichier JSON des monstres
 * @note Fonction privée
 */
static void LoadMonstersDB(const char* filepath)
{
    char* buffer = LoadFileText(filepath);
    if (!buffer)
    {
        printf("[ERREUR] Impossible de lire le fichier %s\n", filepath);
        return;
    }

    cJSON* json = cJSON_Parse(buffer);
    if (!json)
    {
        printf("[ERREUR] Syntaxe JSON invalide dans %s\n", filepath);
        UnloadFileText(buffer);
        return;
    }

    cJSON* monstersArray = cJSON_GetObjectItemCaseSensitive(json, "monsters");
    cJSON* monsterNode   = NULL;

    g_monsterCount = 0;
    printf("\n--- DEBUT LECTURE JSON MONSTRES ---\n");

    cJSON_ArrayForEach(monsterNode, monstersArray)
    {
        if (g_monsterCount >= MAX_MONSTERS_DB)
        {
            printf("[ATTENTION] Limite de MAX_MONSTERS_DB (%d) atteinte !\n", MAX_MONSTERS_DB);
            break;
        }

        MonsterTemplate* t = &g_monsterDB[g_monsterCount];

        strcpy(t->id, cJSON_GetObjectItem(monsterNode, "id")->valuestring);
        strcpy(t->name_en, cJSON_GetObjectItem(monsterNode, "name_en")->valuestring);
        strcpy(t->name_fr, cJSON_GetObjectItem(monsterNode, "name_fr")->valuestring);
        strcpy(t->flavor_en, cJSON_GetObjectItem(monsterNode, "flavor_en")->valuestring);
        strcpy(t->flavor_fr, cJSON_GetObjectItem(monsterNode, "flavor_fr")->valuestring);

        cJSON* bossNode = cJSON_GetObjectItem(monsterNode, "is_boss");
        t->is_boss      = false;
        if (bossNode != NULL && bossNode->type == cJSON_True)
        {
            t->is_boss = true;
        }

        if (t->is_boss)
        {
            cJSON* bFloor = cJSON_GetObjectItem(monsterNode, "boss_floor");
            t->boss_floor = bFloor ? bFloor->valueint : 10;
            t->min_floor  = 0; // Sécurité
            t->max_floor  = 0; // Sécurité
        }
        else
        {
            cJSON* mMin   = cJSON_GetObjectItem(monsterNode, "min_floor");
            cJSON* mMax   = cJSON_GetObjectItem(monsterNode, "max_floor");
            t->min_floor  = mMin ? mMin->valueint : 1;
            t->max_floor  = mMax ? mMax->valueint : 999;
            t->boss_floor = 0; // Sécurité
        }

        cJSON* stats = cJSON_GetObjectItem(monsterNode, "stats");
        if (stats)
        {
            cJSON* hpNode  = cJSON_GetObjectItem(stats, "hp");
            cJSON* atkNode = cJSON_GetObjectItem(stats, "atk");
            cJSON* spdNode = cJSON_GetObjectItem(stats, "spd");
            cJSON* xpNode  = cJSON_GetObjectItem(stats, "xp");

            // On utilise valuedouble (très fiable) qu'on cast en int
            t->hp  = hpNode ? (int)hpNode->valuedouble : 50;
            t->atk = atkNode ? (int)atkNode->valuedouble : 5;
            t->spd = spdNode ? spdNode->valuedouble : 1.0;
            t->xp  = xpNode ? (int)xpNode->valuedouble : 10;
        }

        cJSON* colorArray = cJSON_GetObjectItem(monsterNode, "color");
        if (colorArray && cJSON_GetArraySize(colorArray) >= 3)
        {
            t->base_color.r = cJSON_GetArrayItem(colorArray, 0)->valueint;
            t->base_color.g = cJSON_GetArrayItem(colorArray, 1)->valueint;
            t->base_color.b = cJSON_GetArrayItem(colorArray, 2)->valueint;
            t->base_color.a = 255;
        }
        else
        {
            t->base_color = LIGHTGRAY;
        }

        // ---  GESTION DE L'IMAGE (PNG) ---
        cJSON* imageNode = cJSON_GetObjectItem(monsterNode, "image");
        
        if (imageNode && imageNode->valuestring)
        {
            strcpy(t->image_path, imageNode->valuestring);
        }
        else
        {
            printf("pas d'image par defaut pour le monstre %d ",g_monsterCount);
        }

        // LE PRINTF ESPION !
        printf("[JSON] Lu: %s | Boss: %d | Etages: %d-%d (BossFloor: %d)\n", t->id, t->is_boss, t->min_floor, t->max_floor, t->boss_floor);

        g_monsterCount++;
    }

    cJSON_Delete(json);
    UnloadFileText(buffer);
    printf("--- FIN LECTURE JSON : %d monstres charges ---\n\n", g_monsterCount);
}

/**
 * @brief Charge la base de données des objets depuis un fichier JSON.
 * @details Remplit g_itemDB et g_itemCount.
 * @param[in] filepath Chemin vers le fichier JSON des objets
 * @note Fonction privée
 */
static void LoadItemsDB(const char* filepath)
{
    char* buffer = LoadFileText(filepath);
    if (!buffer)
        return;

    cJSON* json = cJSON_Parse(buffer);
    if (!json)
    {
        UnloadFileText(buffer);
        return;
    }

    cJSON* itemsArray = cJSON_GetObjectItemCaseSensitive(json, "items");
    cJSON* itemNode   = NULL;
    g_itemCount       = 0;

    cJSON_ArrayForEach(itemNode, itemsArray)
    {
        if (g_itemCount >= MAX_ITEMS_DB)
            break;
        ItemTemplate* t = &g_itemDB[g_itemCount];

        strcpy(t->id, cJSON_GetObjectItem(itemNode, "id")->valuestring);
        strcpy(t->name_en, cJSON_GetObjectItem(itemNode, "name_en")->valuestring);
        strcpy(t->name_fr, cJSON_GetObjectItem(itemNode, "name_fr")->valuestring);
        strcpy(t->type, cJSON_GetObjectItem(itemNode, "type")->valuestring);

        cJSON* stats = cJSON_GetObjectItem(itemNode, "stats");
        t->hp        = cJSON_GetObjectItem(stats, "hp")->valueint;
        t->atk       = cJSON_GetObjectItem(stats, "atk")->valueint;
        t->mana      = cJSON_GetObjectItem(stats, "mana")->valueint;
        t->spd       = cJSON_GetObjectItem(stats, "spd")->valuedouble;
        t->fog       = cJSON_GetObjectItem(stats, "fog")->valueint;

        cJSON* stat_inc = cJSON_GetObjectItem(itemNode, "stat_inc");
        t->inc_hp       = cJSON_GetObjectItem(stat_inc, "hp")->valueint;
        t->inc_atk      = cJSON_GetObjectItem(stat_inc, "atk")->valueint;
        t->inc_mana     = cJSON_GetObjectItem(stat_inc, "mana")->valueint;
        t->inc_spd      = cJSON_GetObjectItem(stat_inc, "spd")->valuedouble;
        t->inc_fog      = cJSON_GetObjectItem(stat_inc, "fog")->valueint;

        cJSON* upgrade   = cJSON_GetObjectItem(itemNode, "upgrade");
        cJSON* ferNode   = cJSON_GetObjectItem(upgrade, "fer");
        t->cost_fer_base = ferNode ? cJSON_GetObjectItem(ferNode, "base")->valueint : 0;
        t->cost_fer_inc  = ferNode ? cJSON_GetObjectItem(ferNode, "inc")->valueint : 0;

        cJSON* boisNode   = cJSON_GetObjectItem(upgrade, "bois");
        t->cost_bois_base = boisNode ? cJSON_GetObjectItem(boisNode, "base")->valueint : 0;
        t->cost_bois_inc  = boisNode ? cJSON_GetObjectItem(boisNode, "inc")->valueint : 0;

        // --- GESTION DE L'IMAGE DES OBJETS ---
        cJSON* imageNode = cJSON_GetObjectItem(itemNode, "image");
        
        if (imageNode && imageNode->valuestring)
        {
            strcpy(t->image_path, imageNode->valuestring);
        }
        else
        {
            // Image par défaut si oubliée dans le JSON
            strcpy(t->image_path, "assets/sprites/armors/default.png");
        }
        
        // On charge l'image en mémoire !
        t->sprite = LoadTexture(t->image_path);
        
        g_itemCount++;
    }
    cJSON_Delete(json);
    UnloadFileText(buffer);
}

/**
 * @brief Charge les bases de données de magie (sorts et potions).
 * @details Remplit g_spellDB, g_spellCount, g_potionDB et g_potionCount.
 * @param[in] spell_path Chemin vers le fichier JSON des sorts
 * @param[in] potion_path Chemin vers le fichier JSON des potions
 * @note Fonction privée
 */
static void LoadMagicDB(const char* spell_path, const char* potion_path)
{
    // --- 1. CHARGEMENT DES SORTS ---
    char* s_buf = LoadFileText(spell_path);
    if (s_buf)
    {
        cJSON* json = cJSON_Parse(s_buf);
        if (json)
        {
            cJSON* arr = cJSON_GetObjectItemCaseSensitive(json, "spells");
            cJSON* item;
            g_spellCount = 0;
            cJSON_ArrayForEach(item, arr)
            {
                if (g_spellCount >= MAX_SPELLS_DB)
                    break;
                SpellTemplate* t = &g_spellDB[g_spellCount];

                strcpy(t->id, cJSON_GetObjectItem(item, "id")->valuestring);
                strcpy(t->name_en, cJSON_GetObjectItem(item, "name_en")->valuestring);
                strcpy(t->name_fr, cJSON_GetObjectItem(item, "name_fr")->valuestring);
                t->type = ParseEffectType(cJSON_GetObjectItem(item, "type")->valuestring);

                t->base_val = cJSON_GetObjectItem(item, "base_val")->valueint;
                t->inc_val  = cJSON_GetObjectItem(item, "inc_val")->valueint;

                // base_dur et inc_dur peuvent ne pas exister pour les sorts de dégâts directs
                cJSON* bd   = cJSON_GetObjectItem(item, "base_dur");
                t->base_dur = bd ? bd->valuedouble : 0.0f;
                cJSON* idur = cJSON_GetObjectItem(item, "inc_dur");
                t->inc_dur  = idur ? idur->valuedouble : 0.0f;

                t->mana_cost     = cJSON_GetObjectItem(item, "mana_cost")->valueint;
                t->learn_gold    = cJSON_GetObjectItem(item, "learn_gold")->valueint;
                t->learn_crystal = cJSON_GetObjectItem(item, "learn_crystal")->valueint;
                t->upg_gold_base = cJSON_GetObjectItem(item, "upg_gold_base")->valueint;
                t->upg_gold_inc  = cJSON_GetObjectItem(item, "upg_gold_inc")->valueint;
                t->prep_crystal  = cJSON_GetObjectItem(item, "prep_crystal")->valueint;

                g_spellCount++;
            }
            cJSON_Delete(json);
        }
        UnloadFileText(s_buf);
    }

    // --- 2. CHARGEMENT DES POTIONS ---
    char* p_buf = LoadFileText(potion_path);
    if (p_buf)
    {
        cJSON* json = cJSON_Parse(p_buf);
        if (json)
        {
            cJSON* arr = cJSON_GetObjectItemCaseSensitive(json, "potions");
            cJSON* item;
            g_potionCount = 0;
            cJSON_ArrayForEach(item, arr)
            {
                if (g_potionCount >= MAX_POTIONS_DB)
                    break;
                PotionTemplate* t = &g_potionDB[g_potionCount];

                strcpy(t->id, cJSON_GetObjectItem(item, "id")->valuestring);
                strcpy(t->name_en, cJSON_GetObjectItem(item, "name_en")->valuestring);
                strcpy(t->name_fr, cJSON_GetObjectItem(item, "name_fr")->valuestring);
                t->type = ParseEffectType(cJSON_GetObjectItem(item, "type")->valuestring);

                t->base_val = cJSON_GetObjectItem(item, "base_val")->valueint;
                t->inc_val  = cJSON_GetObjectItem(item, "inc_val")->valueint;

                t->learn_gold       = cJSON_GetObjectItem(item, "learn_gold")->valueint;
                t->upg_gold_base    = cJSON_GetObjectItem(item, "upg_gold_base")->valueint;
                t->upg_gold_inc     = cJSON_GetObjectItem(item, "upg_gold_inc")->valueint;
                t->craft_herbs_base = cJSON_GetObjectItem(item, "craft_herbs_base")->valueint;
                t->craft_herbs_inc  = cJSON_GetObjectItem(item, "craft_herbs_inc")->valueint;

                g_potionCount++;
            }
            cJSON_Delete(json);
        }
        UnloadFileText(p_buf);
    }
    printf("--- MAGIE CHARGEE : %d sorts, %d potions ---\n", g_spellCount, g_potionCount);
}

/**
 * @brief Charge les données du donjon (ambiance et salles).
 * @details Remplit les tableaux globaux d'ambiance et d'événements.
 * @param[in] ambiance_path Chemin vers le fichier JSON des ambiances
 * @param[in] rooms_path Chemin vers le fichier JSON des salles
 * @note Fonction privée
 */
static void LoadDungeonDB(const char* ambiance_path, const char* rooms_path)
{
    // 1. Charger l'ambiance
    char* buf1 = LoadFileText(ambiance_path);
    if (buf1)
    {
        cJSON* json = cJSON_Parse(buf1);
        cJSON* arr  = cJSON_GetObjectItemCaseSensitive(json, "ambiance");
        cJSON* item;
        g_ambianceCount = 0;
        cJSON_ArrayForEach(item, arr)
        {
            if (g_ambianceCount >= MAX_AMBIANCE)
                break;
            strcpy(g_ambiance_en[g_ambianceCount], cJSON_GetObjectItem(item, "en")->valuestring);
            strcpy(g_ambiance_fr[g_ambianceCount], cJSON_GetObjectItem(item, "fr")->valuestring);
            g_ambianceCount++;
        }
        cJSON_Delete(json);
        UnloadFileText(buf1);
    }

    // 2. Charger les salles d'événements
    char* buf2 = LoadFileText(rooms_path);
    if (buf2)
    {
        cJSON* json = cJSON_Parse(buf2);
        cJSON* arr  = cJSON_GetObjectItemCaseSensitive(json, "rooms");
        cJSON* item;
        g_eventCount = 0;
        cJSON_ArrayForEach(item, arr)
        {
            if (g_eventCount >= MAX_EVENTS)
                break;
            EventRoomTemplate* t = &g_eventDB[g_eventCount];
            strcpy(t->id, cJSON_GetObjectItem(item, "id")->valuestring);
            strcpy(t->name_en, cJSON_GetObjectItem(item, "name_en")->valuestring);
            strcpy(t->name_fr, cJSON_GetObjectItem(item, "name_fr")->valuestring);
            strcpy(t->type, cJSON_GetObjectItem(item, "type")->valuestring);
            t->amount = cJSON_GetObjectItem(item, "amount")->valueint;
            strcpy(t->flavor_en, cJSON_GetObjectItem(item, "flavor_en")->valuestring);
            strcpy(t->flavor_fr, cJSON_GetObjectItem(item, "flavor_fr")->valuestring);
            // Ascii art
            cJSON* asciiArray   = cJSON_GetObjectItem(item, "ascii");
            cJSON* line         = NULL;
            t->ascii_line_count = 0;
            if (asciiArray)
            {
                cJSON_ArrayForEach(line, asciiArray)
                {
                    if (t->ascii_line_count < MAX_ASCII_LINES)
                    {
                        strcpy(t->ascii[t->ascii_line_count], line->valuestring);
                        t->ascii_line_count++;
                    }
                }
            }

            g_eventCount++;
        }
        cJSON_Delete(json);
        UnloadFileText(buf2);
    }
}

/**
 * @brief Convertit une chaîne en type d'effet.
 * @details Analyse une chaîne de caractères pour déterminer le type de sort associé.
 * @param[in] str Chaîne représentant le type d'effet
 * @return Type d'effet correspondant
 * @note Fonction privée
 */
static EffectType ParseEffectType(const char* str)
{
    if (strstr(str, "DAMAGE"))
        return SPELL_DAMAGE;
    if (strstr(str, "HEAL"))
        return SPELL_HEAL;
    if (strstr(str, "POISON"))
        return SPELL_POISON;
    if(strstr(str,"MANA"))
        return SPELL_MANA;
    if (strstr(str, "FREEZE"))
        return SPELL_FREEZE;
    if (strstr(str, "VAMPIRISM"))
        return SPELL_VAMPIRISM;
    if (strstr(str, "STUN"))
        return SPELL_STUN;
    if (strstr(str, "MANA"))
        return SPELL_HEAL; // Simplification
    return SPELL_DAMAGE;
}
