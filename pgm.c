/* ============================================================================
 * pgm.c -- Moteur graphique minimal (PGM P5) "from scratch"
 * ==========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pgm.h"

/* ---------------------------------------------------------------------- */
Canvas canvas_create(int w, int h, unsigned char bg) {
    Canvas c;
    c.w = w; c.h = h;
    c.data = (unsigned char*)malloc((size_t)w * (size_t)h);
    memset(c.data, bg, (size_t)w * (size_t)h);
    return c;
}
void canvas_free(Canvas *c) { if (c->data) free(c->data); c->data = NULL; }

void canvas_set(Canvas *c, int x, int y, unsigned char v) {
    if (x < 0 || y < 0 || x >= c->w || y >= c->h) return;
    c->data[y * c->w + x] = v;
}
unsigned char canvas_get(const Canvas *c, int x, int y) {
    if (x < 0 || y < 0 || x >= c->w || y >= c->h) return 255;
    return c->data[y * c->w + x];
}

/* Segment épais : disque de rayon th autour de chaque pixel de la ligne */
static void plot_thick(Canvas *c, int x, int y, unsigned char v, int th) {
    for (int dy = -th; dy <= th; dy++)
        for (int dx = -th; dx <= th; dx++)
            if (dx * dx + dy * dy <= th * th) canvas_set(c, x + dx, y + dy, v);
}

/* Algorithme de Bresenham */
void canvas_line(Canvas *c, int x0, int y0, int x1, int y1, unsigned char v) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        canvas_set(c, x0, y0, v);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
void canvas_thick_line(Canvas *c, int x0, int y0, int x1, int y1, unsigned char v, int th) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        plot_thick(c, x0, y0, v, th);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* Algorithme du cercle médian (Bresenham cercle) */
void canvas_circle(Canvas *c, int cx, int cy, int r, unsigned char v) {
    int x = r, y = 0, err = 0;
    while (x >= y) {
        canvas_set(c, cx + x, cy + y, v); canvas_set(c, cx + y, cy + x, v);
        canvas_set(c, cx - y, cy + x, v); canvas_set(c, cx - x, cy + y, v);
        canvas_set(c, cx - x, cy - y, v); canvas_set(c, cx - y, cy - x, v);
        canvas_set(c, cx + y, cy - x, v); canvas_set(c, cx + x, cy - y, v);
        y++;
        if (err <= 0) err += 2 * y + 1;
        if (err > 0)  { x--; err -= 2 * x + 1; }
    }
}
void canvas_circle_fill(Canvas *c, int cx, int cy, int r, unsigned char v) {
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
            if (x * x + y * y <= r * r) canvas_set(c, cx + x, cy + y, v);
}

void canvas_arrowhead(Canvas *c, int fromx, int fromy, int tox, int toy, unsigned char v) {
    double ang = atan2(toy - fromy, tox - fromx);
    double a1 = ang + 2.6, a2 = ang - 2.6;
    int L = 9;
    int x1 = tox + (int)(L * cos(a1)), y1 = toy + (int)(L * sin(a1));
    int x2 = tox + (int)(L * cos(a2)), y2 = toy + (int)(L * sin(a2));
    canvas_thick_line(c, tox, toy, x1, y1, v, 1);
    canvas_thick_line(c, tox, toy, x2, y2, v, 1);
}

/* Arc courbe entre deux points (quadratique), avec flèche au bout.
   bend = amplitude perpendiculaire de la courbure (>0 vers un côté). */
void canvas_curved_arc(Canvas *c, int x0, int y0, int x1, int y1, double bend, unsigned char v, int arrow) {
    double mx = (x0 + x1) / 2.0, my = (y0 + y1) / 2.0;
    double dx = x1 - x0, dy = y1 - y0;
    double len = sqrt(dx * dx + dy * dy); if (len < 1e-6) len = 1;
    double nx = -dy / len, ny = dx / len;
    double cxp = mx + nx * bend, cyp = my + ny * bend;
    int steps = 60;
    int px = x0, py = y0;
    for (int i = 1; i <= steps; i++) {
        double t = (double)i / steps;
        double xt = (1 - t) * (1 - t) * x0 + 2 * (1 - t) * t * cxp + t * t * x1;
        double yt = (1 - t) * (1 - t) * y0 + 2 * (1 - t) * t * cyp + t * t * y1;
        canvas_thick_line(c, px, py, (int)xt, (int)yt, v, 1);
        px = (int)xt; py = (int)yt;
    }
    if (arrow) canvas_arrowhead(c, (int)cxp, (int)cyp, x1, y1, v);
}

void canvas_self_loop(Canvas *c, int cx, int cy, int r, unsigned char v) {
    int lcx = cx, lcy = cy - r - 16;
    canvas_circle(c, lcx, lcy, 14, v);
    canvas_arrowhead(c, lcx - 8, lcy + 10, lcx - 12, lcy + 13, v);
}

/* ---------------------------------------------------------------------- *
 * Police bitmap 5x7 (chaque caractère = 7 lignes de 5 bits, '#'=allume)   *
 * ---------------------------------------------------------------------- */
typedef struct { char ch; const char *rows[7]; } Glyph;

static const Glyph FONT[] = {
{'0',{".###","#...#","#..##","#.#.#","##..#","#...#",".###"}},
{'1',{"..#.",".##.","..#.","..#.","..#.","..#.",".###"}},
{'2',{".###","#...#","....#","...#.","..#..",".#...","#####"}},
{'3',{"####","....#","...#.",".###","....#","#...#",".###"}},
{'4',{"...#","..##",".#.#","#..#","#####","...#","...#"}},
{'5',{"#####","#....","####.","....#","....#","#...#",".###"}},
{'6',{"..##",".#..","#....","####.","#...#","#...#",".###"}},
{'7',{"#####","....#","...#","..#.",".#..",".#..",".#.."}},
{'8',{".###","#...#","#...#",".###","#...#","#...#",".###"}},
{'9',{".###","#...#","#...#",".####","....#","...#.","###.."}},
{'a',{".....",".###","....#",".####","#...#","#..##",".##.#"}},
{'b',{"#....","#....","#.##.","##..#","#...#","#...#","##.##"}},
{'c',{".....",".....",".###","#....","#....","#....",".###"}},
{'d',{"....#","....#",".####","#...#","#...#","#...#",".####"}},
{'e',{".....",".###","#...#","#####","#....","#...#",".###"}},
{'f',{"..##",".#..","####","..#.","..#.","..#.","..#."}},
{'g',{".....",".####","#...#","#...#",".####","....#",".###"}},
{'h',{"#....","#....","#.##.","##..#","#...#","#...#","#...#"}},
{'i',{"..#.","....","..##","..#.","..#.","..#.",".###"}},
{'j',{"...#","....","..##","...#","...#","#..#",".##."}},
{'k',{"#....","#....","#..#.","#.#..","##...","#.#..","#..#."}},
{'l',{".##.","..#.","..#.","..#.","..#.","..#.",".###"}},
{'m',{".....","....." ,"##.#.","#.#.#","#.#.#","#...#","#...#"}},
{'n',{".....",".....","#.##.","##..#","#...#","#...#","#...#"}},
{'o',{".....",".....",".###","#...#","#...#","#...#",".###"}},
{'p',{".....","....." ,"####.","#...#","####.","#....","#...."}},
{'q',{".....","....." ,".####","#...#",".####","....#","....#"}},
{'r',{".....",".....","#.##","##..#","#....","#....","#...."}},
{'s',{".....",".....",".####","##...",".###.","...##","####."}},
{'t',{"..#.",".###","..#.","..#.","..#.","..#.","...#"}},
{'u',{".....",".....","#...#","#...#","#...#","#..##",".##.#"}},
{'v',{".....",".....","#...#","#...#","#...#",".#.#.","..#.."}},
{'w',{".....",".....","#...#","#...#","#.#.#","#.#.#",".#.#."}},
{'x',{".....",".....","#...#",".#.#.","..#..",".#.#.","#...#"}},
{'y',{".....",".....","#...#","#...#",".####","....#",".###."}},
{'z',{".....",".....","#####","...#.","..#..",".#...","#####"}},
{'{',{"..##",".#..","#...",".#..","..##","....","...."}},
{'}',{"##..","..#.","...#","..#.","##..","....","...."}},
{',',{"....","....","....","....","..#.",".#..","...."}},
{'-',{"....","....","####","....","....","....","...."}},
{'>',{"#...",".#..","..#.","...#","..#.",".#..","#..."}},
{'_',{"....","....","....","....","....","....","#####"}},
{'.',{"....","....","....","....","....","..#.","..#."}},
{' ',{".....",".....",".....",".....",".....",".....","....."}},
};
#define NFONT (int)(sizeof(FONT)/sizeof(FONT[0]))

void canvas_char(Canvas *c, int x, int y, int scale, char ch, unsigned char v) {
    const Glyph *g = NULL;
    char lc = ch;
    if (lc >= 'A' && lc <= 'Z') lc = (char)(lc - 'A' + 'a');
    for (int i = 0; i < NFONT; i++) if (FONT[i].ch == lc) { g = &FONT[i]; break; }
    if (!g) return;
    for (int row = 0; row < 7; row++) {
        const char *r = g->rows[row];
        for (int col = 0; r[col]; col++) {
            if (r[col] == '#') {
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        canvas_set(c, x + col * scale + sx, y + row * scale + sy, v);
            }
        }
    }
}

int canvas_text_width(const char *s, int scale) {
    return (int)strlen(s) * 6 * scale;
}

void canvas_text(Canvas *c, int x, int y, int scale, const char *s, unsigned char v) {
    int cx = x;
    for (const char *p = s; *p; p++) {
        canvas_char(c, cx, y, scale, *p, v);
        cx += 6 * scale;
    }
}

int canvas_save_pgm(const Canvas *c, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fprintf(f, "P5\n%d %d\n255\n", c->w, c->h);
    fwrite(c->data, 1, (size_t)c->w * (size_t)c->h, f);
    fclose(f);
    return 1;
}
