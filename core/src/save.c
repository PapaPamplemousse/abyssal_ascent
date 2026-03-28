#include "save.h"
#include "cJSON.h"
#include "lang.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern bool  g_isEnglish;
extern bool  g_camp_fire_lit;
extern float g_camp_fire_timer;

static void newgamestat(GameContext* game);

/**
 * @brief Charge un tableau d'entiers depuis un objet JSON vers un tableau C.
 * @details Parcourt un tableau JSON et copie ses valeurs dans un tableau C
 *          en respectant la taille maximale fournie.
 * @param[in] jsonName Nom de la clé JSON contenant le tableau
 * @param[out] cArray Tableau C de destination
 * @param[in] maxSize Taille maximale du tableau C
 * @note Macro publique
 * @warning Ne vérifie pas le type des éléments JSON (doit être des entiers)
 */
#define LOAD_INT_ARRAY(jsonName, cArray, maxSize)                                                                                                                                                                          \
    do                                                                                                                                                                                                                     \
    {                                                                                                                                                                                                                      \
        cJSON* arr = cJSON_GetObjectItem(root, jsonName);                                                                                                                                                                  \
        if (arr)                                                                                                                                                                                                           \
        {                                                                                                                                                                                                                  \
            int    i = 0;                                                                                                                                                                                                  \
            cJSON* item;                                                                                                                                                                                                   \
            cJSON_ArrayForEach(item, arr)                                                                                                                                                                                  \
            {                                                                                                                                                                                                              \
                if (i < maxSize)                                                                                                                                                                                           \
                {                                                                                                                                                                                                          \
                    cArray[i] = item->valueint;                                                                                                                                                                            \
                    i++;                                                                                                                                                                                                   \
                }                                                                                                                                                                                                          \
            }                                                                                                                                                                                                              \
        }                                                                                                                                                                                                                  \
    } while (0)

void SaveGame(GameContext* game, DungeonContext* dungeon)
{
    cJSON* root = cJSON_CreateObject();
    if (!root)
        return;

    cJSON_AddBoolToObject(root, "is_english", g_isEnglish);

    // On utilise le pointeur dungeon au lieu de la variable statique
    cJSON_AddNumberToObject(root, "highest_floor_curr", dungeon->highest_floor_curr);
    cJSON_AddNumberToObject(root, "highest_floor_all", dungeon->highest_floor_all_time);
    

    cJSON_AddNumberToObject(root, "player_level", game->combat.player.level);
    cJSON_AddNumberToObject(root, "player_xp", game->combat.player.xp);
    cJSON_AddNumberToObject(root, "player_max_xp", game->combat.player.max_xp);
    cJSON_AddNumberToObject(root, "player_hp", game->combat.player.hp);
    cJSON_AddNumberToObject(root, "player_mana", game->combat.player.mana);

    cJSON_AddBoolToObject(root, "camp_fire_lit", g_camp_fire_lit);
    cJSON_AddNumberToObject(root, "camp_fire_timer", g_camp_fire_timer);

    // Sauvegarde des Ressources
    cJSON_AddNumberToObject(root, "fer", game->clicker.inventory.fer);
    cJSON_AddNumberToObject(root, "or", game->clicker.inventory.or);
    cJSON_AddNumberToObject(root, "cristaux", game->clicker.inventory.cristaux);
    cJSON_AddNumberToObject(root, "bois", game->clicker.inventory.bois);
    cJSON_AddNumberToObject(root, "viande", game->clicker.inventory.viande);
    cJSON_AddNumberToObject(root, "herbes", game->clicker.inventory.herbes);

    cJSON_AddBoolToObject(root, "unlock_or", game->clicker.inventory.unlock_or);
    cJSON_AddBoolToObject(root, "unlock_bois", game->clicker.inventory.unlock_bois);
    cJSON_AddBoolToObject(root, "unlock_cristaux", game->clicker.inventory.unlock_cristaux);
    cJSON_AddBoolToObject(root, "unlock_viande", game->clicker.inventory.unlock_viande);

    cJSON_AddNumberToObject(root, "boss_souls", game->combat.player.boss_souls);
    cJSON_AddNumberToObject(root, "passive_hp", game->combat.player.passive_hp_level);
    cJSON_AddNumberToObject(root, "passive_atk", game->combat.player.passive_atk_level);
    cJSON_AddNumberToObject(root, "passive_loot", game->combat.player.passive_loot_level);
    cJSON_AddNumberToObject(root, "passive_mana", game->combat.player.passive_mana_level);

    cJSON_AddNumberToObject(root, "monsters_killed", game->combat.monsters_killed);

#define SAVE_INT_ARRAY(name, arr, size)                                                                                                                                                                                    \
    do                                                                                                                                                                                                                     \
    {                                                                                                                                                                                                                      \
        cJSON* jArr = cJSON_CreateArray();                                                                                                                                                                                 \
        if (jArr)                                                                                                                                                                                                          \
        {                                                                                                                                                                                                                  \
            for (int i = 0; i < size; i++)                                                                                                                                                                                 \
                cJSON_AddItemToArray(jArr, cJSON_CreateNumber(arr[i]));                                                                                                                                                    \
            cJSON_AddItemToObject(root, name, jArr);                                                                                                                                                                       \
        }                                                                                                                                                                                                                  \
    } while (0)

    SAVE_INT_ARRAY("b_fer", game->clicker.inventory.b_fer, 4);
    SAVE_INT_ARRAY("b_or", game->clicker.inventory.b_or, 4);
    SAVE_INT_ARRAY("b_cristaux", game->clicker.inventory.b_cristaux, 4);
    SAVE_INT_ARRAY("b_herbes", game->clicker.inventory.b_herbes, 4);
    SAVE_INT_ARRAY("b_bois", game->clicker.inventory.b_bois, 4);
    SAVE_INT_ARRAY("b_viande", game->clicker.inventory.b_viande, 4);

    cJSON* sp_unl = cJSON_CreateArray();
    cJSON* sp_lvl = cJSON_CreateArray();
    if (sp_unl && sp_lvl)
    {
        for (int i = 0; i < g_spellCount; i++)
        {
            cJSON_AddItemToArray(sp_unl, cJSON_CreateBool(game->combat.player.spell_unlocked[i]));
            cJSON_AddItemToArray(sp_lvl, cJSON_CreateNumber(game->combat.player.spell_level[i]));
        }
        cJSON_AddItemToObject(root, "spell_unlocked", sp_unl);
        cJSON_AddItemToObject(root, "spell_level", sp_lvl);
    }

    cJSON* po_unl = cJSON_CreateArray();
    cJSON* po_lvl = cJSON_CreateArray();
    cJSON* po_qty = cJSON_CreateArray();
    if (po_unl && po_lvl && po_qty)
    {
        for (int i = 0; i < g_potionCount; i++)
        {
            cJSON_AddItemToArray(po_unl, cJSON_CreateBool(game->combat.player.potion_unlocked[i]));
            cJSON_AddItemToArray(po_lvl, cJSON_CreateNumber(game->combat.player.potion_level[i]));
            cJSON_AddItemToArray(po_qty, cJSON_CreateNumber(game->combat.player.potion_qty[i]));
        }
        cJSON_AddItemToObject(root, "potion_unlocked", po_unl);
        cJSON_AddItemToObject(root, "potion_level", po_lvl);
        cJSON_AddItemToObject(root, "potion_qty", po_qty);
    }

    cJSON* eq_sp = cJSON_CreateArray();
    cJSON* eq_po = cJSON_CreateArray();
    if (eq_sp && eq_po)
    {
        for (int i = 0; i < 3; i++)
        {
            cJSON_AddItemToArray(eq_sp, cJSON_CreateNumber(game->combat.player.equipped_spells[i]));
            cJSON_AddItemToArray(eq_po, cJSON_CreateNumber(game->combat.player.equipped_potions[i]));
        }
        cJSON_AddItemToObject(root, "equipped_spells", eq_sp);
        cJSON_AddItemToObject(root, "equipped_potions", eq_po);
    }

    int save_inv_count = (game->currentState == STATE_DUNGEON || game->currentState == STATE_GAMEOVER) ? game->combat.player.inventory_safe_count : game->combat.player.inventory_count;

    if (save_inv_count < 0)
        save_inv_count = 0;
    if (save_inv_count > MAX_INVENTORY)
        save_inv_count = MAX_INVENTORY;

    cJSON_AddNumberToObject(root, "inventory_count", save_inv_count);

    cJSON* inv_arr = cJSON_CreateArray();
    if (inv_arr)
    {
        for (int i = 0; i < save_inv_count; i++)
        {
            cJSON* itemObj = cJSON_CreateObject();
            if (itemObj)
            {
                cJSON_AddNumberToObject(itemObj, "template_idx", game->combat.player.inventory[i].template_idx);
                cJSON_AddNumberToObject(itemObj, "level", game->combat.player.inventory[i].level);
                cJSON_AddNumberToObject(itemObj, "effect", game->combat.player.inventory[i].effect);
                cJSON_AddNumberToObject(itemObj, "rarity", game->combat.player.inventory[i].rarity);
                cJSON_AddItemToArray(inv_arr, itemObj);
            }
        }
        cJSON_AddItemToObject(root, "inventory", inv_arr);
    }

    cJSON* eq_arr = cJSON_CreateArray();
    if (eq_arr)
    {
        for (int i = 0; i < MAX_SLOTS; i++)
        {
            cJSON_AddItemToArray(eq_arr, cJSON_CreateNumber(game->combat.player.equipped[i]));
        }
        cJSON_AddItemToObject(root, "equipped", eq_arr);
    }

    char* jsonStr = cJSON_Print(root);
    if (jsonStr)
    {
        SaveFileText("save.json", jsonStr);
        free(jsonStr);
    }
    cJSON_Delete(root);

#undef SAVE_INT_ARRAY
}

void LoadGame(GameContext* game, DungeonContext* dungeon)
{
    // =========================================================
    // 1. SÉCURITÉ : FORMATAGE DE LA MÉMOIRE DE DEPART
    // =========================================================

    newgamestat(game);

    // =========================================================
    // =========================================================
    // 2. LECTURE DU FICHIER DE SAUVEGARDE
    // =========================================================
    char* file = LoadFileText("save.json");
    if (!file)
        return; // Si pas de sauvegarde, on garde nos zéros bien propres !

    cJSON* root = cJSON_Parse(file);
    if (!root)
    {
        UnloadFileText(file);
        return;
    }

    cJSON* langNode = cJSON_GetObjectItem(root, "is_english");
    if (langNode)
        g_isEnglish = cJSON_IsTrue(langNode);

    cJSON* hf = cJSON_GetObjectItem(root, "highest_floor_curr");
    if (hf)
        dungeon->highest_floor_curr = hf->valueint;

    cJSON* hfa = cJSON_GetObjectItem(root, "highest_floor_all");
    if (hfa)
        dungeon->highest_floor_all_time = hfa->valueint;

        

    cJSON* lvlNode = cJSON_GetObjectItem(root, "player_level");
    if (lvlNode)
        game->combat.player.level = lvlNode->valueint;
    cJSON* xpNode = cJSON_GetObjectItem(root, "player_xp");
    if (xpNode)
        game->combat.player.xp = xpNode->valueint;
    cJSON* mxpNode = cJSON_GetObjectItem(root, "player_max_xp");
    if (mxpNode)
        game->combat.player.max_xp = mxpNode->valueint;
    cJSON* hpNode = cJSON_GetObjectItem(root, "player_hp");
    if (hpNode)
        game->combat.player.hp = hpNode->valueint;
    cJSON* manaNode = cJSON_GetObjectItem(root, "player_mana");
    if (manaNode)
        game->combat.player.mana = manaNode->valueint;

    cJSON* fireLitNode = cJSON_GetObjectItem(root, "camp_fire_lit");
    if (fireLitNode)
        g_camp_fire_lit = cJSON_IsTrue(fireLitNode);

    cJSON* fireTimerNode = cJSON_GetObjectItem(root, "camp_fire_timer");
    if (fireTimerNode)
        g_camp_fire_timer = fireTimerNode->valuedouble;
    // --- Chargement des Passifs (propre, sans les else devenus inutiles) ---
    cJSON* bsNode = cJSON_GetObjectItem(root, "boss_souls");
    if (bsNode)
        game->combat.player.boss_souls = bsNode->valueint;
    cJSON* phpNode = cJSON_GetObjectItem(root, "passive_hp");
    if (phpNode)
        game->combat.player.passive_hp_level = phpNode->valueint;
    cJSON* patkNode = cJSON_GetObjectItem(root, "passive_atk");
    if (patkNode)
        game->combat.player.passive_atk_level = patkNode->valueint;
    cJSON* plootNode = cJSON_GetObjectItem(root, "passive_loot");
    if (plootNode)
        game->combat.player.passive_loot_level = plootNode->valueint;
    cJSON* pmanaNode = cJSON_GetObjectItem(root, "passive_mana");
    if (pmanaNode)
        game->combat.player.passive_mana_level = pmanaNode->valueint;

    // --- Chargement des Ressources ---
    cJSON* fNode = cJSON_GetObjectItem(root, "fer");
    if (fNode)
        game->clicker.inventory.fer = fNode->valueint;
    cJSON* oNode = cJSON_GetObjectItem(root, "or");
    if (oNode)
        game->clicker.inventory.or = oNode->valueint;
    cJSON* cNode = cJSON_GetObjectItem(root, "cristaux");
    if (cNode)
        game->clicker.inventory.cristaux = cNode->valueint;
    cJSON* bNode = cJSON_GetObjectItem(root, "bois");
    if (bNode)
        game->clicker.inventory.bois = bNode->valueint;
    cJSON* vNode = cJSON_GetObjectItem(root, "viande");
    if (vNode)
        game->clicker.inventory.viande = vNode->valueint;
    cJSON* hNode = cJSON_GetObjectItem(root, "herbes");
    if (hNode)
        game->clicker.inventory.herbes = hNode->valueint;

    game->clicker.inventory.unlock_or       = cJSON_IsTrue(cJSON_GetObjectItem(root, "unlock_or"));
    game->clicker.inventory.unlock_bois     = cJSON_IsTrue(cJSON_GetObjectItem(root, "unlock_bois"));
    game->clicker.inventory.unlock_cristaux = cJSON_IsTrue(cJSON_GetObjectItem(root, "unlock_cristaux"));
    game->clicker.inventory.unlock_viande   = cJSON_IsTrue(cJSON_GetObjectItem(root, "unlock_viande"));

    game->combat.monsters_killed = cJSON_IsTrue(cJSON_GetObjectItem(root, "monsters_killed"));

    LOAD_INT_ARRAY("b_fer", game->clicker.inventory.b_fer, 4);
    LOAD_INT_ARRAY("b_or", game->clicker.inventory.b_or, 4);
    LOAD_INT_ARRAY("b_cristaux", game->clicker.inventory.b_cristaux, 4);
    LOAD_INT_ARRAY("b_herbes", game->clicker.inventory.b_herbes, 4);
    LOAD_INT_ARRAY("b_bois", game->clicker.inventory.b_bois, 4);
    LOAD_INT_ARRAY("b_viande", game->clicker.inventory.b_viande, 4);

    LOAD_INT_ARRAY("spell_unlocked", game->combat.player.spell_unlocked, MAX_SPELLS_DB);
    LOAD_INT_ARRAY("spell_level", game->combat.player.spell_level, MAX_SPELLS_DB);
    LOAD_INT_ARRAY("potion_unlocked", game->combat.player.potion_unlocked, MAX_POTIONS_DB);
    LOAD_INT_ARRAY("potion_level", game->combat.player.potion_level, MAX_POTIONS_DB);
    LOAD_INT_ARRAY("potion_qty", game->combat.player.potion_qty, MAX_POTIONS_DB);
    LOAD_INT_ARRAY("equipped_spells", game->combat.player.equipped_spells, 3);
    LOAD_INT_ARRAY("equipped_potions", game->combat.player.equipped_potions, 3);

    // --- Chargement de l'Inventaire Physique ---
    cJSON* inv_count_node = cJSON_GetObjectItem(root, "inventory_count");
    if (inv_count_node)
    {
        int count = inv_count_node->valueint;
        if (count < 0)
            count = 0;
        if (count > MAX_INVENTORY)
            count = MAX_INVENTORY;
        game->combat.player.inventory_count      = count;
        game->combat.player.inventory_safe_count = count;

        cJSON* inv_arr = cJSON_GetObjectItem(root, "inventory");
        if (inv_arr)
        {
            int    i        = 0;
            cJSON* itemNode = NULL;
            cJSON_ArrayForEach(itemNode, inv_arr)
            {
                if (i < count)
                {
                    cJSON* tNode = cJSON_GetObjectItem(itemNode, "template_idx");
                    cJSON* lNode = cJSON_GetObjectItem(itemNode, "level");
                    cJSON* rNode = cJSON_GetObjectItem(itemNode, "rarity");
                    cJSON* eNode = cJSON_GetObjectItem(itemNode, "effect");

                    game->combat.player.inventory[i].template_idx = tNode ? tNode->valueint : 0;
                    game->combat.player.inventory[i].level        = lNode ? lNode->valueint : 0;
                    game->combat.player.inventory[i].rarity       = rNode ? rNode->valueint : 0;
                    game->combat.player.inventory[i].effect       = eNode ? eNode->valueint : 0;
                    i++;
                }
            }
        }
    }

    LOAD_INT_ARRAY("equipped", game->combat.player.equipped, MAX_SLOTS);

    cJSON_Delete(root);
    UnloadFileText(file);

    // Et on applique tout ça proprement !
    Combat_RecalculateStats(&game->combat);

    if (game->combat.player.hp > game->combat.player.max_hp) 
        game->combat.player.hp = game->combat.player.max_hp;
        
    if (game->combat.player.mana > game->combat.player.max_mana) 
        game->combat.player.mana = game->combat.player.max_mana;
}

static void newgamestat(GameContext* game)
{
    // On met TOUTE la structure du joueur à ZÉRO (inventaire, passifs, sorts, TOUT).
    memset(&game->combat.player, 0, sizeof(game->combat.player));

    // On définit les statistiques de base d'une "Nouvelle Partie"
    game->combat.player.level  = 1;
    game->combat.player.xp     = 0;
    game->combat.player.max_xp = 100;

    game->combat.player.base_max_hp   = 50;
    game->combat.player.hp            = 50;
    game->combat.player.base_atk      = 5;
    game->combat.player.base_max_mana = 20;
    game->combat.player.mana          = 20;
    game->combat.player.base_spd      = 0.8f;

    game->combat.monsters_killed = 0;

    // Initialisation des emplacements "Vides" à -1 (car 0 = le premier objet/sort)
    for (int i = 0; i < MAX_SLOTS; i++)
    {
        game->combat.player.equipped[i] = -1;
    }
    for (int i = 0; i < 3; i++)
    {
        game->combat.player.equipped_spells[i]  = -1;
        game->combat.player.equipped_potions[i] = -1;
    }

    // On sécurise aussi le clicker au cas où !
    memset(&game->clicker.inventory, 0, sizeof(game->clicker.inventory));

    // au cas ou la lecture du json plante on recalcul ici
    Combat_RecalculateStats(&game->combat);
}