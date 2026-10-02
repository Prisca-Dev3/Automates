/* ============================================================================
 * equation.c -- Lemme d'Arden / methode de Gauss + Thompson + Glushkov
 * INF3421 - UY1
 *
 * Idee cle : l'algorithme d'elimination d'etats (Brzozowski-McCluskey, section
 * 3.3.2 du cours) et la resolution d'un systeme d'equations lineaires en
 * langages (section 2.3.3) sont UNE SEULE ET MEME METHODE. On implemente donc
 * un unique moteur de resolution (eqsys_solve), reutilise a la fois pour :
 *   - l'operation menu "resolution d'un systeme d'equations" (saisie libre),
 *   - l'operation menu "extraction d'expression reguliere d'un automate".
 * ==========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "equation.h"

/* ============================= EqSystem ================================ */
void eqsys_init(EqSystem *s, int n) {
    s->n = n;
    for (int i = 0; i < n; i++) {
        s->B[i] = NULL;
        snprintf(s->names[i], 8, "X%d", i);
        for (int j = 0; j < n; j++) s->A[i][j] = NULL;
    }
}
void eqsys_print(const EqSystem *s, const char *title) {
    printf("\n--- %s ---\n", title);
    for (int i = 0; i < s->n; i++) {
        printf("  %s = ", s->names[i]);
        int first = 1;
        for (int j = 0; j < s->n; j++) if (s->A[i][j]) {
            char *cs = regex_to_string(s->A[i][j]);
            printf("%s%s.%s", first ? "" : " + ", cs, s->names[j]);
            free(cs); first = 0;
        }
        if (s->B[i]) {
            char *bs = regex_to_string(s->B[i]);
            printf("%s%s", first ? "" : " + ", bs);
            free(bs); first = 0;
        }
        if (first) printf("#");
        printf("\n");
    }
}

static EqSystem copy_system(const EqSystem *s) {
    EqSystem c; c.n = s->n;
    for (int i = 0; i < s->n; i++) {
        c.B[i] = r_copy(s->B[i]);
        strncpy(c.names[i], s->names[i], 7); c.names[i][7] = '\0';
        for (int j = 0; j < s->n; j++) c.A[i][j] = r_copy(s->A[i][j]);
    }
    return c;
}
static RNode *cb_union(RNode *a, RNode *b) {
    if (!a) return b;
    if (!b) return a;
    return regex_simplify(r_union(a, b));
}
static RNode *cb_concat(RNode *a, RNode *b) {
    if (!a || !b) return NULL;
    return regex_simplify(r_concat(a, b));
}
static RNode *cb_star(RNode *a) {
    if (!a) return r_eps();
    return regex_simplify(r_star(a));
}

/* Elimine l'inconnue Xk du systeme (in place), par la resolvante partielle
 * de Xk (lemme d'Arden) suivie de la substitution dans toutes les autres
 * equations -- exactement l'operation illustree figure 3.20 du cours.      */
static void eliminate(EqSystem *s, int k, int verbose) {
    RNode *loopStar = cb_star(s->A[k][k]);
    for (int i = 0; i < s->n; i++) {
        if (i == k || !s->A[i][k]) continue;
        RNode *eik = s->A[i][k];
        for (int j = 0; j < s->n; j++) {
            if (j == k || !s->A[k][j]) continue;
            RNode *add = cb_concat(cb_concat(eik, loopStar), s->A[k][j]);
            s->A[i][j] = cb_union(s->A[i][j], add);
        }
        if (s->B[k]) {
            RNode *add = cb_concat(cb_concat(eik, loopStar), s->B[k]);
            s->B[i] = cb_union(s->B[i], add);
        }
        s->A[i][k] = NULL;
    }
    s->A[k][k] = NULL;
    for (int j = 0; j < s->n; j++) s->A[k][j] = NULL;
    if (verbose) printf("  elimination de %s (resolvante : %s = ...*...)\n", s->names[k], s->names[k]);
}

RNode *eqsys_solve_for(EqSystem *orig, int keep, int verbose) {
    EqSystem s = copy_system(orig);
    if (verbose) printf("\n[Gauss] resolution pour %s :\n", s.names[keep]);
    for (int k = 0; k < s.n; k++) if (k != keep) eliminate(&s, k, verbose);
    RNode *loopStar = cb_star(s.A[keep][keep]);
    RNode *result = cb_concat(loopStar, s.B[keep] ? s.B[keep] : r_empty());
    if (!result) result = r_empty();
    return regex_simplify(result);
}

RNode **eqsys_solve(EqSystem *s, int verbose) {
    RNode **sols = (RNode**)malloc(sizeof(RNode*) * (size_t)s->n);
    for (int i = 0; i < s->n; i++) {
        sols[i] = eqsys_solve_for(s, i, verbose);
        if (verbose) {
            char *str = regex_to_string(sols[i]);
            printf("  ==> %s = %s\n", s->names[i], str);
            free(str);
        }
    }
    return sols;
}

/* ==================== Extraction d'expression reguliere d'un automate ===
 * On construit le systeme Lq = (union sur r) Zq,r.Lr + Fq  (section 3.2.1)
 * et on reutilise le solveur ci-dessus.                                    */
char *aut_to_regex(const Automaton *a) {
    EqSystem s; eqsys_init(&s, a->nStates);
    for (int q = 0; q < a->nStates; q++) {
        snprintf(s.names[q], 8, "L%d", q);
        for (int q2 = 0; q2 < a->nStates; q2++) {
            RNode *z = NULL;
            for (int sy = 0; sy < a->nSymb; sy++) if (a->delta[q][sy][q2]) {
                RNode *sym = r_sym(a->alphabet[sy]);
                z = z ? r_union(z, sym) : sym;
            }
            if (a->hasEpsilon && a->delta[q][EPS_IDX][q2]) z = z ? r_union(z, r_eps()) : r_eps();
            s.A[q][q2] = z;
        }
        s.B[q] = a->isFinal[q] ? r_eps() : NULL;
    }
    eqsys_print(&s, "Systeme associe a l'automate : Lq = U Zq,r.Lr + Fq");
    RNode **sols = eqsys_solve(&s, 1);
    RNode *final = NULL;
    for (int q = 0; q < a->nStates; q++) if (a->isInitial[q]) final = final ? r_union(final, sols[q]) : sols[q];
    if (!final) final = r_empty();
    final = regex_simplify(final);
    char *res = regex_to_string(final);
    free(sols);
    return res;
}

/* ================================ Thompson =============================
 * Construction "pure" (section 3.3.2, figures 3.13 a 3.18) : etat initial
 * unique sans transition entrante, etat final unique sans transition
 * sortante, exactement deux etats de plus par symbole de l'expression.     */
static void collect_symbols(const RNode *r, char *alpha, int *n) {
    if (!r) return;
    if (r->type == R_SYMBOL) {
        int found = 0;
        for (int i = 0; i < *n; i++) if (alpha[i] == r->symbol) found = 1;
        if (!found && *n < MAX_SYMB) alpha[(*n)++] = r->symbol;
    }
    collect_symbols(r->left, alpha, n);
    collect_symbols(r->right, alpha, n);
}

static Automaton th_leaf(RType t, char sym, const char *alpha, int n) {
    Automaton a; aut_init(&a, 2, alpha, 1);
    a.isInitial[0] = 1; a.isFinal[1] = 1;
    if (t == R_EPSILON) a.delta[0][EPS_IDX][1] = 1;
    else if (t == R_SYMBOL) { int si = aut_symidx(&a, sym); if (si >= 0) a.delta[0][si][1] = 1; }
    (void)n;
    return a;
}
static Automaton th_union(Automaton a1, Automaton a2, const char *alpha) {
    int total = a1.nStates + a2.nStates + 2; if (total > MAX_STATES) total = MAX_STATES;
    Automaton r; aut_init(&r, total, alpha, 1);
    int off1 = 2, off2 = 2 + a1.nStates, q0 = 0, qF = 1;
    r.isInitial[q0] = 1; r.isFinal[qF] = 1;
    int i1 = aut_unique_initial(&a1), i2 = aut_unique_initial(&a2);
    int f1 = -1, f2 = -1;
    for (int q = 0; q < a1.nStates; q++) if (a1.isFinal[q]) f1 = q;
    for (int q = 0; q < a2.nStates; q++) if (a2.isFinal[q]) f2 = q;
    if (off1 + i1 < MAX_STATES) r.delta[q0][EPS_IDX][off1 + i1] = 1;
    if (off2 + i2 < MAX_STATES) r.delta[q0][EPS_IDX][off2 + i2] = 1;
    if (f1 >= 0 && off1 + f1 < MAX_STATES) r.delta[off1 + f1][EPS_IDX][qF] = 1;
    if (f2 >= 0 && off2 + f2 < MAX_STATES) r.delta[off2 + f2][EPS_IDX][qF] = 1;
    int m1 = a1.nSymb + 1;
    for (int sy = 0; sy < m1; sy++) { int si = (sy == a1.nSymb) ? EPS_IDX : sy;
        for (int q = 0; q < a1.nStates; q++) for (int q2 = 0; q2 < a1.nStates; q2++)
            if (a1.delta[q][si][q2] && off1 + q < MAX_STATES && off1 + q2 < MAX_STATES) r.delta[off1 + q][si][off1 + q2] = 1;
    }
    int m2 = a2.nSymb + 1;
    for (int sy = 0; sy < m2; sy++) { int si = (sy == a2.nSymb) ? EPS_IDX : sy;
        for (int q = 0; q < a2.nStates; q++) for (int q2 = 0; q2 < a2.nStates; q2++)
            if (a2.delta[q][si][q2] && off2 + q < MAX_STATES && off2 + q2 < MAX_STATES) r.delta[off2 + q][si][off2 + q2] = 1;
    }
    return r;
}
static Automaton th_concat(Automaton a1, Automaton a2, const char *alpha) {
    int total = a1.nStates + a2.nStates; if (total > MAX_STATES) total = MAX_STATES;
    Automaton r; aut_init(&r, total, alpha, 1);
    int off2 = a1.nStates;
    int i1 = aut_unique_initial(&a1), i2 = aut_unique_initial(&a2);
    int f1 = -1, f2 = -1;
    for (int q = 0; q < a1.nStates; q++) if (a1.isFinal[q]) f1 = q;
    for (int q = 0; q < a2.nStates; q++) if (a2.isFinal[q]) f2 = q;
    r.isInitial[i1] = 1;
    if (off2 + f2 < MAX_STATES) r.isFinal[off2 + f2] = 1;
    int m1 = a1.nSymb + 1;
    for (int sy = 0; sy < m1; sy++) { int si = (sy == a1.nSymb) ? EPS_IDX : sy;
        for (int q = 0; q < a1.nStates; q++) for (int q2 = 0; q2 < a1.nStates; q2++) if (a1.delta[q][si][q2]) r.delta[q][si][q2] = 1;
    }
    int m2 = a2.nSymb + 1;
    for (int sy = 0; sy < m2; sy++) { int si = (sy == a2.nSymb) ? EPS_IDX : sy;
        for (int q = 0; q < a2.nStates; q++) for (int q2 = 0; q2 < a2.nStates; q2++)
            if (a2.delta[q][si][q2] && off2 + q < MAX_STATES && off2 + q2 < MAX_STATES) r.delta[off2 + q][si][off2 + q2] = 1;
    }
    if (f1 >= 0 && off2 + i2 < MAX_STATES) r.delta[f1][EPS_IDX][off2 + i2] = 1;
    return r;
}
static Automaton th_star(Automaton a1, const char *alpha) {
    int total = a1.nStates + 2; if (total > MAX_STATES) total = MAX_STATES;
    Automaton r; aut_init(&r, total, alpha, 1);
    int q0 = a1.nStates, qF = a1.nStates + 1;
    if (qF >= MAX_STATES) { qF = MAX_STATES - 1; q0 = MAX_STATES - 2; }
    r.isInitial[q0] = 1; r.isFinal[qF] = 1;
    r.delta[q0][EPS_IDX][qF] = 1;
    int i1 = aut_unique_initial(&a1), f1 = -1;
    for (int q = 0; q < a1.nStates; q++) if (a1.isFinal[q]) f1 = q;
    r.delta[q0][EPS_IDX][i1] = 1;
    if (f1 >= 0) { r.delta[f1][EPS_IDX][i1] = 1; r.delta[f1][EPS_IDX][qF] = 1; }
    int m1 = a1.nSymb + 1;
    for (int sy = 0; sy < m1; sy++) { int si = (sy == a1.nSymb) ? EPS_IDX : sy;
        for (int q = 0; q < a1.nStates; q++) for (int q2 = 0; q2 < a1.nStates; q2++) if (a1.delta[q][si][q2]) r.delta[q][si][q2] = 1;
    }
    return r;
}
static Automaton thompson_rec(const RNode *r, const char *alpha, int n) {
    switch (r->type) {
        case R_EMPTY: case R_EPSILON: case R_SYMBOL:
            return th_leaf(r->type, r->symbol, alpha, n);
        case R_UNION: {
            Automaton a1 = thompson_rec(r->left, alpha, n), a2 = thompson_rec(r->right, alpha, n);
            return th_union(a1, a2, alpha);
        }
        case R_CONCAT: {
            Automaton a1 = thompson_rec(r->left, alpha, n), a2 = thompson_rec(r->right, alpha, n);
            return th_concat(a1, a2, alpha);
        }
        case R_STAR: {
            Automaton a1 = thompson_rec(r->left, alpha, n);
            return th_star(a1, alpha);
        }
    }
    Automaton e; aut_init(&e, 1, alpha, 1); return e;
}
Automaton thompson_build(const RNode *r) {
    char alpha[MAX_SYMB + 1]; int n = 0;
    collect_symbols(r, alpha, &n);
    alpha[n] = '\0';
    return thompson_rec(r, alpha, n);
}

/* ================================ Glushkov ==============================
 * Automate des positions : on numerote chaque occurrence de symbole (1..k),
 * on calcule nullable/first/last par induction sur l'arbre, puis follow(p)
 * par parcours des noeuds concat/star ; l'automate a k+1 etats (0..k).     */
#define MAXPOS (MAX_STATES - 1)
static char   g_posSymbol[MAXPOS + 1];
static int    g_nPos;
static BitSet g_follow[MAXPOS + 1];

static void number_positions(RNode *r) {
    if (!r) return;
    if (r->type == R_SYMBOL) {
        g_nPos++;
        if (g_nPos <= MAXPOS) { r->pos = g_nPos; g_posSymbol[g_nPos] = r->symbol; }
    }
    number_positions(r->left);
    number_positions(r->right);
}
static int glu_nullable(const RNode *r) {
    switch (r->type) {
        case R_EMPTY: return 0;
        case R_EPSILON: return 1;
        case R_SYMBOL: return 0;
        case R_UNION: return glu_nullable(r->left) || glu_nullable(r->right);
        case R_CONCAT: return glu_nullable(r->left) && glu_nullable(r->right);
        case R_STAR: return 1;
    }
    return 0;
}
static BitSet glu_first(const RNode *r) {
    switch (r->type) {
        case R_EMPTY: case R_EPSILON: return 0;
        case R_SYMBOL: return (BitSet)1 << (r->pos - 1);
        case R_UNION: return glu_first(r->left) | glu_first(r->right);
        case R_CONCAT: return glu_first(r->left) | (glu_nullable(r->left) ? glu_first(r->right) : 0);
        case R_STAR: return glu_first(r->left);
    }
    return 0;
}
static BitSet glu_last(const RNode *r) {
    switch (r->type) {
        case R_EMPTY: case R_EPSILON: return 0;
        case R_SYMBOL: return (BitSet)1 << (r->pos - 1);
        case R_UNION: return glu_last(r->left) | glu_last(r->right);
        case R_CONCAT: return glu_last(r->right) | (glu_nullable(r->right) ? glu_last(r->left) : 0);
        case R_STAR: return glu_last(r->left);
    }
    return 0;
}
static void glu_follow(const RNode *r) {
    if (!r) return;
    if (r->type == R_CONCAT) {
        BitSet lst = glu_last(r->left), fst = glu_first(r->right);
        for (int p = 1; p <= g_nPos; p++) if ((lst >> (p - 1)) & 1ULL) g_follow[p] |= fst;
    } else if (r->type == R_STAR) {
        BitSet lst = glu_last(r->left), fst = glu_first(r->left);
        for (int p = 1; p <= g_nPos; p++) if ((lst >> (p - 1)) & 1ULL) g_follow[p] |= fst;
    }
    glu_follow(r->left);
    glu_follow(r->right);
}
Automaton glushkov_build(const RNode *rIn) {
    RNode *r = r_copy(rIn);
    g_nPos = 0;
    for (int i = 0; i <= MAXPOS; i++) g_follow[i] = 0;
    number_positions(r);
    if (g_nPos > MAXPOS) fprintf(stderr, "[glushkov] expression trop grande : troncature a %d positions\n", MAXPOS);
    glu_follow(r);
    char alpha[MAX_SYMB + 1]; int na = 0; collect_symbols(r, alpha, &na); alpha[na] = '\0';
    Automaton a; aut_init(&a, g_nPos + 1, alpha, 0);
    a.isInitial[0] = 1;
    if (glu_nullable(r)) a.isFinal[0] = 1;
    BitSet fst = glu_first(r), lst = glu_last(r);
    for (int p = 1; p <= g_nPos; p++) {
        if ((fst >> (p - 1)) & 1ULL) { int si = aut_symidx(&a, g_posSymbol[p]); if (si >= 0) a.delta[0][si][p] = 1; }
        if ((lst >> (p - 1)) & 1ULL) a.isFinal[p] = 1;
        snprintf(a.name[p], NAME_LEN, "p%d", p);
        for (int q = 1; q <= g_nPos; q++) if ((g_follow[p] >> (q - 1)) & 1ULL) {
            int si = aut_symidx(&a, g_posSymbol[q]);
            if (si >= 0) a.delta[p][si][q] = 1;
        }
    }
    printf("\n[Glushkov] %d position(s) numerotee(s) :\n", g_nPos);
    for (int p = 1; p <= g_nPos; p++) printf("   position %d -> symbole '%c'\n", p, g_posSymbol[p]);
    printf("First(E) = "); bitset_print(fst, g_nPos); printf("   Last(E) = "); bitset_print(lst, g_nPos);
    printf("   nullable(E) = %s\n", glu_nullable(r) ? "oui" : "non");
    for (int p = 1; p <= g_nPos; p++) { printf("Follow(%d) = ", p); bitset_print(g_follow[p], g_nPos); printf("\n"); }
    regex_free(r);
    return a;
}
