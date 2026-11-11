# Rapport de projet — TrueHuffer

**Compression et décompression de fichiers par codage de Huffman, en langage C**

**Auteurs :** Yanis et Daniel
---

## Sommaire

1. [Introduction](#1-introduction)
2. [Équipe et répartition du travail](#2-équipe-et-répartition-du-travail)
3. [Cahier des charges](#3-cahier-des-charges)
4. [L'algorithme de Huffman](#4-lalgorithme-de-huffman)
5. [Architecture du projet](#5-architecture-du-projet)
6. [Format de fichier `.thman`](#6-format-de-fichier-thman)
7. [Fonctionnalités optionnelles](#7-fonctionnalités-optionnelles)
8. [Compilation et utilisation](#8-compilation-et-utilisation)
9. [Tests et résultats](#9-tests-et-résultats)
10. [Utilisation de l'IA et sources externes](#10-utilisation-de-lia-et-sources-externes)
11. [Difficultés, limites et pistes d'amélioration](#11-difficultés-limites-et-pistes-damélioration)
12. [Conclusion](#12-conclusion)

\---

## 1\. Introduction

TrueHuffer est un programme en C qui compresse (*deflate*) et décompresse (*inflate*) des données grâce au **codage de Huffman**, un algorithme de compression sans perte. Le principe : les octets fréquents reçoivent des codes binaires courts, les octets rares des codes plus longs, ce qui réduit la taille totale lorsque la distribution des octets est inégale (typiquement du texte).

Les fichiers compressés portent l'extension **`.thman`**.

Ce rapport présente l'organisation de l'équipe, le fonctionnement de l'algorithme tel qu'il est implémenté, l'architecture du code, les fonctionnalités réalisées au-delà du besoin initial, ainsi que les tests effectués et les limites actuelles.

## 2\. Équipe et répartition du travail

|Contributeur|Profil|Rôle dans le projet|
|-|-|-|
|**Yanis**|Débutant en C|S'est concentré sur la **compréhension de l'algorithme de Huffman** et a travaillé sur l'**inflation et la déflation** du contenu (table de fréquences, tri, construction de l'arbre, compression et décompression).|
|**Daniel**|Expérimenté en C|A réalisé **tout le reste et a adapté l'usage de l'algorithme aux fichiers** : lecture/écriture des fichiers, structure du projet, optimisation pour de longues entrées etc.|

Cette répartition a permis à Yanis de se concentrer sur le cœur algorithmique, qui était le sujet pédagogique principal, pendant que Daniel s'occupait de l'infrastructure autour.

## 3\. Cahier des charges

### 3.1 Exigence initiale (obligatoire)

L'exigence de départ était **de dégonfler et de gonfler une entrée texte**.

### 3.2 Fonctionnalités optionnelles

La plupart des fonctionnalités du programme vont au-delà de cette exigence et sont donc **optionnelles** :

|Fonctionnalité|Statut|
|-|-|
|Dégonfler un texte|Obligatoire|
|Gonfler un texte|Obligatoire|
|Lecture et écriture de fichiers `.thman`|Optionnel|
|Prise en charge de fichiers binaires quelconques (pas seulement du texte)|Optionnel|
|Utilisation en ligne de commande avec arguments|Optionnel|
|Interface graphique GTK3|Optionnel|
|Détection des fichiers corrompus avec messages d'erreur|Optionnel|
|Suite de tests automatisés|Optionnel (?)|
|Fonctions de débogage (affichage de la table et de l'arbre)|Optionnel (?)|

Le projet peut d'ailleurs être compilé **sans interface graphique** (`make no-gui`), ce qui reflète le caractère optionnel de cette partie.

## 4\. L'algorithme de Huffman

### 4.1 Étapes

1. **Table de fréquences** : on compte combien de fois chaque octet (0 à 255) apparaît dans les données.
2. **Tri** : la table est triée par fréquence croissante avec un **tri fusion** (*merge sort*), stable.
3. **Construction de l'arbre** (algorithme *bottom-up*) : tant qu'il reste plus d'un nœud, on retire les deux nœuds de plus faible fréquence et on crée un nœud parent dont la fréquence est la somme des deux ; l'enfant `child\_a` correspond au bit `0`, `child\_b` au bit `1`.
4. **Codes** : le code d'un octet est le chemin de la racine jusqu'à sa feuille.
5. **Écriture** : on écrit un en-tête contenant la table de fréquences, puis les codes de tous les octets, bit par bit.

À la décompression, on relit la table, on **reconstruit exactement le même arbre** (la construction est déterministe), puis on parcourt l'arbre bit par bit pour retrouver chaque octet.

### 4.2 Exemple : `abracadabra`

|Octet|Fréquence|Code|
|-|-|-|
|`a`|5|`0`|
|`b`|2|`10`|
|`r`|2|`111`|
|`c`|1|`1101`|
|`d`|1|`1100`|

Le texte (11 octets) est codé sur 5·1 + 2·2 + 2·3 + 4 + 4 = **23 bits**, soit 3 octets. Avec l'en-tête (30 octets pour 5 symboles distincts), le fichier fait 33 octets : sur de très petits textes, l'en-tête coûte donc plus que ce que la compression économise.

Arbre obtenu (affichage du module de débogage, de côté) :

```
   -      -      -   \['r':2]
   -      -   \[00:4]
   -      -      -      -   \['c':1]
   -      -      -   \[00:2]
   -      -      -      -   \['d':1]
   -   \[00:6]
   -      -   \['b':2]
\[00:11]
   -   \['a':5]
```

## 5\. Architecture du projet

```
.
├── Makefile
├── main.c              Point d'entrée et programme en ligne de commande
├── src/
│   ├── compress.c/.h   Cœur : table de fréquences, arbre, deflate, inflate, fichiers
│   ├── debug.c/.h      Affichage de la table de fréquences et de l'arbre
│   └── gui.c/.h        Interface graphique GTK3 (optionnelle)
└── tests/
    ├── tests.c         Suite de tests
    └── lorem.txt       Fichier d'exemple pour les tests
```

### 5.1 Structures de données (`compress.h`)

|Structure|Rôle|
|-|-|
|`TextObject`|Contient le contenu inflaté et/ou déflaté, avec leurs tailles. Les tampons sont toujours terminés par `'\\0'`, mais ce sont de véritables tableaux d'octets (les octets nuls sont gérés).|
|`FrequencyTable`|Tableau d'octets distincts (`keys`), leurs fréquences (`freq`) et leur nombre (`size`).|
|`Node`|Nœud de l'arbre : octet, fréquence (ou somme des enfants), `child\_a` et `child\_b`. Une feuille n'a pas d'enfants.|
|`BinaryTree`|Racine de l'arbre de Huffman.|

### 5.2 Fonctions principales

|Fonction|Rôle|
|-|-|
|`get\_freq\_table()`|Compte les octets et renvoie la table triée.|
|`freq\_t\_merge\_sort()`|Tri fusion de la table par fréquence croissante.|
|`build\_btree()`|Construit l'arbre de Huffman.|
|`deflate()`|Compresse le contenu d'un `TextObject`.|
|`inflate()`|Décompresse le contenu d'un `TextObject`, avec vérification de la validité des données.|
|`init\_text\_obj()`|Lit un fichier ; il est considéré comme déflaté s'il commence par l'en-tête TrueHuffer.|
|`writeTextObjToFile()`|Écrit le contenu courant dans un fichier.|
|`make\_output\_path()`|Propose un nom de sortie (`a.txt` ↔ `a.txt.thman`).|
|`compress\_last\_error()`|Message décrivant la dernière erreur, utilisé par la ligne de commande et l'interface graphique.|

## 6\. Format de fichier `.thman`

|Position|Taille|Contenu|
|-|-|-|
|0 à 2|3 octets|Signature : `1`, `127`, `2`|
|3 à 4|2 octets|Nombre *N* d'octets distincts (little endian, de 0 à 256)|
|ensuite|*N* × 5 octets|Pour chaque octet : sa valeur (1 octet) et sa fréquence (4 octets, little endian)|
|fin|variable|Flux de bits des codes de Huffman, bit de poids fort en premier, dernier octet complété par des zéros|

Le décodeur s'arrête après avoir produit autant d'octets que la somme des fréquences ; le bourrage final est donc ignoré.

**Cas particuliers gérés :**

* **Fichier vide** : en-tête de 5 octets sans données.
* **Un seul octet distinct** (arbre réduit à une feuille) : 1 bit par symbole.
* **Données corrompues** (signature absente, en-tête ou données tronqués, table invalide) : `inflate()` renvoie une erreur sans planter ni allouer une mémoire démesurée.

## 7\. Fonctionnalités optionnelles

### 7.1 Lecture et écriture de fichiers

Le programme lit n'importe quel fichier (texte ou binaire) et écrit les résultats avec `writeTextObjToFile()`. Les écritures vérifient les erreurs de `fwrite` et de `fclose`.

### 7.2 Utilisation en ligne de commande

```
./app deflate <fichier> \[-o <sortie>] \[-f]
./app inflate <fichier> \[-o <sortie>] \[-f]
./app gui
./app --help
```

* Par défaut, `deflate` crée `<fichier>.thman` et `inflate` retire cette extension.
* Le programme **refuse d'écraser** un fichier existant, sauf avec l'option `-f` (`--force`).
* Il affiche les tailles avant et après (et le pourcentage pour la compression).
* Codes de retour : `0` en cas de succès, `1` en cas d'erreur, `2` en cas d'erreur d'utilisation.
* Sans argument, la version avec interface graphique ouvre la fenêtre ; la version `no-gui` affiche l'aide.

### 7.3 Interface graphique (GTK3)

La fenêtre, définie dans `gui.c`, comporte un champ de saisie du chemin, un bouton **Parcourir…**, les boutons **Déflater** et **Inflater**, et une ligne d'état. Après l'opération, une boîte de dialogue « Enregistrer » propose un nom de sortie et demande confirmation avant d'écraser un fichier. Les erreurs s'affichent dans une boîte de dialogue.

Cette partie n'est compilée que si nécessaire : le `Makefile` ne fait appel à GTK que pour `gui.c`, et la cible `no-gui` permet de compiler sans GTK installé.

### 7.4 Fonctions de débogage

`debug.c` fournit `print\_freq\_table()`, `print\_node()` et `print\_btree()`, qui affichent la table de fréquences et l'arbre de côté. Elles ont servi à vérifier la construction de l'arbre (voir l'exemple de la section 4.2).

## 8\. Compilation et utilisation

**Prérequis :** `gcc` et `make`. Pour l'interface graphique : `libgtk-3-dev` et `pkg-config`.

|Commande|Effet|
|-|-|
|`make`|Compile le programme `app` avec l'interface graphique|
|`make no-gui`|Compile `app` en ligne de commande uniquement (sans GTK)|
|`make tests`|Compile et exécute la suite de tests|
|`make clean`|Supprime les fichiers générés|

Options de compilation : `-Wall -Wextra -Werror -pedantic`, c'est-à-dire que tout avertissement fait échouer la compilation.

Exemple d'utilisation :

```
$ ./app deflate tests/lorem.txt
tests/lorem.txt -> tests/lorem.txt.thman
1017 bytes -> 693 bytes (68.1% of the original size)

$ ./app inflate tests/lorem.txt.thman -o copie.txt
tests/lorem.txt.thman -> copie.txt
693 bytes -> 1017 bytes
```

## 9\. Tests et résultats

### 9.1 Suite de tests (`make tests`)

|Test|Vérification|
|-|-|
|Entrée vide|Aller-retour correct|
|Un seul symbole répété|Cas de l'arbre à une feuille|
|Un seul octet, `abracadabra`|Petites entrées|
|Les 256 valeurs d'octets (dont l'octet nul)|Gestion des données binaires|
|Données binaires pseudo-aléatoires biaisées (200 ko)|Aller-retour correct et fichier effectivement plus petit|
|Fréquences de Fibonacci|Arbre très profond|
|Fichier `lorem.txt`|Cycle complet fichier → déflation → écriture → relecture → inflation, contenu identique|
|Fichiers corrompus ou tronqués|Erreurs détectées, aucun plantage|
|Noms de fichiers de sortie|`a.txt` → `a.txt.thman`, et inversement|

Tous les tests passent. Le programme et les tests ont aussi été exécutés avec **AddressSanitizer et UBSan** sans erreur mémoire détectée.

### 9.2 Taux de compression observés

|Fichier|Taille d'origine|Taille compressée|Rapport|
|-|-|-|-|
|`lorem.txt` (texte)|1017 octets|693 octets|68,1 %|
|Texte de 5 lettres et espaces (aléatoire)|8243 octets|2980 octets|36,2 %|
|Données aléatoires uniformes|5000 octets|6278 octets|125,6 %|

Comme attendu avec Huffman, les données dont la distribution est très inégale se compressent bien, tandis que des données aléatoires uniformes **grossissent** légèrement à cause de l'en-tête (jusqu'à 5 octets par symbole distinct).

## 10\. Utilisation de l'IA et sources externes

### 10.1 Intelligence artificielle

Une IA a été utilisée pour générer :

* le **`Makefile`** ;
* l'**interface utilisateur en terminal** (programme en ligne de commande) ;
* la **suite de tests**.
* Le formatage partiel de ce rapport en fichier Markdown, avec une présentation structuré dans des tableaux et des sections ordonnées à partir des informations importantes.

### 10.2 Sources externes

L'**interface graphique** a été réalisée à partir d'un **exemple trouvé sur le web** (GTK3), puis adaptée au projet.


## 11\. Difficultés, limites et pistes d'amélioration

### 11.1 Difficultés rencontrées

* **Déterminisme** : le compresseur et le décompresseur doivent construire exactement le même arbre, y compris en cas d'égalité de fréquences ; la construction et les tris sont donc stables et sans dépendance cachée.
* **Données binaires** : la première approche traitait les données comme des chaînes C, ce qui posait problème pour l'octet nul ; les tailles sont désormais gérées explicitement.
* **Arbre à une seule feuille** : un fichier ne contenant qu'un seul octet distinct produirait un code de longueur 0, il faut donc lui attribuer 1 bit.
* **Gestion des erreurs** : vérification de chaque allocation et de chaque lecture/écriture, libération de la mémoire dans tous les cas d'échec.

### 11.2 Limites actuelles

* Taille d'entrée limitée à **4 Gio** (les fréquences sont stockées sur 4 octets).
* Le fichier est **entièrement chargé en mémoire**.
* Un seul fichier par appel en ligne de commande.
* L'en-tête (jusqu'à 1285 octets) rend la compression inefficace sur les très petits fichiers.



### 11.3 Pistes d'amélioration

* MAJEUR: Adapter la taille allouée aux fréquences dynamiquement pour permettre la compression de fichier plus gros.
* MAJEUR:  réduire l'en-tête en stockant les codes Huffman de manière canonique.
* MAJEUR: Traiter les fichiers **par blocs** pour ne pas tout charger en mémoire.
* Mineur: Prendre en charge plusieurs fichiers ou des dossiers d'un seul coup.
* Mineur: Ajouter une barre de progression et le glisser-déposer dans l'interface graphique.
* MAJEUR: Ajouter une somme de contrôle pour détecter les corruptions de données.

## 12\. Conclusion

Le projet remplit l'exigence initiale — déflater et inflater un texte avec l'algorithme de Huffman — et l'étend avec la gestion de fichiers, un programme en ligne de commande, une interface graphique optionnelle et une suite de tests. La répartition des rôles a permis à Yanis, débutant en C, de comprendre en profondeur l'algorithme de Huffman et de le programmer, tandis que Daniel assurait l'infrastructure du projet et la validité du code. Le résultat est fiable sur tous les cas testés (fichiers vides, binaires, corrompus) et la compression est efficace sur du texte.

