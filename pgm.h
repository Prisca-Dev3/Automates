/* ============================================================================
 * pgm.h -- Moteur graphique minimal (PGM P5) "from scratch"
 * INF3421 - Langage Formel & Compilation - UY1
 *
 * Fournit les primitives de dessin (ligne, cercle, texte bitmap) permettant
 * de matérialiser visuellement automates, transitions et diagrammes sous
 * forme d'images PGM (Portable GrayMap) sans aucune bibliothèque externe.
 * ==========================================================================*/
#ifndef PGM_H
#define PGM_H

typedef struct {
    int w, h;
    unsigned char *data; /* 0 = noir, 255 = blanc, niveaux de gris */
} Canvas;

Canvas canvas_create(int w, int h, unsigned char bg);
void   canvas_free(Canvas *c);
void   canvas_set(Canvas *c, int x, int y, unsigned char v);
unsigned char canvas_get(const Canvas *c, int x, int y);

void canvas_line(Canvas *c, int x0, int y0, int x1, int y1, unsigned char v);
void canvas_thick_line(Canvas *c, int x0, int y0, int x1, int y1, unsigned char v, int th);
void canvas_circle(Canvas *c, int cx, int cy, int r, unsigned char v);
void canvas_circle_fill(Canvas *c, int cx, int cy, int r, unsigned char v);
void canvas_arrowhead(Canvas *c, int fromx, int fromy, int tox, int toy, unsigned char v);
void canvas_curved_arc(Canvas *c, int x0, int y0, int x1, int y1, double bend, unsigned char v, int arrow);
void canvas_self_loop(Canvas *c, int cx, int cy, int r, unsigned char v);

/* Police bitmap 5x7 : chiffres, lettres minuscules/majuscules et symboles usuels */
void canvas_char(Canvas *c, int x, int y, int scale, char ch, unsigned char v);
void canvas_text(Canvas *c, int x, int y, int scale, const char *s, unsigned char v);
int  canvas_text_width(const char *s, int scale);

int canvas_save_pgm(const Canvas *c, const char *path);

#endif
