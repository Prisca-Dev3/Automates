/* ============================================================================
 * main.c -- Menu interactif INF3421 : Langage Formel & Compilation
 * Universite de Yaounde I - Faculte des Sciences - Departement d'Informatique
 * Licence 3 Informatique - Annee academique 2025-2026
 *
 * Regroupe, sous forme d'un programme unique, tous les algorithmes du cours :
 * systemes d'equations (Arden/Gauss), automates finis (AFN/AFD/eps-AFN),
 * constructions Thompson et Glushkov, minimisation, operations de cloture...
 * ==========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "automaton.h"
#include "regex.h"
#include "equation.h"

/* ---------------------------- utilitaires de saisie --------------------- */
static void read_line(char *buf, int size) {
    if (fgets(buf, size, stdin) == NULL) { buf[0] = '\0'; return; }
    size_t l = strlen(buf);
    if (l > 0 && buf[l - 1] == '\n') buf[l - 1] = '\0';
}
static int read_int(const char *prompt) {
    char buf[64];
    int v;
    while (1) {
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        if (sscanf(buf, "%d", &v) == 1) return v;
        printf("  (!) veuillez entrer un nombre entier valide.\n");
    }
}
static void read_str(const char *prompt, char *out, int size) {
    printf("%s", prompt);
    read_line(out, size);
}
static void pause_enter(void) {
    printf("\n-- Appuyez sur ENTRER pour continuer --");
    char buf[8]; read_line(buf, sizeof(buf));
}

/* ---------------------------- espace de travail -------------------------- */
static Automaton A, B;
static int hasA = 0, hasB = 0;
static RNode *curRegex = NULL;
static char curRegexStr[512] = "";

static int ctr_afn_afd = 0, ctr_afdc = 0, ctr_etats = 0, ctr_emondage = 0,
           ctr_conv = 0, ctr_thompson = 0, ctr_minim = 0, ctr_glushkov = 0,
           ctr_canon = 0, ctr_cloture = 0, ctr_mots = 0, ctr_equations = 0,
           ctr_divers = 0;

static void show_automaton(const Automaton *a, const char *label, const char *subdir, int *ctr) {
    aut_print(a, label);
    char path[256];
    (*ctr)++;
    snprintf(path, sizeof(path), "results/%s/%03d_%s.pgm", subdir, *ctr, "automate");
    aut_render_pgm(a, path, label);
}

static void ensure_A(void) {
    if (!hasA) printf("\n(!) Aucun automate courant defini. Utilisez d'abord l'option [1] du menu principal.\n");
}
static void ensure_regex(void) {
    if (!curRegex) printf("\n(!) Aucune expression reguliere courante. Utilisez d'abord l'option [2] du menu principal.\n");
}

/* ============================== construction manuelle ==================== */
static void build_manual(Automaton *a) {
    char alpha[MAX_SYMB + 1], buf[256];
    int n = read_int("Nombre d'etats (0..n-1) : ");
    if (n < 1) n = 1;
    read_str("Alphabet (ex: ab, ou 01) : ", alpha, sizeof(alpha));
    int hasEps = read_int("Autoriser les epsilon-transitions ? (1=oui / 0=non) : ");
    aut_init(a, n, alpha, hasEps);

    read_str("Etats initiaux, separes par des espaces (ex: 0 2) : ", buf, sizeof(buf));
    { char *tok = strtok(buf, " "); while (tok) { aut_set_initial(a, atoi(tok), 1); tok = strtok(NULL, " "); } }

    read_str("Etats finaux, separes par des espaces (ex: 3 4) : ", buf, sizeof(buf));
    { char *tok = strtok(buf, " "); while (tok) { aut_set_final(a, atoi(tok), 1); tok = strtok(NULL, " "); } }

    printf("\nSaisie des transitions. Format : etat_origine symbole etat_destination\n");
    printf("(utilisez '@' pour une epsilon-transition si autorisee). Tapez 'fin' pour arreter.\n");
    while (1) {
        read_str("transition> ", buf, sizeof(buf));
        if (strcmp(buf, "fin") == 0 || strlen(buf) == 0) break;
        int from, to; char sym;
        if (sscanf(buf, "%d %c %d", &from, &sym, &to) == 3) aut_add_trans(a, from, sym, to);
        else printf("  (!) format invalide, reessayez (ex: 0 a 1)\n");
    }
}

/* ============================== exemples pre-construits =================== */
static Automaton generate_example(int which) {
    Automaton a;
    switch (which) {
    case 1: /* AFN non-deterministe : mots se terminant par 'ab' */
        aut_init(&a, 3, "ab", 0);
        aut_set_initial(&a, 0, 1); aut_set_final(&a, 2, 1);
        aut_add_trans(&a, 0, 'a', 0); aut_add_trans(&a, 0, 'b', 0);
        aut_add_trans(&a, 0, 'a', 1); aut_add_trans(&a, 1, 'b', 2);
        printf("Exemple 1 charge : AFN non-deterministe reconnaissant {a,b}*.ab\n");
        break;
    case 2: /* eps-AFN pour a*b*c* (cf. figure 3.11 du cours) */
        aut_init(&a, 3, "abc", 1);
        aut_set_initial(&a, 0, 1); aut_set_final(&a, 2, 1);
        aut_add_trans(&a, 0, 'a', 0); aut_add_trans(&a, 1, 'b', 1); aut_add_trans(&a, 2, 'c', 2);
        aut_add_trans(&a, 0, '@', 1); aut_add_trans(&a, 1, '@', 2);
        printf("Exemple 2 charge : epsilon-AFN pour a*b*c* (figure 3.11 du cours)\n");
        break;
    case 3: /* AFD comptant les a modulo 3 (figure 3.3 du cours) */
        aut_init(&a, 3, "ab", 0);
        aut_set_initial(&a, 0, 1); aut_set_final(&a, 0, 1);
        aut_add_trans(&a, 0, 'a', 1); aut_add_trans(&a, 1, 'a', 2); aut_add_trans(&a, 2, 'a', 0);
        aut_add_trans(&a, 0, 'b', 0); aut_add_trans(&a, 1, 'b', 1); aut_add_trans(&a, 2, 'b', 2);
        printf("Exemple 3 charge : AFD comptant les 'a' modulo 3 (figure 3.3 du cours)\n");
        break;
    case 4: /* automate avec etats inutiles */
        aut_init(&a, 5, "ab", 0);
        aut_set_initial(&a, 0, 1); aut_set_final(&a, 2, 1);
        aut_add_trans(&a, 0, 'a', 1); aut_add_trans(&a, 1, 'b', 2);
        aut_add_trans(&a, 2, 'a', 3); /* 3 accessible mais non co-accessible (pas de chemin vers un final) */
        aut_add_trans(&a, 3, 'a', 4); /* 4 idem */
        printf("Exemple 4 charge : automate avec des etats non utiles (illustration emondage)\n");
        break;
    case 5: /* AFD redondant pour la minimisation : mots se terminant par 'a' */
        aut_init(&a, 4, "ab", 0);
        aut_set_initial(&a, 0, 1); aut_set_final(&a, 2, 1); aut_set_final(&a, 3, 1);
        aut_add_trans(&a, 0, 'a', 1); aut_add_trans(&a, 0, 'b', 0);
        aut_add_trans(&a, 1, 'a', 2); aut_add_trans(&a, 1, 'b', 0);
        aut_add_trans(&a, 2, 'a', 3); aut_add_trans(&a, 2, 'b', 0);
        aut_add_trans(&a, 3, 'a', 3); aut_add_trans(&a, 3, 'b', 0);
        printf("Exemple 5 charge : AFD redondant (les etats 2 et 3 sont equivalents) pour illustrer Moore\n");
        break;
    default:
        aut_init(&a, 1, "a", 0);
        aut_set_initial(&a, 0, 1);
        break;
    }
    return a;
}

/* ============================== menu : automate courant =================== */
static void menu_define_automaton(void) {
    printf("\n--- Definir l'automate courant (A) ---\n");
    printf(" 1. Saisie manuelle complete\n");
    printf(" 2. Exemple 1 : AFN 'se termine par ab' (non-deterministe)\n");
    printf(" 3. Exemple 2 : epsilon-AFN pour a*b*c*\n");
    printf(" 4. Exemple 3 : AFD comptant les a modulo 3 (complet)\n");
    printf(" 5. Exemple 4 : automate avec etats inutiles\n");
    printf(" 6. Exemple 5 : AFD redondant (demo minimisation)\n");
    printf(" 0. Annuler\n");
    int c = read_int("Choix : ");
    if (c == 1) { build_manual(&A); hasA = 1; }
    else if (c >= 2 && c <= 6) { A = generate_example(c - 1); hasA = 1; }
    else return;
    show_automaton(&A, "Automate courant A", "14_divers", &ctr_divers);
}
static void menu_define_second(void) {
    printf("\n--- Definir le second automate (B) ---\n");
    printf(" 1. Saisie manuelle complete\n");
    printf(" 2..6. Exemples (identiques au menu precedent)\n");
    printf(" 0. Annuler\n");
    int c = read_int("Choix : ");
    if (c == 1) { build_manual(&B); hasB = 1; }
    else if (c >= 2 && c <= 6) { B = generate_example(c - 1); hasB = 1; }
    else return;
    show_automaton(&B, "Automate secondaire B", "14_divers", &ctr_divers);
}
static void menu_define_regex(void) {
    char buf[256];
    printf("\nConvention : symboles=lettres/chiffres, union='+', etoile='*', epsilon='@', vide='#', concatenation implicite.\n");
    read_str("Entrez l'expression reguliere : ", buf, sizeof(buf));
    if (curRegex) regex_free(curRegex);
    curRegex = regex_parse(buf);
    curRegex = regex_simplify(curRegex);
    regex_to_string_buf(curRegex, curRegexStr, sizeof(curRegexStr));
    printf("Expression enregistree : %s\n", curRegexStr);
}

/* ============================== sous-menus =================== */
static void menu_equations(void) {
    printf("\n--- Systemes d'equations lineaires en langages (lemme d'Arden) ---\n");
    printf(" 1. Saisir et resoudre un systeme libre\n");
    printf(" 2. Extraire l'expression reguliere de l'automate courant A\n");
    printf(" 0. Retour\n");
    int c = read_int("Choix : ");
    if (c == 1) {
        int n = read_int("Nombre d'inconnues (X0..Xn-1) : ");
        if (n < 1 || n > MAX_EQ) { printf("valeur invalide\n"); return; }
        EqSystem s; eqsys_init(&s, n);
        printf("Pour chaque equation Xi = A0.X0 + A1.X1 + ... + B, entrez les coefficients.\n");
        printf("Coefficient vide autorise (appuyer sur ENTRER = absent / langage vide).\n");
        for (int i = 0; i < n; i++) {
            printf("\n-- Equation de X%d --\n", i);
            for (int j = 0; j < n; j++) {
                char buf[128];
                char prompt[64]; snprintf(prompt, sizeof(prompt), "  coefficient de X%d (regex ou vide) : ", j);
                read_str(prompt, buf, sizeof(buf));
                if (strlen(buf) > 0) s.A[i][j] = regex_simplify(regex_parse(buf));
            }
            char buf[128];
            read_str("  terme constant B (regex ou vide) : ", buf, sizeof(buf));
            if (strlen(buf) > 0) s.B[i] = regex_simplify(regex_parse(buf));
        }
        eqsys_print(&s, "Systeme saisi");
        eqsys_solve(&s, 1);
        ctr_equations++;
        printf("\n(Resultats affiches ci-dessus. Une trace complete a ete imprimee, methode de Gauss.)\n");
    } else if (c == 2) {
        ensure_A(); if (!hasA) return;
        char *rex = aut_to_regex(&A);
        printf("\n===> Expression reguliere de L(A) : %s\n", rex);
        free(rex);
        ctr_equations++;
    }
}

static void menu_transform(void) {
    printf("\n--- Transformations sur l'automate courant A ---\n");
    printf("  1. AFN/eps-AFN -> AFD  (determinisation par sous-ensembles)\n");
    printf("  2. AFD -> AFDC (completion, ajout d'un etat puits)\n");
    printf("  3. eps-AFN -> AFN (suppression des epsilon-transitions)\n");
    printf("  4. AFN -> eps-AFN (conversion triviale)\n");
    printf("  5. AFD -> AFN (conversion triviale)\n");
    printf("  6. AFD -> eps-AFN (conversion triviale)\n");
    printf("  7. Emondage (etats utiles uniquement)\n");
    printf("  8. Minimisation (algorithme de Moore)\n");
    printf("  9. Automate canonique A_L (emondage + minimisation)\n");
    printf(" 10. Miroir (automate du langage renverse)\n");
    printf("  0. Retour\n");
    int c = read_int("Choix : ");
    ensure_A(); if (!hasA) return;
    Automaton r;
    switch (c) {
        case 1: r = aut_determinize(&A); show_automaton(&r, "AFD (determinisation)", "02_afn_afd", &ctr_afn_afd); A = r; break;
        case 2: r = aut_complete(&A); show_automaton(&r, "AFDC (automate complet)", "04_afdc", &ctr_afdc); A = r; break;
        case 3: r = aut_epsnfa_to_nfa(&A); show_automaton(&r, "AFN (eps-transitions supprimees)", "07_conversions", &ctr_conv); A = r; break;
        case 4: r = aut_to_epsnfa(&A); show_automaton(&r, "eps-AFN (conversion triviale)", "07_conversions", &ctr_conv); A = r; break;
        case 5: r = aut_dfa_to_nfa(&A); show_automaton(&r, "AFN (a partir de l'AFD)", "07_conversions", &ctr_conv); A = r; break;
        case 6: r = aut_dfa_to_epsnfa(&A); show_automaton(&r, "eps-AFN (a partir de l'AFD)", "07_conversions", &ctr_conv); A = r; break;
        case 7: r = aut_trim(&A); show_automaton(&r, "Automate emonde", "06_emondage", &ctr_emondage); A = r; break;
        case 8: r = aut_minimize(&A); show_automaton(&r, "AFD minimal (Moore)", "09_minimisation", &ctr_minim); A = r; break;
        case 9: r = aut_canonical(&A); show_automaton(&r, "Automate canonique A_L", "11_canonique", &ctr_canon); A = r; break;
        case 10: r = aut_mirror(&A); show_automaton(&r, "Automate miroir", "14_divers", &ctr_divers); A = r; break;
        default: return;
    }
    printf("\n(!) L'automate courant A a ete remplace par le resultat -- vous pouvez enchainer une autre operation dessus.\n");
}

static void menu_states(void) {
    printf("\n--- Etats de l'automate courant A ---\n");
    printf(" 1. epsilon-fermeture d'un etat donne\n");
    printf(" 2. Etats accessibles\n");
    printf(" 3. Etats co-accessibles\n");
    printf(" 4. Etats utiles\n");
    printf(" 0. Retour\n");
    int c = read_int("Choix : ");
    ensure_A(); if (!hasA) return;
    if (c == 1) {
        int q = read_int("Numero de l'etat : ");
        BitSet cl = eps_closure_state(&A, q);
        printf("epsilon-fermeture({%d}) = ", q); bitset_print(cl, A.nStates); printf("\n");
    } else if (c == 2) {
        BitSet s = accessible_states(&A);
        printf("Etats accessibles = "); bitset_print(s, A.nStates); printf("\n");
    } else if (c == 3) {
        BitSet s = coaccessible_states(&A);
        printf("Etats co-accessibles = "); bitset_print(s, A.nStates); printf("\n");
    } else if (c == 4) {
        BitSet s = useful_states(&A);
        printf("Etats utiles = "); bitset_print(s, A.nStates); printf("\n");
    }
    ctr_etats++;
}

static void menu_properties(void) {
    printf("\n--- Proprietes et tests sur l'automate courant A ---\n");
    printf(" 1. L'automate est-il deterministe ?\n");
    printf(" 2. L'automate est-il complet ?\n");
    printf(" 3. Le langage est-il vide ?\n");
    printf(" 4. Reconnaissance d'un mot (w appartient a L(A) ?)\n");
    printf(" 5. Enumeration des mots de L(A) jusqu'a une longueur donnee\n");
    printf(" 0. Retour\n");
    int c = read_int("Choix : ");
    ensure_A(); if (!hasA) return;
    if (c == 1) printf("Deterministe : %s\n", aut_is_deterministic(&A) ? "OUI" : "NON");
    else if (c == 2) printf("Complet : %s\n", aut_is_complete(&A) ? "OUI" : "NON");
    else if (c == 3) printf("Langage vide : %s\n", aut_is_empty(&A) ? "OUI" : "NON");
    else if (c == 4) {
        char w[128]; read_str("Mot a tester (sans espaces, epsilon = mot vide) : ", w, sizeof(w));
        int acc = aut_accepts(&A, w);
        printf("Le mot \"%s\" est %s par L(A)\n", w, acc ? "ACCEPTE" : "REJETE");
        ctr_mots++;
    } else if (c == 5) {
        int len = read_int("Longueur maximale (<= 6 recommande) : ");
        if (len > 8) { printf("(!) trop grand, limite a 8\n"); len = 8; }
        aut_enumerate(&A, len);
        ctr_mots++;
    }
}

static void menu_binary(void) {
    printf("\n--- Operations binaires (necessitent A et B) ---\n");
    printf(" 1. Definir l'automate B\n");
    printf(" 2. Union (produit d'automates completes)\n");
    printf(" 3. Union (construction parallele, eps-AFN, a la Thompson)\n");
    printf(" 4. Intersection (produit)\n");
    printf(" 5. Difference A \\ B\n");
    printf(" 6. Concatenation A.B\n");
    printf(" 7. Test d'equivalence L(A) = L(B) ?\n");
    printf(" 0. Retour\n");
    int c = read_int("Choix : ");
    if (c == 1) { menu_define_second(); return; }
    ensure_A(); if (!hasA) return;
    if (!hasB) { printf("(!) definissez d'abord B (option 1)\n"); return; }
    Automaton r;
    switch (c) {
        case 2: r = aut_union_product(&A, &B); show_automaton(&r, "Union (produit)", "12_clotures", &ctr_cloture); A = r; break;
        case 3: r = aut_union_parallel(&A, &B); show_automaton(&r, "Union (parallele eps-AFN)", "12_clotures", &ctr_cloture); A = r; break;
        case 4: r = aut_intersection(&A, &B); show_automaton(&r, "Intersection", "12_clotures", &ctr_cloture); A = r; break;
        case 5: r = aut_difference(&A, &B); show_automaton(&r, "Difference A\\B", "12_clotures", &ctr_cloture); A = r; break;
        case 6: r = aut_concat(&A, &B); show_automaton(&r, "Concatenation A.B", "12_clotures", &ctr_cloture); A = r; break;
        case 7: printf("\nL(A) = L(B) ? %s\n", aut_equivalent(&A, &B) ? "OUI (equivalents)" : "NON"); break;
        default: return;
    }
    if (c >= 2 && c <= 6) printf("\n(!) L'automate courant A contient maintenant le resultat.\n");
}

static void menu_unary_closure(void) {
    printf("\n--- Cloture unaire sur l'automate courant A ---\n");
    printf(" 1. Complement\n");
    printf(" 2. Etoile de Kleene\n");
    printf(" 0. Retour\n");
    int c = read_int("Choix : ");
    ensure_A(); if (!hasA) return;
    Automaton r;
    if (c == 1) { r = aut_complement(&A); show_automaton(&r, "Complement", "12_clotures", &ctr_cloture); A = r; }
    else if (c == 2) { r = aut_star(&A); show_automaton(&r, "Etoile de Kleene", "12_clotures", &ctr_cloture); A = r; }
}

static void menu_regex_ops(void) {
    printf("\n--- Operations sur l'expression reguliere courante ---\n");
    printf(" 1. Afficher l'expression courante\n");
    printf(" 2. Simplifier l'expression (regles algebriques du chapitre 2)\n");
    printf(" 3. Construction de Thompson (regex -> eps-AFN)\n");
    printf(" 4. Automate de Glushkov (regex -> AFN des positions)\n");
    printf(" 0. Retour\n");
    int c = read_int("Choix : ");
    ensure_regex(); if (!curRegex) return;
    if (c == 1) printf("Expression courante : %s\n", curRegexStr);
    else if (c == 2) {
        curRegex = regex_simplify(curRegex);
        regex_to_string_buf(curRegex, curRegexStr, sizeof(curRegexStr));
        printf("Expression simplifiee : %s\n", curRegexStr);
    } else if (c == 3) {
        A = thompson_build(curRegex); hasA = 1;
        char title[700]; snprintf(title, sizeof(title), "Thompson(%s)", curRegexStr);
        show_automaton(&A, title, "08_thompson", &ctr_thompson);
        printf("\n(!) L'automate courant A contient maintenant ce eps-AFN.\n");
    } else if (c == 4) {
        A = glushkov_build(curRegex); hasA = 1;
        char title[700]; snprintf(title, sizeof(title), "Glushkov(%s)", curRegexStr);
        show_automaton(&A, title, "10_glushkov", &ctr_glushkov);
        printf("\n(!) L'automate courant A contient maintenant cet AFN des positions.\n");
    }
}

static void menu_simulation(void) {
    ensure_A(); if (!hasA) return;
    char w[128];
    read_str("\nMot a simuler pas-a-pas : ", w, sizeof(w));
    printf("\nSimulation sur A :\n");
    BitSet cur = 0;
    for (int q = 0; q < A.nStates; q++) if (A.isInitial[q]) cur |= (1ULL << q);
    cur = eps_closure_set(&A, cur);
    printf("  etat(s) courant(s) apres epsilon-fermeture initiale : "); bitset_print(cur, A.nStates); printf("\n");
    for (const char *p = w; *p; p++) {
        int sy = aut_symidx(&A, *p);
        if (sy < 0) { printf("  (!) symbole '%c' hors alphabet -> rejet\n", *p); return; }
        cur = eps_closure_set(&A, move_set(&A, cur, sy));
        printf("  apres lecture de '%c' : ", *p); bitset_print(cur, A.nStates); printf("\n");
        if (cur == 0) { printf("  -> blocage : mot REJETE\n"); return; }
    }
    int acc = 0;
    for (int q = 0; q < A.nStates; q++) if ((cur >> q & 1ULL) && A.isFinal[q]) acc = 1;
    printf("  etat(s) final(aux) atteint(s), mot %s\n", acc ? "ACCEPTE" : "REJETE");
    ctr_mots++;
}

/* ============================== menu principal =================== */
static void print_main_menu(void) {
    printf("\n");
    printf("=========================================================================\n");
    printf("  INF3421 - LANGAGE FORMEL & COMPILATION -- UY1 / Departement Informatique\n");
    printf("  Boite a outils des automates finis et expressions regulieres\n");
    printf("=========================================================================\n");
    if (hasA) printf("  Automate courant A : %d etats, alphabet '%.*s'%s\n", A.nStates, A.nSymb, A.alphabet, A.hasEpsilon ? " (+eps)" : "");
    else printf("  Automate courant A : (non defini)\n");
    if (hasB) printf("  Automate secondaire B : %d etats\n", B.nStates);
    if (curRegex) printf("  Expression courante  : %s\n", curRegexStr);
    printf("-------------------------------------------------------------------------\n");
    printf("  1. Definir / generer l'automate courant A\n");
    printf("  2. Definir une expression reguliere courante\n");
    printf("  3. Systemes d'equations (Arden/Gauss) & extraction de regex\n");
    printf("  4. Transformations sur A (determinisation, completion, minimisation...)\n");
    printf("  5. Etats de A (accessibles / co-accessibles / utiles / eps-fermeture)\n");
    printf("  6. Proprietes et tests sur A (deterministe, complet, mot, enumeration)\n");
    printf("  7. Operations binaires (union, intersection, difference, concat, equiv.)\n");
    printf("  8. Cloture unaire (complement, etoile de Kleene)\n");
    printf("  9. Expression reguliere -> automate (Thompson, Glushkov) & simplification\n");
    printf(" 10. Simulation pas-a-pas d'un mot sur A\n");
    printf(" 11. Afficher A (table de transition + image PGM)\n");
    printf("  0. Quitter\n");
    printf("-------------------------------------------------------------------------\n");
}

int main(void) {
    printf("Bienvenue dans la boite a outils INF3421.\n");
    printf("Toutes les images generees sont enregistrees automatiquement dans results/\n");
    int running = 1;
    while (running) {
        print_main_menu();
        int c = read_int("Votre choix : ");
        switch (c) {
            case 1: menu_define_automaton(); break;
            case 2: menu_define_regex(); break;
            case 3: menu_equations(); break;
            case 4: menu_transform(); break;
            case 5: menu_states(); break;
            case 6: menu_properties(); break;
            case 7: menu_binary(); break;
            case 8: menu_unary_closure(); break;
            case 9: menu_regex_ops(); break;
            case 10: menu_simulation(); break;
            case 11: ensure_A(); if (hasA) show_automaton(&A, "Automate courant A", "14_divers", &ctr_divers); break;
            case 0: running = 0; printf("\nMerci d'avoir utilise la boite a outils INF3421. Au revoir !\n"); break;
            default: printf("Choix invalide.\n");
        }
        if (running) pause_enter();
    }
    return 0;
}
