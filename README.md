# TrueHuffer
TrueHuffer, programme de compression de fichiers basé sur la méthode Huffman.

[![Dan149 Commits](https://img.shields.io/badge/Dan149_Commits-Ici-blue?logo=github)](https://github.com/Dan149/TrueHuffer/commits?author=Dan149)
[![JustYanyan Commits](https://img.shields.io/badge/JustYanyan_Commits-Ici-green?logo=github)](https://github.com/Dan149/TrueHuffer/commits?author=JustYanyan)

## Remarques pour Mme Sabahi
Nous avons décidé de commenter le code en anglais durant le développement. Si cela change pour la version finale, ne soyez pas étonnée de trouver des explications en anglais dans les commits précédents de ma part pour Yanis et inversement.
De la même manière, le nom des variables & fonctions est en anglais.

## Ce que l'IA a fait:
v (Note de Daniel) v
J'ai pris la liberté d'utiliser l'IA pour les usages suivants:
- Conseils sur la façon de stocker les données d'un fichier.
- Création du Makefile.
- Création de tests robustes et déterministes.
- Conception de l'UI dans le terminal.

Ces modifications ont étés apportés lors de mon dernier commit, qui comprend aussi la majorité de mon travail sur l'optimisation de la génération de l'arbre binaire et son stockage efficace dans un fichier.

___

## Documentation du Makefile

Ce Makefile permet de compiler l'application principale (avec ou sans interface graphique) ainsi que sa suite de tests. Il est configuré pour utiliser le compilateur `gcc` avec des drapeaux de compilation stricts (`-Wall -Wextra -Werror -pedantic`) pour garantir la qualité du code.

### 🚀 Commandes disponibles (Cibles)

Voici les différentes commandes que vous pouvez exécuter dans votre terminal :

*   **`make`** ou **`make all`** (Cible par défaut)
    Compile l'application standard complète (nommée `app`) en incluant l'interface graphique (GUI).
    
*   **`make no-gui`**
    Compile une version de l'application sans l'interface graphique. 
    *Note technique : Cette cible ajoute la macro `-DNO_GUI` à la compilation, nettoie automatiquement les anciens fichiers objets pour éviter les conflits, et exclut les fichiers sources liés au GUI.*

*   **`make tests`**
    Compile et exécute automatiquement la suite de tests (génère l'exécutable `test_runner`).
    *Note technique : Lors de la compilation des tests, la fonction `tests()` est redéfinie en `main()` via le préprocesseur (`-Dtests=main`).*

*   **`make clean`**
    Nettoie complètement le projet en supprimant tous les fichiers objets (`.o`) ainsi que les exécutables générés (`app` et `test_runner`).

*   **`make clean-obj`**
    Supprime uniquement les fichiers objets (`.o`), en conservant les exécutables finaux.

### 📁 Structure des sources attendue

Le Makefile s'attend à l'arborescence suivante pour fonctionner correctement :

*   `main.c` : Point d'entrée de l'application.
*   `src/` : Dossier contenant le code source.
    *   `src/compress.c`, `src/debug.c` : Fichiers cœurs (utilisés par l'app et les tests).
    *   `src/gui.c` : Fichiers de l'interface graphique.
*   `tests/` : Dossier contenant les tests.
    *   `tests/tests.c` : Point d'entrée de la suite de tests.

### ⚙️ Configuration

*   **Compilateur :** `gcc`
*   **Dossier d'inclusion (`Include`) :** `-Isrc`
*   **Exécutable principal :** `app`
*   **Exécutable de test :** `test_runner`
