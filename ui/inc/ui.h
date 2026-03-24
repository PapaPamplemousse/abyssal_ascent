#ifndef UI_H
#define UI_H

#include "raylib.h"
#include "game.h"
#include "dungeon.h"

/**
 * @brief Dessine l'interface utilisateur complète du jeu.
 * @details Affiche les stats, ressources, mini-carte, équipement et logs de combat.
 * @param[in] game Contexte du jeu
 * @param[in] dungeon Contexte du donjon (peut être NULL)
 * @param[in] w Largeur de l'écran
 * @param[in] h Hauteur de l'écran
 * @note Fonction publique
 */
void DrawGameUI(GameContext* game, DungeonContext* dungeon, int w, int h);

/**
 * @brief Dessine une case d'équipement avec son contenu.
 * @details Affiche soit "[ Vide ]" soit l'ASCII art de l'objet équipé.
 * @param[in] game Contexte du jeu
 * @param[in] inv_idx Index de l'objet dans l'inventaire (-1 si vide)
 * @param[in] slot_label Nom du slot
 * @param[in] x Position X
 * @param[in] y Position Y
 * @param[in] width Largeur de la case
 * @param[in] height Hauteur de la case
 * @note Fonction publique
 */
void DrawEquipSlotGrid(GameContext* game, int inv_idx, const char* slot_label, int x, int y, int width, int height);

/**
 * @brief Dessine un bouton de boutique interactif.
 * @details Affiche le texte, gère le survol et détecte le clic utilisateur.
 * @param[in] font Police utilisée
 * @param[in] text Texte du bouton
 * @param[in] x Position X
 * @param[in] y Position Y
 * @param[in] fontSize Taille du texte
 * @param[in] canAfford Indique si l'achat est possible
 * @return true si le bouton est cliqué et achetable, false sinon
 * @note Fonction publique
 */
bool DoShopButton(Font font, const char* text, int x, int y, int fontSize, bool canAfford);

/**
 * @brief Retourne une chaîne représentant l'effet d'un objet.
 * @param[in] effect Identifiant de l'effet
 * @return Chaîne de caractères correspondant à l'effet
 * @note Fonction publique
 */
const char* GetEffectString(int effect);

#endif // UI_H