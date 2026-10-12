# TrueHuffer
TrueHuffer, programme de compression de fichiers basé sur la méthode Huffman.

## Roadmap

### 1 : Creer les fonctions principales

Deflate (compression) et inflate (décompressions) dans src/compression.c

### 2 : Gestion d'erreurs

Gestion en cas de plantage, ou de overflow memoire 

### 3 L'ouverture et utilisation via Shell

Utilisation classique (execution dans console avec argument)

### 4 : Interface utilisateur

Création d'une interface type 7-zip / WinRAR (Design : Yanis, Code : Daniel)

### 5 : Tests

Compressions puis décompressions en chaine suivis de chekcsum (verification de l'identicité des fichiers)