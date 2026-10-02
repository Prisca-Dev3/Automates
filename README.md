# TP INF3421 — Langage Formel & Compilation
### Boîte à outils interactive pour la théorie des langages et des automates finis

**Université de Yaoundé I — Faculté des Sciences — Département d'Informatique**
**Licence 3 Informatique — Année académique 2025-2026**
**Auteure : KAMDOM NGUETCHESSI Merveille Prisca**

---

## 1. Présentation

Ce projet implémente **en C, sans aucune bibliothèque externe** ("from scratch"),
l'intégralité des algorithmes vus dans le cours INF3421 : systèmes d'équations
linéaires en langages (lemme d'Arden), automates finis (déterministes,
non-déterministes, avec ε-transitions), constructions de Thompson et de
Glushkov, minimisation de Moore, opérations de clôture, etc.

Le programme se présente comme **un menu interactif unique**, dans lequel on
manipule un « automate courant » A (et éventuellement un second automate B
pour les opérations binaires) que l'on fait évoluer d'une opération à l'autre :
construire un automate, le déterminiser, le minimiser, en extraire
l'expression régulière, etc. Chaque affichage de la table de transition
s'accompagne de la **génération automatique d'une image PGM** représentant
graphiquement l'automate (états, transitions, flèche d'entrée, états finaux à
double cercle), sauvegardée dans le dossier `results/`.

## 2. Compilation et exécution

Un `Makefile` est fourni. Aucune dépendance autre que `gcc` et la bibliothèque
mathématique standard (`libm`) n'est nécessaire.

```bash
make          # compile le projet (crée aussi les dossiers results/*)
make run      # compile puis lance le programme interactif
make clean    # supprime les fichiers objets et l'executable
make distclean  # supprime en plus toutes les images generees
```

L'exécutable produit s'appelle `tp_inf3421`. Il peut aussi être lancé
directement après compilation :

```bash
./tp_inf3421
```

## 3. Organisation du code

```
include/            fichiers d'en-tete (.h) -- interfaces publiques
    pgm.h            moteur graphique (canvas, lignes, cercles, police bitmap)
    automaton.h       structure Automaton + tous les algorithmes sur automates
    regex.h           arbre syntaxique des expressions regulieres
    equation.h        systemes d'equations (Arden/Gauss), Thompson, Glushkov
src/                 implementations (.c)
    pgm.c             primitives de dessin + police 5x7 + sauvegarde P5
    automaton.c       automates : determinisation, minimisation, cloture...
    regex.c           analyseur syntaxique, simplification algebrique
    equation.c        resolution de systemes, Thompson, Glushkov
    main.c            menu interactif
results/             images PGM generees automatiquement, classees par theme
    01_equations/ ... 14_divers/
Makefile
README.md            (ce fichier)
rapport_INF3421.docx  rapport de TP (20 pages max)
```

## 4. Convention de saisie des expressions régulières

Pour rester lisible dans un terminal, les expressions régulières se saisissent
avec la syntaxe suivante :

| Symbole      | Signification                              |
|--------------|---------------------------------------------|
| `a`, `b`, `0`, `1`, ... | symbole de l'alphabet (lettre/chiffre)  |
| `+`          | union (au lieu de ∪)                         |
| juxtaposition | concaténation (implicite, pas de symbole)   |
| `*`          | étoile de Kleene                             |
| `@`          | mot vide ε (epsilon)                         |
| `#`          | langage vide ∅                               |
| `( )`        | parenthésage                                 |

Exemple : `(a+b)*abb` désigne le langage des mots sur `{a,b}` se terminant par
`abb`.

## 5. Fonctionnalités couvertes (menu principal)

1. **Définir / générer l'automate courant A** — saisie manuelle complète
   (nombre d'états, alphabet, états initiaux/finaux, transitions), ou choix
   parmi 5 automates d'exemple prêts à l'emploi illustrant chacun un aspect du
   cours (AFN non-déterministe, ε-AFN, AFD complet, automate avec états
   inutiles, AFD redondant pour la minimisation).
2. **Expression régulière courante** — saisie et analyse syntaxique.
3. **Systèmes d'équations (lemme d'Arden / méthode de Gauss)** — résolution
   d'un système saisi librement par l'utilisateur, ou extraction automatique
   de l'expression régulière associée à l'automate courant (le même moteur de
   résolution est réutilisé dans les deux cas — voir rapport §3.4).
4. **Transformations sur A** : déterminisation (construction des
   sous-ensembles), complétion (AFD → AFDC), suppression des
   ε-transitions (ε-AFN → AFN), conversions triviales AFN ↔ ε-AFN et
   AFD → AFN / ε-AFN, émondage, minimisation (algorithme de Moore),
   automate canonique, automate miroir.
5. **États** : ε-fermeture d'un état, états accessibles, co-accessibles,
   utiles.
6. **Propriétés et tests** : déterminisme, complétude, vacuité du langage,
   reconnaissance d'un mot, énumération des mots du langage.
7. **Opérations binaires** (nécessitent un second automate B) : union (par
   produit d'automates complets **et** par construction parallèle à la
   Thompson), intersection, différence, concaténation, test d'équivalence.
8. **Clôture unaire** : complémentation, étoile de Kleene.
9. **Expression régulière → automate** : construction de Thompson "pure",
   automate de Glushkov (automate des positions), simplification algébrique
   de l'expression.
10. **Simulation pas-à-pas** d'un mot sur l'automate courant (trace les
    ensembles d'états successivement atteints).
11. **Affichage** de la table de transition et génération de l'image PGM.

## 6. Validation

Chaque algorithme a été testé unitairement pendant le développement
(déterminisation, minimisation, Thompson, Glushkov, opérations de clôture),
puis validé par un test d'équivalence automatisé : pour huit expressions
régulières couvrant tous les opérateurs, on vérifie que
`regex → Thompson → automate_vers_regex → Thompson → automate` est
équivalent à l'automate de départ. Les huit cas passent (voir rapport,
section « Validation »).

## 7. Limites connues

- Le nombre d'états est plafonné à `MAX_STATES = 60` (voir `automaton.h`) afin
  de garder une structure de données statique simple et rapide ; largement
  suffisant pour tous les exercices du cours.
- Les opérations binaires (union, intersection...) déterminisent et complètent
  systématiquement leurs deux opérandes avant de construire le produit,
  conformément aux théorèmes 5 et 6 du cours.
- Le moteur graphique PGM est volontairement minimaliste (police bitmap 5×7,
  disposition circulaire des états) : l'objectif est la clarté pédagogique,
  pas le rendu esthétique d'un outil comme Graphviz.
