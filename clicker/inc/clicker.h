#ifndef CLICKER_H
#define CLICKER_H

#include "raylib.h"

/**
 * @brief Structure contenant toutes les ressources du joueur ainsi que les états de progression.
 * @details Gère les ressources, les déblocages et les auto-producteurs par tiers.
 * @note Structure publique
 */
typedef struct
{
    /** @brief Quantité d'herbes possédées */
    unsigned long long herbes;

    /** @brief Quantité de fer possédée */
    unsigned long long fer;

    /** @brief Quantité de viande possédée */
    unsigned long long viande;

    /** @brief Quantité d'or possédée */
    unsigned long long or ;

    /** @brief Quantité de bois possédée */
    unsigned long long bois;

    /** @brief Quantité de cristaux possédée */
    unsigned long long cristaux;

    /** @brief Indique si la ressource or est débloquée */
    bool unlock_or;

    /** @brief Indique si la ressource bois est débloquée */
    bool unlock_bois;

    /** @brief Indique si la ressource cristaux est débloquée */
    bool unlock_cristaux;

    /** @brief Indique si la ressource viande est débloquée */
    bool unlock_viande;

    /** @brief Auto-producteurs de fer (tiers 0 à 3) */
    int b_fer[4];

    /** @brief Auto-producteurs d'or */
    int b_or[4];

    /** @brief Auto-producteurs de cristaux */
    int b_cristaux[4];

    /** @brief Auto-producteurs d'herbes */
    int b_herbes[4];

    /** @brief Auto-producteurs de bois */
    int b_bois[4];

    /** @brief Auto-producteurs de viande */
    int b_viande[4];

} PlayerResources;

/**
 * @brief Contexte global du système de clicker.
 * @details Contient les ressources du joueur et les timers de production automatique.
 * @note Structure publique
 */
typedef struct
{
    /** @brief Inventaire du joueur */
    PlayerResources inventory;

    /** @brief Timer accumulant le temps pour la production automatique */
    float autoTimer;

    // Textures des ressources
    Texture2D tex_iron;
    Texture2D tex_gold;
    Texture2D tex_crystal;
    Texture2D tex_herbs;
    Texture2D tex_wood;
    Texture2D tex_meat;

} ClickerContext;

/**
 * @brief Initialise le contexte du clicker.
 * @param[out] clicker Contexte à initialiser
 * @note Fonction publique
 */
void Clicker_Init(ClickerContext* clicker);

/**
 * @brief Met à jour la production automatique des ressources.
 * @details Ajoute les ressources générées chaque seconde en fonction des bâtiments.
 * @param[in,out] clicker Contexte du clicker
 * @param[in] deltaTime Temps écoulé depuis la dernière frame
 * @note Fonction publique
 */
void Clicker_ProcessAuto(ClickerContext* clicker, float deltaTime, bool fire_lit);

/**
 * @brief Met à jour la logique de la mine (inputs, achats, clics).
 * @param[in,out] clicker Contexte du clicker
 * @param[in] viewStartX Position X de départ de la vue
 * @param[in] viewWidth Largeur de la vue
 * @param[in] screenHeight Hauteur de l'écran
 * @param[in] font Police utilisée pour le rendu
 * @note Fonction publique
 */
void Clicker_UpdateMine(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font);

/**
 * @brief Affiche l'interface de la mine.
 * @param[in] clicker Contexte du clicker
 * @param[in] viewStartX Position X de départ de la vue
 * @param[in] viewWidth Largeur de la vue
 * @param[in] screenHeight Hauteur de l'écran
 * @param[in] font Police utilisée pour le rendu
 * @note Fonction publique
 */
void Clicker_RenderMine(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font);

/**
 * @brief Met à jour la logique de la forêt (inputs, achats, clics).
 * @param[in,out] clicker Contexte du clicker
 * @param[in] viewStartX Position X de départ de la vue
 * @param[in] viewWidth Largeur de la vue
 * @param[in] screenHeight Hauteur de l'écran
 * @param[in] font Police utilisée pour le rendu
 * @note Fonction publique
 */
void Clicker_UpdateForest(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font);

/**
 * @brief Affiche l'interface de la forêt.
 * @param[in] clicker Contexte du clicker
 * @param[in] viewStartX Position X de départ de la vue
 * @param[in] viewWidth Largeur de la vue
 * @param[in] screenHeight Hauteur de l'écran
 * @param[in] font Police utilisée pour le rendu
 * @note Fonction publique
 */
void Clicker_RenderForest(ClickerContext* clicker, int viewStartX, int viewWidth, int screenHeight, Font font);


void Clicker_Unload(ClickerContext* clicker);
#endif // CLICKER_H