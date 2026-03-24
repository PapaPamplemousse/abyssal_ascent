#ifndef CAMP_MENUS_H
#define CAMP_MENUS_H

#include "game.h"

/**
 * @brief Affiche l'écran d'inventaire du joueur.
 * @details Permet de visualiser l'équipement et d'équiper/déséquiper des objets.
 * @param[in,out] game Contexte du jeu
 * @param[in] w Largeur de l'écran
 * @param[in] h Hauteur de l'écran
 * @note Fonction publique
 */
void Game_RenderInventory(GameContext* game, int w, int h);

/**
 * @brief Affiche l'écran de la forge.
 * @details Permet de sélectionner et améliorer un équipement.
 * @param[in,out] game Contexte du jeu
 * @param[in] w Largeur de l'écran
 * @param[in] h Hauteur de l'écran
 * @note Fonction publique
 */
void Game_RenderForge(GameContext* game, int w, int h);

/**
 * @brief Affiche l'écran de l'archiforge (gestion des sorts).
 * @details Permet d'apprendre, améliorer et équiper des sorts.
 * @param[in,out] game Contexte du jeu
 * @param[in] w Largeur de l'écran
 * @param[in] h Hauteur de l'écran
 * @note Fonction publique
 */
void Game_RenderArchiforge(GameContext* game, int w, int h);

/**
 * @brief Affiche l'écran de l'alchimiste.
 * @details Permet d'apprendre, améliorer, fabriquer et équiper des potions.
 * @param[in,out] game Contexte du jeu
 * @param[in] w Largeur de l'écran
 * @param[in] h Hauteur de l'écran
 * @note Fonction publique
 */
void Game_RenderAlchemist(GameContext* game, int w, int h);

#endif // CAMP_MENUS_H