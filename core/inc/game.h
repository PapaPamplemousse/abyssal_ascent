#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "raylib.h"
#include "clicker.h"
#include "combat.h"

/**
 * @enum GameState
 * @brief Représente les différents états possibles du jeu.
 */
typedef enum
{
    STATE_MENU,        /**< Menu principal */
    STATE_CAMP,        /**< Camp (hub principal) */
    STATE_MINE,        /**< Mine (clicker fer) */
    STATE_FOREST,      /**< Forêt (clicker bois) */
    STATE_FORGE,       /**< Forge (amélioration équipement) */
    STATE_ALCHEMIST,   /**< Alchimiste (potions) */
    STATE_ARCHIFORGE,  /**< Archiforge (sorts) */
    STATE_DUNGEON,     /**< Exploration du donjon */
    STATE_INVENTORY,   /**< Inventaire */
    STATE_ALTAR,       /**< Altar */
    STATE_GAMEOVER     /**< Écran de mort */
} GameState;

/**
 * @struct GameContext
 * @brief Contexte principal du jeu contenant tous les sous-systèmes.
 */
typedef struct
{
    GameState      currentState; /**< État courant du jeu */
    bool           isRunning;    /**< Indique si le jeu est en cours d'exécution */
    Font           uiFont;       /**< Police pour l'interface utilisateur */
    Font           dungeonFont;  /**< Police pour les ASCII du donjon */
    Texture2D tex_pentagram;     /**< pentacle pour l'autel  */
    // Images du Feu de Camp 
    Texture2D tex_fire_lit[4]; // Tableau de 4 frames pour l'animation
    Texture2D tex_fire_unlit;  // Le feu éteint
    
    ClickerContext clicker;      /**< Contexte du système clicker */
    CombatContext  combat;       /**< Contexte du système de combat */
} GameContext;

/**
 * @brief Initialise le jeu et tous ses sous-systèmes.
 * 
 * @param game Pointeur vers le contexte du jeu.
 */
void Game_Init(GameContext* game);

/**
 * @brief Lance la boucle principale du jeu.
 * 
 * @param game Pointeur vers le contexte du jeu.
 */
void Game_Run(GameContext* game);

/**
 * @brief Met à jour la logique du jeu (input, états, systèmes).
 * 
 * @param game Pointeur vers le contexte du jeu.
 */
void Game_Update(GameContext* game);


/**
 * @brief Rend le jeu à l'écran.
 * 
 * @param game Pointeur vers le contexte du jeu.
 */
void Game_Render(GameContext* game);

/**
 * @brief Libère les ressources du jeu.
 * 
 * @param game Pointeur vers le contexte du jeu.
 */
void Game_Close(GameContext* game);


/**
 * @brief Nettoie l'état complet du jeu pour recommencer à zéro
 * 
 * @param game Pointeur vers le contexte du jeu.
 */
void Game_ResetState(GameContext* game);


#endif // GAME_H