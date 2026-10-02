/* ============================================================================
 * regex.h -- Expressions regulieres : arbre, analyseur, simplification
 * INF3421 - UY1
 *
 * Convention d'ecriture (cf. rapport / README) :
 *   symboles      : lettres 'a'-'z' ou chiffres '0'-'9'
 *   union         : +
 *   concatenation : juxtaposition (implicite)
 *   etoile        : *
 *   epsilon       : @
 *   langage vide  : #
 *   groupement    : ( )
 * ==========================================================================*/
#ifndef REGEX_H
#define REGEX_H

typedef enum { R_EMPTY, R_EPSILON, R_SYMBOL, R_UNION, R_CONCAT, R_STAR } RType;

typedef struct RNode {
    RType type;
    char symbol;
    int  pos;              /* numero de position (utilise par l'automate de Glushkov) */
    struct RNode *left, *right;
} RNode;

/* constructeurs */
RNode *r_empty(void);
RNode *r_eps(void);
RNode *r_sym(char c);
RNode *r_union(RNode *a, RNode *b);
RNode *r_concat(RNode *a, RNode *b);
RNode *r_star(RNode *a);
RNode *r_copy(const RNode *r);

/* analyse syntaxique descendante recursive */
RNode *regex_parse(const char *expr);

/* affichage */
void  regex_to_string_buf(const RNode *r, char *buf, int bufsize);
char *regex_to_string(const RNode *r);   /* chaine allouee dynamiquement */

/* simplification algebrique (proprietes du chapitre 2) */
RNode *regex_simplify(RNode *r);

/* proprietes */
int regex_nullable(const RNode *r);      /* epsilon appartient au langage ? */
int regex_equal_syntax(const RNode *a, const RNode *b);

void regex_free(RNode *r);

#endif
