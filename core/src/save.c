#include "save.h"
#include "../../utils/inc/cJSON.h"
#include "../../utils/inc/lang.h"
#include <stdlib.h>
#include <stdio.h>

extern bool g_isEnglish;

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
#define LOAD_INT_ARRAY(jsonName, cArray, maxSize) \
        do { \
            cJSON* arr = cJSON_GetObjectItem(root, jsonName); \
            if (arr) { \
                int i = 0; cJSON* item; \
                cJSON_ArrayForEach(item, arr) { \
                    if (i < maxSize) { cArray[i] = item->valueint; i++; } \
                } \
            } \
        } while(0)


void SaveGame(GameContext* game, DungeonContext* dungeon)
{
    cJSON* root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddBoolToObject(root, "is_english", g_isEnglish);
    
    // On utilise le pointeur dungeon au lieu de la variable statique
    cJSON_AddNumberToObject(root, "highest_floor", dungeon->highest_floor);

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

    #define SAVE_INT_ARRAY(name, arr, size) \
        do { \
            cJSON* jArr = cJSON_CreateArray(); \
            if(jArr) { \
                for(int i=0; i<size; i++) cJSON_AddItemToArray(jArr, cJSON_CreateNumber(arr[i])); \
                cJSON_AddItemToObject(root, name, jArr); \
            } \
        } while(0)

    SAVE_INT_ARRAY("b_fer", game->clicker.inventory.b_fer, 4);
    SAVE_INT_ARRAY("b_or", game->clicker.inventory.b_or, 4);
    SAVE_INT_ARRAY("b_cristaux", game->clicker.inventory.b_cristaux, 4);
    SAVE_INT_ARRAY("b_herbes", game->clicker.inventory.b_herbes, 4);
    SAVE_INT_ARRAY("b_bois", game->clicker.inventory.b_bois, 4);
    SAVE_INT_ARRAY("b_viande", game->clicker.inventory.b_viande, 4);

    cJSON* sp_unl = cJSON_CreateArray();
    cJSON* sp_lvl = cJSON_CreateArray();
    if(sp_unl && sp_lvl) {
        for(int i = 0; i < g_spellCount; i++) {
            cJSON_AddItemToArray(sp_unl, cJSON_CreateBool(game->combat.player.spell_unlocked[i]));
            cJSON_AddItemToArray(sp_lvl, cJSON_CreateNumber(game->combat.player.spell_level[i]));
        }
        cJSON_AddItemToObject(root, "spell_unlocked", sp_unl);
        cJSON_AddItemToObject(root, "spell_level", sp_lvl);
    }

    cJSON* po_unl = cJSON_CreateArray();
    cJSON* po_lvl = cJSON_CreateArray();
    cJSON* po_qty = cJSON_CreateArray();
    if(po_unl && po_lvl && po_qty) {
        for(int i = 0; i < g_potionCount; i++) {
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
    if(eq_sp && eq_po) {
        for(int i = 0; i < 3; i++) {
            cJSON_AddItemToArray(eq_sp, cJSON_CreateNumber(game->combat.player.equipped_spells[i]));
            cJSON_AddItemToArray(eq_po, cJSON_CreateNumber(game->combat.player.equipped_potions[i]));
        }
        cJSON_AddItemToObject(root, "equipped_spells", eq_sp);
        cJSON_AddItemToObject(root, "equipped_potions", eq_po);
    }

    int save_inv_count = (game->currentState == STATE_DUNGEON || game->currentState == STATE_GAMEOVER) 
                         ? game->combat.player.inventory_safe_count 
                         : game->combat.player.inventory_count;

    if (save_inv_count < 0) save_inv_count = 0;
    if (save_inv_count > MAX_INVENTORY) save_inv_count = MAX_INVENTORY;

    cJSON_AddNumberToObject(root, "inventory_count", save_inv_count);
    
    cJSON* inv_arr = cJSON_CreateArray();
    if (inv_arr) {
        for (int i = 0; i < save_inv_count; i++) { 
            cJSON* itemObj = cJSON_CreateObject();
            if(itemObj) {
                cJSON_AddNumberToObject(itemObj, "template_idx", game->combat.player.inventory[i].template_idx);
                cJSON_AddNumberToObject(itemObj, "level", game->combat.player.inventory[i].level);
                cJSON_AddNumberToObject(itemObj, "effect", game->combat.player.inventory[i].effect); 
                cJSON_AddItemToArray(inv_arr, itemObj);
            }
        }
        cJSON_AddItemToObject(root, "inventory", inv_arr);
    }

    cJSON* eq_arr = cJSON_CreateArray();
    if (eq_arr) {
        for (int i = 0; i < MAX_SLOTS; i++) {
            cJSON_AddItemToArray(eq_arr, cJSON_CreateNumber(game->combat.player.equipped[i]));
        }
        cJSON_AddItemToObject(root, "equipped", eq_arr);
    }

    char* jsonStr = cJSON_Print(root);
    if (jsonStr) {
        SaveFileText("save.json", jsonStr); 
        free(jsonStr);
    }
    cJSON_Delete(root);
    
    #undef SAVE_INT_ARRAY
}

void LoadGame(GameContext* game, DungeonContext* dungeon)
{
    // Sécurité : Initialisation de base
    for(int i = 0; i < MAX_POTIONS_DB; i++) {
        game->combat.player.potion_unlocked[i] = false;
        game->combat.player.potion_level[i] = 0;
        game->combat.player.potion_qty[i] = 0;
    }
    for(int i = 0; i < MAX_SPELLS_DB; i++) {
        game->combat.player.spell_unlocked[i] = false;
        game->combat.player.spell_level[i] = 0;
    }
    for(int i = 0; i < 3; i++) {
        game->combat.player.equipped_spells[i] = -1;
        game->combat.player.equipped_potions[i] = -1;
    }
    for(int i = 0; i < MAX_SLOTS; i++) {
        game->combat.player.equipped[i] = -1;
    }
    game->combat.player.inventory_count = 0;
    game->combat.player.inventory_safe_count = 0;

    char* file = LoadFileText("save.json");
    if (!file) return;

    cJSON* root = cJSON_Parse(file);
    if (!root) { UnloadFileText(file); return; }

    cJSON* langNode = cJSON_GetObjectItem(root, "is_english");
    if (langNode) g_isEnglish = cJSON_IsTrue(langNode);

    // On utilise le pointeur dungeon
    cJSON* hf = cJSON_GetObjectItem(root, "highest_floor");
    if (hf) dungeon->highest_floor = hf->valueint;

    cJSON* fNode = cJSON_GetObjectItem(root, "fer"); if(fNode) game->clicker.inventory.fer = fNode->valueint;
    cJSON* oNode = cJSON_GetObjectItem(root, "or"); if(oNode) game->clicker.inventory.or = oNode->valueint;
    cJSON* cNode = cJSON_GetObjectItem(root, "cristaux"); if(cNode) game->clicker.inventory.cristaux = cNode->valueint;
    cJSON* bNode = cJSON_GetObjectItem(root, "bois"); if(bNode) game->clicker.inventory.bois = bNode->valueint;
    cJSON* vNode = cJSON_GetObjectItem(root, "viande"); if(vNode) game->clicker.inventory.viande = vNode->valueint;
    cJSON* hNode = cJSON_GetObjectItem(root, "herbes"); if(hNode) game->clicker.inventory.herbes = hNode->valueint;

    game->clicker.inventory.unlock_or       = cJSON_IsTrue(cJSON_GetObjectItem(root, "unlock_or"));
    game->clicker.inventory.unlock_bois     = cJSON_IsTrue(cJSON_GetObjectItem(root, "unlock_bois"));
    game->clicker.inventory.unlock_cristaux = cJSON_IsTrue(cJSON_GetObjectItem(root, "unlock_cristaux"));
    game->clicker.inventory.unlock_viande   = cJSON_IsTrue(cJSON_GetObjectItem(root, "unlock_viande"));

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

    cJSON* inv_count_node = cJSON_GetObjectItem(root, "inventory_count");
    if (inv_count_node) {
        int count = inv_count_node->valueint;
        if(count < 0) count = 0;
        if(count > MAX_INVENTORY) count = MAX_INVENTORY;
        game->combat.player.inventory_count = count;
        game->combat.player.inventory_safe_count = count;

        cJSON* inv_arr = cJSON_GetObjectItem(root, "inventory");
        if (inv_arr) {
            int i = 0;
            cJSON* itemNode = NULL;
            cJSON_ArrayForEach(itemNode, inv_arr) {
                if (i < count) {
                    cJSON* tNode = cJSON_GetObjectItem(itemNode, "template_idx");
                    cJSON* lNode = cJSON_GetObjectItem(itemNode, "level");
                    cJSON* eNode = cJSON_GetObjectItem(itemNode, "effect");
                    
                    game->combat.player.inventory[i].template_idx = tNode ? tNode->valueint : 0;
                    game->combat.player.inventory[i].level = lNode ? lNode->valueint : 0;
                    game->combat.player.inventory[i].effect = eNode ? eNode->valueint : 0;
                    i++;
                }
            }
        }
    }

    LOAD_INT_ARRAY("equipped", game->combat.player.equipped, MAX_SLOTS);

    cJSON_Delete(root);
    UnloadFileText(file);

    Combat_RecalculateStats(&game->combat);
}