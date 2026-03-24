#ifndef DATABASE_H
#define DATABASE_H

/**
 * @brief Initialise toutes les bases de données du jeu.
 * @details Charge les fichiers JSON pour les monstres, objets, sorts, potions,
 *          ambiance du donjon et salles, ainsi que la langue.
 * @param[in] monsters_path Chemin vers le fichier JSON des monstres
 * @param[in] items_path Chemin vers le fichier JSON des objets
 * @param[in] spells_path Chemin vers le fichier JSON des sorts
 * @param[in] potions_path Chemin vers le fichier JSON des potions
 * @param[in] ambiance_path Chemin vers le fichier JSON des ambiances
 * @param[in] rooms_path Chemin vers le fichier JSON des salles
 * @param[in] lang_path Chemin vers le fichier de langue
 * @note Fonction publique
 */
void DB_Init(const char* monsters_path, const char* items_path, const char* spells_path, const char* potions_path, const char* ambiance_path, const char* rooms_path, const char* lang_path);

#endif // DATABASE_H