/* ============================================================================
 * equation.h -- Systemes d'equations lineaires en langages (Lemme d'Arden)
 *               + constructions Thompson / Glushkov (utilisent regex + automaton)
 * INF3421 - UY1
 * ==========================================================================*/
#ifndef EQUATION_H
#define EQUATION_H

#include "regex.h"
#include "automaton.h"

#define MAX_EQ 60

typedef struct {
    int n;                       /* nombre d'inconnues X0..Xn-1 */
    RNode *A[MAX_EQ][MAX_EQ];    /* coefficient de Xj dans l'equation de Xi (NULL = absent) */
    RNode *B[MAX_EQ];            /* terme constant de l'equation de Xi (NULL = vide) */
    char  names[MAX_EQ][8];      /* nom d'affichage de chaque inconnue */
} EqSystem;

void   eqsys_init(EqSystem *s, int n);
void   eqsys_print(const EqSystem *s, const char *title);
/* resout le systeme par la methode de Gauss (resolution partielle + substitution),
   affiche les etapes intermediaires si verbose=1, renvoie les solutions X0..Xn-1 */
RNode **eqsys_solve(EqSystem *s, int verbose);

/* Constructions vues au chapitre 3 */
Automaton thompson_build(const RNode *r);      /* construction de Thompson "pure" */
Automaton glushkov_build(const RNode *r);      /* automate de Glushkov (des positions) */

#endif
