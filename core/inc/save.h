#ifndef SAVE_H
#define SAVE_H

#include "game.h"
#include "../../dungeon/inc/dungeon.h"

/**
 * @brief Sauvegarde l'état du jeu et du donjon.
 * @details Sérialise les données contenues dans les contextes Game et Dungeon.
 * @param[in] game Contexte du jeu à sauvegarder
 * @param[in] dungeon Contexte du donjon à sauvegarder
 * @note Fonction publique
 */
void SaveGame(GameContext* game, DungeonContext* dungeon);

/**
 * @brief Charge l'état du jeu et du donjon.
 * @details Désérialise les données depuis une sauvegarde vers les contextes fournis.
 * @param[out] game Contexte du jeu à remplir
 * @param[out] dungeon Contexte du donjon à remplir
 * @note Fonction publique
 */
void LoadGame(GameContext* game, DungeonContext* dungeon);

#endif // SAVE_H