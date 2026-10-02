/* ============================================================================
 * automaton.h -- Structure de base des automates finis + tous les algorithmes
 * INF3421 - Langage Formel & Compilation - UY1 - Licence 3 Info
 *
 * Un seul type "Automaton" représente indifféremment :
 *   - un AFD  (déterministe, complet ou partiel)
 *   - un AFN  (non-déterministe)
 *   - un epsilon-AFN (non-déterministe avec transitions spontanées, hasEpsilon=1)
 * conformément au cours (définitions 1, 4, 6 et 7).
 * ==========================================================================*/
#ifndef AUTOMATON_H
#define AUTOMATON_H

#define MAX_STATES 60
#define MAX_SYMB   10
#define EPS_IDX    MAX_SYMB   /* indice réservé à la transition spontanée epsilon */
#define NAME_LEN   40

typedef unsigned long long BitSet;   /* sous-ensemble d'états (<=60 bits utilisés) */

typedef struct {
    int  nStates;
    int  nSymb;
    char alphabet[MAX_SYMB];
    int  hasEpsilon;                 /* 1 si epsilon-AFN */
    unsigned char delta[MAX_STATES][MAX_SYMB + 1][MAX_STATES];
    unsigned char isInitial[MAX_STATES];
    unsigned char isFinal[MAX_STATES];
    char name[MAX_STATES][NAME_LEN]; /* étiquette d'affichage (ex: "{1,2}") */
} Automaton;

/* ---- construction de base ---- */
void aut_init(Automaton *a, int nStates, const char *alphabet, int hasEpsilon);
int  aut_symidx(const Automaton *a, char c);
void aut_add_trans(Automaton *a, int from, char sym, int to); /* sym = '@' pour epsilon */
void aut_set_initial(Automaton *a, int q, int on);
void aut_set_final(Automaton *a, int q, int on);
void aut_default_names(Automaton *a);
int  aut_unique_initial(const Automaton *a); /* -1 si non unique */

/* ---- affichage ---- */
void aut_print(const Automaton *a, const char *title);
void aut_render_pgm(const Automaton *a, const char *path, const char *title);

/* ---- propriétés ---- */
int  aut_is_deterministic(const Automaton *a);
int  aut_is_complete(const Automaton *a);
int  aut_is_empty(const Automaton *a);      /* langage vide ? */
int  aut_accepts(const Automaton *a, const char *word);
void aut_enumerate(const Automaton *a, int maxLen);

/* ---- ensembles d'états ---- */
BitSet eps_closure_state(const Automaton *a, int q);
BitSet eps_closure_set(const Automaton *a, BitSet set);
BitSet move_set(const Automaton *a, BitSet set, int symIdx);
void   bitset_print(BitSet s, int n);
BitSet accessible_states(const Automaton *a);
BitSet coaccessible_states(const Automaton *a);
BitSet useful_states(const Automaton *a);

/* ---- transformations fondamentales ---- */
Automaton aut_complete(const Automaton *a);          /* AFD -> AFDC (etat puits) */
Automaton aut_determinize(const Automaton *a);        /* AFN/eps-AFN -> AFD (sous-ensembles) */
Automaton aut_epsnfa_to_nfa(const Automaton *a);      /* suppression des eps-transitions */
Automaton aut_to_epsnfa(const Automaton *a);          /* AFN -> eps-AFN (trivial) */
Automaton aut_dfa_to_nfa(const Automaton *a);         /* trivial (cast conceptuel) */
Automaton aut_dfa_to_epsnfa(const Automaton *a);      /* trivial (cast conceptuel) */
Automaton aut_trim(const Automaton *a);               /* emondage (etats utiles) */
Automaton aut_minimize(const Automaton *a);           /* algorithme de Moore */
Automaton aut_canonical(const Automaton *a);          /* automate canonique A_L complet */
Automaton aut_mirror(const Automaton *a);             /* automate miroir (langage renverse) */

/* ---- opérations de clôture ---- */
Automaton aut_union_product(const Automaton *a1, const Automaton *a2);   /* Theoreme 5 */
Automaton aut_union_parallel(const Automaton *a1, const Automaton *a2);  /* union eps-AFN */
Automaton aut_intersection(const Automaton *a1, const Automaton *a2);    /* Theoreme 6 */
Automaton aut_difference(const Automaton *a1, const Automaton *a2);
Automaton aut_complement(const Automaton *a);                            /* Theoreme 4 */
Automaton aut_concat(const Automaton *a1, const Automaton *a2);          /* Theoreme 8 */
Automaton aut_star(const Automaton *a);                                  /* Theoreme 9 */
int       aut_equivalent(const Automaton *a1, const Automaton *a2);

/* ---- expression reguliere associee a un automate (elimination d'etats / BMC) ---- */
char *aut_to_regex(const Automaton *a);

#endif
