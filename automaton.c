/* ============================================================================
 * automaton.c -- Tous les algorithmes sur les automates finis (chapitres 3)
 * INF3421 - UY1
 * ==========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "automaton.h"
#include "pgm.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ==================== construction de base =============================*/
void aut_default_names(Automaton *a) {
    for (int q = 0; q < a->nStates; q++) snprintf(a->name[q], NAME_LEN, "%d", q);
}
void aut_init(Automaton *a, int nStates, const char *alphabet, int hasEpsilon) {
    memset(a, 0, sizeof(Automaton));
    a->nStates = nStates > MAX_STATES ? MAX_STATES : nStates;
    a->nSymb = (int)strlen(alphabet);
    if (a->nSymb > MAX_SYMB) a->nSymb = MAX_SYMB;
    memcpy(a->alphabet, alphabet, (size_t)a->nSymb);
    a->hasEpsilon = hasEpsilon;
    aut_default_names(a);
}
int aut_symidx(const Automaton *a, char c) {
    for (int i = 0; i < a->nSymb; i++) if (a->alphabet[i] == c) return i;
    return -1;
}
void aut_add_trans(Automaton *a, int from, char sym, int to) {
    if (from < 0 || from >= a->nStates || to < 0 || to >= a->nStates) {
        fprintf(stderr, "[transition] etat hors bornes (%d,%c,%d)\n", from, sym, to);
        return;
    }
    if (sym == '@') {
        if (!a->hasEpsilon) { fprintf(stderr, "[transition] automate sans epsilon-transitions\n"); return; }
        a->delta[from][EPS_IDX][to] = 1;
        return;
    }
    int sy = aut_symidx(a, sym);
    if (sy < 0) { fprintf(stderr, "[transition] symbole '%c' absent de l'alphabet\n", sym); return; }
    a->delta[from][sy][to] = 1;
}
void aut_set_initial(Automaton *a, int q, int on) { if (q >= 0 && q < a->nStates) a->isInitial[q] = (unsigned char)on; }
void aut_set_final(Automaton *a, int q, int on)   { if (q >= 0 && q < a->nStates) a->isFinal[q]   = (unsigned char)on; }
int  aut_unique_initial(const Automaton *a) {
    int r = -1, cnt = 0;
    for (int q = 0; q < a->nStates; q++) if (a->isInitial[q]) { r = q; cnt++; }
    return cnt == 1 ? r : -1;
}

/* ==================== affichage table =============================*/
void aut_print(const Automaton *a, const char *title) {
    printf("\n===================================================================\n");
    printf(" %s\n", title);
    printf("===================================================================\n");
    printf("Etats (%d)      : ", a->nStates);
    for (int q = 0; q < a->nStates; q++) printf("%d ", q);
    printf("\nAlphabet (%d)   : ", a->nSymb);
    for (int i = 0; i < a->nSymb; i++) printf("%c ", a->alphabet[i]);
    if (a->hasEpsilon) printf("  [+ epsilon-transitions autorisees]");
    printf("\nEtats initiaux : {");
    int first = 1;
    for (int q = 0; q < a->nStates; q++) if (a->isInitial[q]) { printf("%s%d", first ? "" : ",", q); first = 0; }
    printf("}\nEtats finaux   : {");
    first = 1;
    for (int q = 0; q < a->nStates; q++) if (a->isFinal[q]) { printf("%s%d", first ? "" : ",", q); first = 0; }
    printf("}\n");
    printf("Deterministe   : %s\nComplet        : %s\n",
           aut_is_deterministic(a) ? "oui" : "non", aut_is_complete(a) ? "oui" : "non");
    printf("\nTable de transition delta :\n%-6s", "etat");
    for (int sy = 0; sy < a->nSymb; sy++) printf("%-10c", a->alphabet[sy]);
    if (a->hasEpsilon) printf("%-10s", "eps");
    printf("\n");
    for (int q = 0; q < a->nStates; q++) {
        printf("%-6d", q);
        for (int sy = 0; sy < a->nSymb; sy++) {
            char buf[64] = ""; int idx = 0, f2 = 1;
            for (int q2 = 0; q2 < a->nStates; q2++) if (a->delta[q][sy][q2])
                { idx += snprintf(buf + idx, sizeof(buf) - idx, "%s%d", f2 ? "" : "|", q2); f2 = 0; }
            printf("%-10s", idx ? buf : "-");
        }
        if (a->hasEpsilon) {
            char buf[64] = ""; int idx = 0, f2 = 1;
            for (int q2 = 0; q2 < a->nStates; q2++) if (a->delta[q][EPS_IDX][q2])
                { idx += snprintf(buf + idx, sizeof(buf) - idx, "%s%d", f2 ? "" : "|", q2); f2 = 0; }
            printf("%-10s", idx ? buf : "-");
        }
        printf("\n");
    }
}

/* ==================== rendu graphique PGM =============================*/
void aut_render_pgm(const Automaton *a, const char *path, const char *title) {
    int W = 960, H = 900;
    Canvas c = canvas_create(W, H, 255);
    int n = a->nStates;
    canvas_text(&c, 20, 15, 2, title, 0);
    if (n == 0) { canvas_save_pgm(&c, path); canvas_free(&c); return; }
    int cx = W / 2, cy = H / 2 + 30, R = (n <= 1) ? 0 : 280, r = 27;
    if (n > 14) r = 20;
    int px[MAX_STATES], py[MAX_STATES];
    for (int q = 0; q < n; q++) {
        double ang = -M_PI / 2 + 2 * M_PI * q / (n == 1 ? 1 : n);
        px[q] = cx + (int)(R * cos(ang));
        py[q] = cy + (int)(R * sin(ang));
        if (n == 1) { px[q] = cx; py[q] = cy; }
    }
    /* transitions (regroupees par paire d'etats) */
    for (int q = 0; q < n; q++) {
        for (int q2 = 0; q2 < n; q2++) {
            char lbl[80] = ""; int has = 0;
            for (int sy = 0; sy < a->nSymb; sy++) if (a->delta[q][sy][q2]) {
                if (has) strcat(lbl, ",");
                char tmp[2] = { a->alphabet[sy], 0 }; strcat(lbl, tmp); has = 1;
            }
            if (a->hasEpsilon && a->delta[q][EPS_IDX][q2]) { if (has) strcat(lbl, ","); strcat(lbl, "@"); has = 1; }
            if (!has) continue;
            if (q == q2) {
                canvas_self_loop(&c, px[q], py[q], r, 60);
                canvas_text(&c, px[q] - 10, py[q] - r - 42, 2, lbl, 60);
            } else {
                int rev = 0;
                for (int sy = 0; sy < a->nSymb; sy++) if (a->delta[q2][sy][q]) rev = 1;
                if (a->hasEpsilon && a->delta[q2][EPS_IDX][q]) rev = 1;
                double bend = rev ? 34.0 : 0.0;
                double ddx = px[q2] - px[q], ddy = py[q2] - py[q];
                double dl = sqrt(ddx * ddx + ddy * ddy); if (dl < 1) dl = 1;
                int sx = px[q] + (int)(ddx / dl * r), sy0 = py[q] + (int)(ddy / dl * r);
                int ex = px[q2] - (int)(ddx / dl * r), ey = py[q2] - (int)(ddy / dl * r);
                canvas_curved_arc(&c, sx, sy0, ex, ey, bend, 60, 1);
                int mx = (sx + ex) / 2, my = (sy0 + ey) / 2;
                double nx = -ddy / dl, ny = ddx / dl;
                mx += (int)(nx * (bend + 14)); my += (int)(ny * (bend + 14));
                canvas_text(&c, mx - 4, my - 4, 2, lbl, 60);
            }
        }
    }
    /* etats */
    for (int q = 0; q < n; q++) {
        canvas_circle_fill(&c, px[q], py[q], r, 255);
        canvas_circle(&c, px[q], py[q], r, 0);
        if (a->isFinal[q]) canvas_circle(&c, px[q], py[q], r - 5, 0);
        if (a->isInitial[q]) {
            int ax = px[q] - (int)(1.9 * r), ay = py[q];
            canvas_thick_line(&c, ax, ay, px[q] - r, py[q], 0, 1);
            canvas_arrowhead(&c, ax, ay, px[q] - r, py[q], 0);
        }
        char buf[8]; snprintf(buf, sizeof(buf), "%d", q);
        int tw = canvas_text_width(buf, 2);
        canvas_text(&c, px[q] - tw / 2, py[q] - 7, 2, buf, 0);
    }
    canvas_save_pgm(&c, path);
    canvas_free(&c);
    printf("  [image] %s\n", path);
}

/* ==================== proprietes =============================*/
int aut_is_deterministic(const Automaton *a) {
    if (a->hasEpsilon)
        for (int q = 0; q < a->nStates; q++)
            for (int q2 = 0; q2 < a->nStates; q2++) if (a->delta[q][EPS_IDX][q2]) return 0;
    if (aut_unique_initial(a) < 0) return 0;
    for (int q = 0; q < a->nStates; q++)
        for (int sy = 0; sy < a->nSymb; sy++) {
            int cnt = 0;
            for (int q2 = 0; q2 < a->nStates; q2++) cnt += a->delta[q][sy][q2];
            if (cnt > 1) return 0;
        }
    return 1;
}
int aut_is_complete(const Automaton *a) {
    if (!aut_is_deterministic(a)) return 0;
    for (int q = 0; q < a->nStates; q++)
        for (int sy = 0; sy < a->nSymb; sy++) {
            int cnt = 0;
            for (int q2 = 0; q2 < a->nStates; q2++) cnt += a->delta[q][sy][q2];
            if (cnt != 1) return 0;
        }
    return 1;
}

/* ==================== ensembles d'etats (bitsets) =============================*/
BitSet eps_closure_state(const Automaton *a, int q) {
    BitSet result = (1ULL << q);
    if (!a->hasEpsilon) return result;
    int stack[MAX_STATES], sp = 0;
    stack[sp++] = q;
    while (sp > 0) {
        int t = stack[--sp];
        for (int u = 0; u < a->nStates; u++)
            if (a->delta[t][EPS_IDX][u] && !((result >> u) & 1ULL)) { result |= (1ULL << u); stack[sp++] = u; }
    }
    return result;
}
BitSet eps_closure_set(const Automaton *a, BitSet set) {
    BitSet result = 0;
    for (int q = 0; q < a->nStates; q++) if ((set >> q) & 1ULL) result |= eps_closure_state(a, q);
    return result;
}
BitSet move_set(const Automaton *a, BitSet set, int symIdx) {
    BitSet result = 0;
    for (int q = 0; q < a->nStates; q++) if ((set >> q) & 1ULL)
        for (int q2 = 0; q2 < a->nStates; q2++) if (a->delta[q][symIdx][q2]) result |= (1ULL << q2);
    return result;
}
void bitset_print(BitSet s, int n) {
    printf("{");
    int first = 1;
    for (int q = 0; q < n; q++) if ((s >> q) & 1ULL) { printf("%s%d", first ? "" : ",", q); first = 0; }
    printf("}");
}

BitSet accessible_states(const Automaton *a) {
    BitSet acc = 0;
    for (int q = 0; q < a->nStates; q++) if (a->isInitial[q]) acc |= (1ULL << q);
    acc = eps_closure_set(a, acc);
    int changed = 1;
    while (changed) {
        changed = 0;
        BitSet cur = acc;
        int maxSym = a->hasEpsilon ? a->nSymb + 1 : a->nSymb;
        for (int q = 0; q < a->nStates; q++) if ((cur >> q) & 1ULL) {
            for (int sy = 0; sy < maxSym; sy++) {
                int si = (sy == a->nSymb) ? EPS_IDX : sy;
                for (int q2 = 0; q2 < a->nStates; q2++)
                    if (a->delta[q][si][q2] && !((acc >> q2) & 1ULL)) { acc |= (1ULL << q2); changed = 1; }
            }
        }
    }
    return acc;
}
BitSet coaccessible_states(const Automaton *a) {
    BitSet co = 0;
    for (int q = 0; q < a->nStates; q++) if (a->isFinal[q]) co |= (1ULL << q);
    int changed = 1;
    int maxSym = a->hasEpsilon ? a->nSymb + 1 : a->nSymb;
    while (changed) {
        changed = 0;
        for (int q = 0; q < a->nStates; q++) {
            if ((co >> q) & 1ULL) continue;
            int reach = 0;
            for (int sy = 0; sy < maxSym && !reach; sy++) {
                int si = (sy == a->nSymb) ? EPS_IDX : sy;
                for (int q2 = 0; q2 < a->nStates; q2++)
                    if (a->delta[q][si][q2] && ((co >> q2) & 1ULL)) { reach = 1; break; }
            }
            if (reach) { co |= (1ULL << q); changed = 1; }
        }
    }
    return co;
}
BitSet useful_states(const Automaton *a) { return accessible_states(a) & coaccessible_states(a); }

/* ==================== emondage =============================*/
Automaton aut_trim(const Automaton *a) {
    BitSet u = useful_states(a);
    int map[MAX_STATES], cnt = 0;
    for (int q = 0; q < a->nStates; q++) map[q] = ((u >> q) & 1ULL) ? cnt++ : -1;
    Automaton r;
    aut_init(&r, cnt > 0 ? cnt : 1, a->alphabet, a->hasEpsilon);
    if (cnt == 0) { fprintf(stderr, "[emondage] aucun etat utile -> langage vide\n"); return r; }
    for (int q = 0; q < a->nStates; q++) if (map[q] >= 0) {
        r.isInitial[map[q]] = a->isInitial[q];
        r.isFinal[map[q]] = a->isFinal[q];
        strncpy(r.name[map[q]], a->name[q], NAME_LEN - 1);
    }
    int maxSym = a->hasEpsilon ? a->nSymb + 1 : a->nSymb;
    for (int sy = 0; sy < maxSym; sy++) { int si = (sy == a->nSymb) ? EPS_IDX : sy;
        for (int q = 0; q < a->nStates; q++) if (map[q] >= 0)
            for (int q2 = 0; q2 < a->nStates; q2++) if (map[q2] >= 0 && a->delta[q][si][q2])
                r.delta[map[q]][si][map[q2]] = 1;
    }
    return r;
}

/* ==================== completion (AFD -> AFDC) =============================*/
Automaton aut_complete(const Automaton *a) {
    Automaton c = *a;
    int need = 0;
    for (int q = 0; q < a->nStates && !need; q++)
        for (int sy = 0; sy < a->nSymb; sy++) {
            int cnt = 0;
            for (int q2 = 0; q2 < a->nStates; q2++) cnt += a->delta[q][sy][q2];
            if (cnt == 0) { need = 1; break; }
        }
    if (!need) return c;
    int trap = a->nStates;
    if (trap >= MAX_STATES) { fprintf(stderr, "[completion] plus de place pour l'etat puits\n"); return c; }
    c.nStates = a->nStates + 1;
    snprintf(c.name[trap], NAME_LEN, "puits");
    for (int sy = 0; sy < c.nSymb; sy++) c.delta[trap][sy][trap] = 1;
    for (int q = 0; q < a->nStates; q++)
        for (int sy = 0; sy < a->nSymb; sy++) {
            int cnt = 0;
            for (int q2 = 0; q2 < a->nStates; q2++) cnt += a->delta[q][sy][q2];
            if (cnt == 0) c.delta[q][sy][trap] = 1;
        }
    return c;
}

/* ==================== determinisation (construction des sous-ensembles) ====*/
static void build_subset_name(BitSet s, int n, char *out, int outsize) {
    int idx = 0; out[idx++] = '{';
    int first = 1;
    for (int q = 0; q < n && idx < outsize - 3; q++) if ((s >> q) & 1ULL) {
        if (!first) out[idx++] = ',';
        idx += snprintf(out + idx, (size_t)(outsize - idx), "%d", q);
        first = 0;
    }
    if (idx < outsize - 1) out[idx++] = '}';
    out[idx] = '\0';
}
Automaton aut_determinize(const Automaton *a) {
    BitSet subsets[MAX_STATES];
    int transTo[MAX_STATES][MAX_SYMB];
    int nSub = 0;
    BitSet initSet = 0;
    for (int q = 0; q < a->nStates; q++) if (a->isInitial[q]) initSet |= (1ULL << q);
    initSet = eps_closure_set(a, initSet);
    subsets[nSub++] = initSet;
    for (int i = 0; i < nSub; i++) {
        BitSet cur = subsets[i];
        for (int sy = 0; sy < a->nSymb; sy++) {
            BitSet nxt = eps_closure_set(a, move_set(a, cur, sy));
            int idx = -1;
            for (int k = 0; k < nSub; k++) if (subsets[k] == nxt) { idx = k; break; }
            if (idx < 0) {
                if (nSub < MAX_STATES) { subsets[nSub] = nxt; idx = nSub; nSub++; }
                else { fprintf(stderr, "[determinisation] limite MAX_STATES atteinte\n"); idx = 0; }
            }
            transTo[i][sy] = idx;
        }
    }
    Automaton d;
    aut_init(&d, nSub, a->alphabet, 0);
    d.isInitial[0] = 1;
    for (int i = 0; i < nSub; i++) {
        for (int q = 0; q < a->nStates; q++) if (((subsets[i] >> q) & 1ULL) && a->isFinal[q]) { d.isFinal[i] = 1; break; }
        build_subset_name(subsets[i], a->nStates, d.name[i], NAME_LEN);
        for (int sy = 0; sy < a->nSymb; sy++) d.delta[i][sy][transTo[i][sy]] = 1;
    }
    printf("\n[determinisation] %d etat(s) de l'AFD genere(s) a partir des sous-ensembles :\n", nSub);
    for (int i = 0; i < nSub; i++) printf("   q%d = %s%s\n", i, d.name[i], d.isFinal[i] ? "  (final)" : "");
    return d;
}

/* ==================== eps-AFN -> AFN (theoreme 3) =============================*/
Automaton aut_epsnfa_to_nfa(const Automaton *a) {
    Automaton r; aut_init(&r, a->nStates, a->alphabet, 0);
    for (int q = 0; q < a->nStates; q++) {
        r.isInitial[q] = a->isInitial[q];
        strncpy(r.name[q], a->name[q], NAME_LEN - 1);
        BitSet cl = eps_closure_state(a, q);
        for (int p = 0; p < a->nStates; p++) if ((cl >> p) & 1ULL) {
            if (a->isFinal[p]) r.isFinal[q] = 1;
            for (int sy = 0; sy < a->nSymb; sy++)
                for (int q2 = 0; q2 < a->nStates; q2++) if (a->delta[p][sy][q2]) r.delta[q][sy][q2] = 1;
        }
    }
    return r;
}
Automaton aut_to_epsnfa(const Automaton *a)   { Automaton r = *a; r.hasEpsilon = 1; return r; }
Automaton aut_dfa_to_nfa(const Automaton *a)  { Automaton r = *a; return r; }
Automaton aut_dfa_to_epsnfa(const Automaton *a) { Automaton r = *a; r.hasEpsilon = 1; return r; }

/* ==================== miroir =============================*/
Automaton aut_mirror(const Automaton *a) {
    Automaton m; aut_init(&m, a->nStates, a->alphabet, a->hasEpsilon);
    for (int q = 0; q < a->nStates; q++) {
        m.isInitial[q] = a->isFinal[q];
        m.isFinal[q] = a->isInitial[q];
        strncpy(m.name[q], a->name[q], NAME_LEN - 1);
    }
    int maxSym = a->hasEpsilon ? a->nSymb + 1 : a->nSymb;
    for (int sy = 0; sy < maxSym; sy++) { int si = (sy == a->nSymb) ? EPS_IDX : sy;
        for (int q = 0; q < a->nStates; q++)
            for (int q2 = 0; q2 < a->nStates; q2++) if (a->delta[q][si][q2]) m.delta[q2][si][q] = 1;
    }
    return m;
}

/* ==================== reconnaissance / enumeration =============================*/
int aut_accepts(const Automaton *a, const char *word) {
    BitSet cur = 0;
    for (int q = 0; q < a->nStates; q++) if (a->isInitial[q]) cur |= (1ULL << q);
    cur = eps_closure_set(a, cur);
    for (const char *p = word; *p; p++) {
        int sy = aut_symidx(a, *p);
        if (sy < 0) { fprintf(stderr, "[reconnaissance] symbole '%c' hors alphabet\n", *p); return 0; }
        cur = eps_closure_set(a, move_set(a, cur, sy));
        if (cur == 0) return 0;
    }
    for (int q = 0; q < a->nStates; q++) if (((cur >> q) & 1ULL) && a->isFinal[q]) return 1;
    return 0;
}
void aut_enumerate(const Automaton *a, int maxLen) {
    int count = 0;
    printf("Mots du langage L(A) de longueur <= %d :\n", maxLen);
    if (aut_accepts(a, "")) { printf("   epsilon (mot vide)\n"); count++; }
    char word[40];
    for (int len = 1; len <= maxLen; len++) {
        long total = 1; for (int i = 0; i < len; i++) total *= a->nSymb;
        for (long code = 0; code < total; code++) {
            long c = code;
            for (int i = 0; i < len; i++) { word[i] = a->alphabet[c % a->nSymb]; c /= a->nSymb; }
            word[len] = '\0';
            if (aut_accepts(a, word)) { printf("   %s\n", word); count++; }
        }
    }
    printf("--> %d mot(s) trouve(s).\n", count);
}
int aut_is_empty(const Automaton *a) {
    BitSet acc = accessible_states(a);
    for (int q = 0; q < a->nStates; q++) if (((acc >> q) & 1ULL) && a->isFinal[q]) return 0;
    return 1;
}

/* ==================== minimisation (Moore) & automate canonique =============*/
Automaton aut_minimize(const Automaton *a) {
    Automaton d0 = aut_determinize(a);
    Automaton d = aut_complete(&d0);
    int n = d.nStates;
    int part[MAX_STATES];
    for (int q = 0; q < n; q++) part[q] = d.isFinal[q] ? 1 : 0;
    printf("\n[minimisation] partition initiale Pi0 : finaux={");
    { int f = 1; for (int q = 0; q < n; q++) if (part[q]) { printf("%s%d", f ? "" : ",", q); f = 0; } }
    printf("}  non-finaux={");
    { int f = 1; for (int q = 0; q < n; q++) if (!part[q]) { printf("%s%d", f ? "" : ",", q); f = 0; } }
    printf("}\n");
    int changed = 1, iter = 0;
    while (changed) {
        changed = 0; iter++;
        int sig[MAX_STATES][MAX_SYMB + 1];
        for (int q = 0; q < n; q++) {
            sig[q][0] = part[q];
            for (int sy = 0; sy < d.nSymb; sy++) {
                int dest = -1;
                for (int q2 = 0; q2 < n; q2++) if (d.delta[q][sy][q2]) dest = q2;
                sig[q][sy + 1] = (dest >= 0) ? part[dest] : -1;
            }
        }
        int assigned[MAX_STATES]; for (int i = 0; i < n; i++) assigned[i] = -1;
        int nextClass = 0;
        for (int q = 0; q < n; q++) {
            if (assigned[q] >= 0) continue;
            assigned[q] = nextClass;
            for (int q2 = q + 1; q2 < n; q2++) {
                if (assigned[q2] >= 0) continue;
                int same = 1;
                for (int k = 0; k <= d.nSymb; k++) if (sig[q][k] != sig[q2][k]) { same = 0; break; }
                if (same) assigned[q2] = nextClass;
            }
            nextClass++;
        }
        for (int q = 0; q < n; q++) if (assigned[q] != part[q]) changed = 1;
        for (int q = 0; q < n; q++) part[q] = assigned[q];
        printf("[minimisation] Pi%d : %d classe(s)\n", iter, nextClass);
    }
    int nClasses = 0;
    for (int q = 0; q < n; q++) if (part[q] + 1 > nClasses) nClasses = part[q] + 1;
    Automaton m; aut_init(&m, nClasses, d.alphabet, 0);
    int repr[MAX_STATES]; for (int i = 0; i < nClasses; i++) repr[i] = -1;
    for (int q = 0; q < n; q++) if (repr[part[q]] < 0) repr[part[q]] = q;
    for (int cIdx = 0; cIdx < nClasses; cIdx++) {
        int q = repr[cIdx];
        m.isFinal[cIdx] = d.isFinal[q];
        if (d.isInitial[q]) m.isInitial[cIdx] = 1;
        for (int sy = 0; sy < d.nSymb; sy++) {
            int dest = -1;
            for (int q2 = 0; q2 < n; q2++) if (d.delta[q][sy][q2]) dest = q2;
            if (dest >= 0) m.delta[cIdx][sy][part[dest]] = 1;
        }
    }
    return m;
}
Automaton aut_canonical(const Automaton *a) {
    Automaton t = aut_trim(a);
    return aut_minimize(&t);
}

/* ==================== operations de cloture =============================*/
static void unify_alphabet(const Automaton *a1, const Automaton *a2, char *out, int *outn) {
    int n = 0;
    for (int i = 0; i < a1->nSymb; i++) out[n++] = a1->alphabet[i];
    for (int i = 0; i < a2->nSymb; i++) {
        int found = 0;
        for (int j = 0; j < n; j++) if (out[j] == a2->alphabet[i]) found = 1;
        if (!found) out[n++] = a2->alphabet[i];
    }
    out[n] = '\0';
    *outn = n;
}
static Automaton reindex_alphabet(const Automaton *a, const char *alpha, int n) {
    Automaton r; aut_init(&r, a->nStates, alpha, a->hasEpsilon);
    for (int q = 0; q < a->nStates; q++) {
        r.isInitial[q] = a->isInitial[q]; r.isFinal[q] = a->isFinal[q];
        strncpy(r.name[q], a->name[q], NAME_LEN - 1);
    }
    for (int i = 0; i < n; i++) {
        int oldIdx = aut_symidx(a, alpha[i]);
        if (oldIdx < 0) continue;
        for (int q = 0; q < a->nStates; q++)
            for (int q2 = 0; q2 < a->nStates; q2++) if (a->delta[q][oldIdx][q2]) r.delta[q][i][q2] = 1;
    }
    if (a->hasEpsilon)
        for (int q = 0; q < a->nStates; q++)
            for (int q2 = 0; q2 < a->nStates; q2++) if (a->delta[q][EPS_IDX][q2]) r.delta[q][EPS_IDX][q2] = 1;
    return r;
}
static Automaton make_product(const Automaton *a1, const Automaton *a2, int mode) {
    char alpha[MAX_SYMB + 1]; int n;
    unify_alphabet(a1, a2, alpha, &n);
    Automaton u1 = reindex_alphabet(a1, alpha, n), u2 = reindex_alphabet(a2, alpha, n);
    Automaton d1t = aut_determinize(&u1), d1 = aut_complete(&d1t);
    Automaton d2t = aut_determinize(&u2), d2 = aut_complete(&d2t);
    int n1 = d1.nStates, n2 = d2.nStates;
    int total = n1 * n2; if (total > MAX_STATES) { fprintf(stderr, "[produit] %d etats requis > MAX_STATES, resultat tronque\n", total); total = MAX_STATES; }
    Automaton p; aut_init(&p, total, alpha, 0);
    for (int i = 0; i < n1; i++) for (int j = 0; j < n2; j++) {
        int id = i * n2 + j; if (id >= MAX_STATES) continue;
        if (i == 0 && j == 0) p.isInitial[id] = 1;
        int f1 = d1.isFinal[i], f2 = d2.isFinal[j], isf = 0;
        if (mode == 0) isf = f1 || f2; else if (mode == 1) isf = f1 && f2; else isf = f1 && !f2;
        p.isFinal[id] = isf;
        snprintf(p.name[id], NAME_LEN, "(%d,%d)", i, j);
        for (int sy = 0; sy < n; sy++) {
            int di = -1, dj = -1;
            for (int k = 0; k < n1; k++) if (d1.delta[i][sy][k]) di = k;
            for (int k = 0; k < n2; k++) if (d2.delta[j][sy][k]) dj = k;
            if (di >= 0 && dj >= 0) { int tid = di * n2 + dj; if (tid < MAX_STATES) p.delta[id][sy][tid] = 1; }
        }
    }
    return p;
}
Automaton aut_union_product(const Automaton *a1, const Automaton *a2) { return make_product(a1, a2, 0); }
Automaton aut_intersection(const Automaton *a1, const Automaton *a2)  { return make_product(a1, a2, 1); }
Automaton aut_difference(const Automaton *a1, const Automaton *a2)   { return make_product(a1, a2, 2); }

Automaton aut_complement(const Automaton *a) {
    Automaton d0 = aut_determinize(a);
    Automaton d = aut_complete(&d0);
    for (int q = 0; q < d.nStates; q++) d.isFinal[q] = !d.isFinal[q];
    return d;
}

Automaton aut_union_parallel(const Automaton *a1, const Automaton *a2) {
    char alpha[MAX_SYMB + 1]; int n; unify_alphabet(a1, a2, alpha, &n);
    Automaton u1 = reindex_alphabet(a1, alpha, n), u2 = reindex_alphabet(a2, alpha, n);
    int total = 1 + u1.nStates + u2.nStates; if (total > MAX_STATES) total = MAX_STATES;
    Automaton r; aut_init(&r, total, alpha, 1);
    r.isInitial[0] = 1;
    int off1 = 1, off2 = 1 + u1.nStates;
    for (int q = 0; q < u1.nStates; q++) if (off1 + q < MAX_STATES) {
        r.isFinal[off1 + q] = u1.isFinal[q];
        if (u1.isInitial[q]) r.delta[0][EPS_IDX][off1 + q] = 1;
    }
    for (int q = 0; q < u2.nStates; q++) if (off2 + q < MAX_STATES) {
        r.isFinal[off2 + q] = u2.isFinal[q];
        if (u2.isInitial[q]) r.delta[0][EPS_IDX][off2 + q] = 1;
    }
    int m1 = u1.hasEpsilon ? u1.nSymb + 1 : u1.nSymb;
    for (int sy = 0; sy < m1; sy++) { int si = (sy == u1.nSymb) ? EPS_IDX : sy;
        for (int q = 0; q < u1.nStates; q++) for (int q2 = 0; q2 < u1.nStates; q2++)
            if (u1.delta[q][si][q2] && off1 + q < MAX_STATES && off1 + q2 < MAX_STATES) r.delta[off1 + q][si][off1 + q2] = 1;
    }
    int m2 = u2.hasEpsilon ? u2.nSymb + 1 : u2.nSymb;
    for (int sy = 0; sy < m2; sy++) { int si = (sy == u2.nSymb) ? EPS_IDX : sy;
        for (int q = 0; q < u2.nStates; q++) for (int q2 = 0; q2 < u2.nStates; q2++)
            if (u2.delta[q][si][q2] && off2 + q < MAX_STATES && off2 + q2 < MAX_STATES) r.delta[off2 + q][si][off2 + q2] = 1;
    }
    return r;
}

Automaton aut_concat(const Automaton *a1, const Automaton *a2) {
    char alpha[MAX_SYMB + 1]; int n; unify_alphabet(a1, a2, alpha, &n);
    Automaton u1 = reindex_alphabet(a1, alpha, n), u2 = reindex_alphabet(a2, alpha, n);
    int total = u1.nStates + u2.nStates; if (total > MAX_STATES) total = MAX_STATES;
    Automaton r; aut_init(&r, total, alpha, 1);
    int off2 = u1.nStates;
    for (int q = 0; q < u1.nStates; q++) r.isInitial[q] = u1.isInitial[q];
    for (int q = 0; q < u2.nStates; q++) if (off2 + q < MAX_STATES) r.isFinal[off2 + q] = u2.isFinal[q];
    int m1 = u1.hasEpsilon ? u1.nSymb + 1 : u1.nSymb;
    for (int sy = 0; sy < m1; sy++) { int si = (sy == u1.nSymb) ? EPS_IDX : sy;
        for (int q = 0; q < u1.nStates; q++) for (int q2 = 0; q2 < u1.nStates; q2++) if (u1.delta[q][si][q2]) r.delta[q][si][q2] = 1;
    }
    int m2 = u2.hasEpsilon ? u2.nSymb + 1 : u2.nSymb;
    for (int sy = 0; sy < m2; sy++) { int si = (sy == u2.nSymb) ? EPS_IDX : sy;
        for (int q = 0; q < u2.nStates; q++) for (int q2 = 0; q2 < u2.nStates; q2++)
            if (u2.delta[q][si][q2] && off2 + q < MAX_STATES && off2 + q2 < MAX_STATES) r.delta[off2 + q][si][off2 + q2] = 1;
    }
    for (int q = 0; q < u1.nStates; q++) if (u1.isFinal[q])
        for (int q2 = 0; q2 < u2.nStates; q2++) if (u2.isInitial[q2] && off2 + q2 < MAX_STATES) r.delta[q][EPS_IDX][off2 + q2] = 1;
    return r;
}

Automaton aut_star(const Automaton *a) {
    int total = a->nStates + 2; if (total > MAX_STATES) total = MAX_STATES;
    Automaton r; aut_init(&r, total, a->alphabet, 1);
    int newInit = a->nStates, newFinal = a->nStates + 1;
    if (newFinal >= MAX_STATES) { newFinal = MAX_STATES - 1; newInit = MAX_STATES - 2; }
    r.isInitial[newInit] = 1; r.isFinal[newFinal] = 1;
    r.delta[newInit][EPS_IDX][newFinal] = 1; /* accepte le mot vide */
    int maxSym = a->hasEpsilon ? a->nSymb + 1 : a->nSymb;
    for (int sy = 0; sy < maxSym; sy++) { int si = (sy == a->nSymb) ? EPS_IDX : sy;
        for (int q = 0; q < a->nStates; q++) for (int q2 = 0; q2 < a->nStates; q2++) if (a->delta[q][si][q2]) r.delta[q][si][q2] = 1;
    }
    for (int q = 0; q < a->nStates; q++) if (a->isInitial[q]) r.delta[newInit][EPS_IDX][q] = 1;
    for (int q = 0; q < a->nStates; q++) if (a->isFinal[q]) { r.delta[q][EPS_IDX][newFinal] = 1; r.delta[q][EPS_IDX][newInit] = 1; }
    return r;
}

int aut_equivalent(const Automaton *a1, const Automaton *a2) {
    Automaton d1 = aut_difference(a1, a2);
    Automaton d2 = aut_difference(a2, a1);
    return aut_is_empty(&d1) && aut_is_empty(&d2);
}
