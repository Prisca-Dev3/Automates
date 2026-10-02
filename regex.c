/* ============================================================================
 * regex.c -- Expressions regulieres
 * ==========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "regex.h"

/* ------------------------- constructeurs ------------------------------- */
static RNode *mk(RType t) {
    RNode *n = (RNode*)malloc(sizeof(RNode));
    n->type = t; n->symbol = 0; n->pos = -1; n->left = n->right = NULL;
    return n;
}
RNode *r_empty(void) { return mk(R_EMPTY); }
RNode *r_eps(void)   { return mk(R_EPSILON); }
RNode *r_sym(char c) { RNode *n = mk(R_SYMBOL); n->symbol = c; return n; }
RNode *r_union(RNode *a, RNode *b)  { RNode *n = mk(R_UNION);  n->left = a; n->right = b; return n; }
RNode *r_concat(RNode *a, RNode *b) { RNode *n = mk(R_CONCAT); n->left = a; n->right = b; return n; }
RNode *r_star(RNode *a)             { RNode *n = mk(R_STAR);   n->left = a; return n; }

RNode *r_copy(const RNode *r) {
    if (!r) return NULL;
    RNode *n = mk(r->type);
    n->symbol = r->symbol;
    n->left  = r_copy(r->left);
    n->right = r_copy(r->right);
    return n;
}
void regex_free(RNode *r) {
    if (!r) return;
    regex_free(r->left);
    regex_free(r->right);
    free(r);
}

/* --------------------- analyseur syntaxique descendant ------------------
 * Grammaire (priorites : * > concat > +) :
 *   expr   := term ('+' term)*
 *   term   := factor factor*
 *   factor := base '*'*
 *   base   := '(' expr ')' | '@' | '#' | [a-z0-9]
 * ------------------------------------------------------------------------*/
typedef struct { const char *s; int pos; } Parser;

static void skip_space(Parser *p) { while (p->s[p->pos] == ' ') p->pos++; }
static char peek(Parser *p) { skip_space(p); return p->s[p->pos]; }

static RNode *parse_expr(Parser *p);

static RNode *parse_base(Parser *p) {
    char c = peek(p);
    if (c == '(') {
        p->pos++;
        RNode *e = parse_expr(p);
        skip_space(p);
        if (p->s[p->pos] == ')') p->pos++;
        else { fprintf(stderr, "[regex] parenthese fermante manquante\n"); }
        return e;
    }
    if (c == '@') { p->pos++; return r_eps(); }
    if (c == '#') { p->pos++; return r_empty(); }
    if (isalnum((unsigned char)c)) { p->pos++; return r_sym(c); }
    if (c == '\0') { fprintf(stderr, "[regex] expression incomplete\n"); return r_eps(); }
    fprintf(stderr, "[regex] caractere inattendu '%c'\n", c);
    p->pos++;
    return r_eps();
}

static RNode *parse_factor(Parser *p) {
    RNode *b = parse_base(p);
    while (peek(p) == '*') { p->pos++; b = r_star(b); }
    return b;
}

static int starts_term(char c) {
    return c == '(' || c == '@' || c == '#' || isalnum((unsigned char)c);
}

static RNode *parse_term(Parser *p) {
    RNode *f = parse_factor(p);
    while (starts_term(peek(p))) {
        RNode *g = parse_factor(p);
        f = r_concat(f, g);
    }
    return f;
}

static RNode *parse_expr(Parser *p) {
    RNode *t = parse_term(p);
    while (peek(p) == '+') { p->pos++; RNode *u = parse_term(p); t = r_union(t, u); }
    return t;
}

RNode *regex_parse(const char *expr) {
    Parser p; p.s = expr; p.pos = 0;
    if (peek(&p) == '\0') return r_empty();
    return parse_expr(&p);
}

/* ------------------------------ affichage -------------------------------*/
static void to_str(const RNode *r, char *buf, int *idx, int bufsize, int parentPrio);
/* priorite : union=1, concat=2, star/base=3 */
static int prio(const RNode *r) {
    switch (r->type) {
        case R_UNION: return 1;
        case R_CONCAT: return 2;
        default: return 3;
    }
}
static void putc_buf(char *buf, int *idx, int bufsize, char c) {
    if (*idx < bufsize - 1) buf[(*idx)++] = c;
}
static void to_str(const RNode *r, char *buf, int *idx, int bufsize, int parentPrio) {
    if (!r) return;
    int needPar = prio(r) < parentPrio;
    if (needPar) putc_buf(buf, idx, bufsize, '(');
    switch (r->type) {
        case R_EMPTY:   putc_buf(buf, idx, bufsize, '#'); break;
        case R_EPSILON: putc_buf(buf, idx, bufsize, '@'); break;
        case R_SYMBOL:  putc_buf(buf, idx, bufsize, r->symbol); break;
        case R_UNION:
            to_str(r->left, buf, idx, bufsize, 1);
            putc_buf(buf, idx, bufsize, '+');
            to_str(r->right, buf, idx, bufsize, 1);
            break;
        case R_CONCAT:
            to_str(r->left, buf, idx, bufsize, 2);
            to_str(r->right, buf, idx, bufsize, 2);
            break;
        case R_STAR:
            to_str(r->left, buf, idx, bufsize, 3);
            putc_buf(buf, idx, bufsize, '*');
            break;
    }
    if (needPar) putc_buf(buf, idx, bufsize, ')');
}
void regex_to_string_buf(const RNode *r, char *buf, int bufsize) {
    int idx = 0;
    to_str(r, buf, &idx, bufsize, 0);
    buf[idx] = '\0';
}
char *regex_to_string(const RNode *r) {
    char *buf = (char*)malloc(4096);
    regex_to_string_buf(r, buf, 4096);
    return buf;
}

/* ------------------------------ proprietes ------------------------------*/
int regex_nullable(const RNode *r) {
    if (!r) return 0;
    switch (r->type) {
        case R_EMPTY:   return 0;
        case R_EPSILON: return 1;
        case R_SYMBOL:  return 0;
        case R_UNION:   return regex_nullable(r->left) || regex_nullable(r->right);
        case R_CONCAT:  return regex_nullable(r->left) && regex_nullable(r->right);
        case R_STAR:    return 1;
    }
    return 0;
}

int regex_equal_syntax(const RNode *a, const RNode *b) {
    if (!a || !b) return a == b;
    if (a->type != b->type) return 0;
    if (a->type == R_SYMBOL) return a->symbol == b->symbol;
    return regex_equal_syntax(a->left, b->left) && regex_equal_syntax(a->right, b->right);
}

/* ------------------------------ simplification ---------------------------
 * Applique recursivement les proprietes algebriques du chapitre 2 :
 *   X+# = X          #+X = X            (neutralite de la somme)
 *   X.# = #          #.X = #            (absorbant de la concatenation)
 *   X.@ = X          @.X = X            (neutralite de la concatenation)
 *   #*  = @          @*  = @
 *   X** = X*                            (idempotence de l'etoile)
 *   X+X = X                             (idempotence de la somme)
 *   (@+X)* = X*
 * --------------------------------------------------------------------------*/
RNode *regex_simplify(RNode *r) {
    if (!r) return r;
    if (r->type == R_UNION || r->type == R_CONCAT) {
        r->left  = regex_simplify(r->left);
        r->right = regex_simplify(r->right);
    } else if (r->type == R_STAR) {
        r->left = regex_simplify(r->left);
    }

    if (r->type == R_UNION) {
        if (r->left->type == R_EMPTY)  return r->right;
        if (r->right->type == R_EMPTY) return r->left;
        if (regex_equal_syntax(r->left, r->right)) return r->left;
        /* (eps + X*) -> X*  et  (X* + eps) -> X* */
        if (r->left->type == R_EPSILON && r->right->type == R_STAR) return r->right;
        if (r->right->type == R_EPSILON && r->left->type == R_STAR) return r->left;
    }
    if (r->type == R_CONCAT) {
        if (r->left->type == R_EMPTY || r->right->type == R_EMPTY) return r_empty();
        if (r->left->type == R_EPSILON)  return r->right;
        if (r->right->type == R_EPSILON) return r->left;
    }
    if (r->type == R_STAR) {
        if (r->left->type == R_EMPTY)   return r_eps();
        if (r->left->type == R_EPSILON) return r_eps();
        if (r->left->type == R_STAR)    return r->left; /* X** = X* */
    }
    return r;
}
