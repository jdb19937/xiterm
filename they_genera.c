/*
 * they_genera.c — generat fontem corporeum "They"
 *
 * Atlas 512x768 pixelorum, 16x16 glyphi, singulus glyphus 32x48.
 * Glyphi in reticulo 16x24 pixelorum definiuntur, deinde 2x amplificantur.
 * Corpora ambulatione fortuita in reticulo 5x7 generantur.
 * Semen = ord(lc(chr(c))), ergo maiusculae et minusculae eandem
 * formam basi habent, sed maiusculae pileum accipiunt.
 */

#include <phantasma/phantasma.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ================================================================
 * constantiae
 * ================================================================ */

#define ATLAS_LAT    512
#define ATLAS_ALT    768
#define GLYPH_LAT    32
#define GLYPH_ALT    48
#define RAW_LAT      16
#define RAW_ALT      24
#define SCALA        (GLYPH_LAT / RAW_LAT)    /* 2 */
#define NUM_GLYPHI   256

#define GRID_LAT     5
#define GRID_ALT     7

#define COL_CORPUS   0xFF555555u
#define COL_VACUUS   0xFF000000u


/* ================================================================
 * generatio fortuita — srand48/drand48 ut Perl
 * ================================================================ */

static int
rand_int(int n)
{
    return (int)(drand48() * n);
}


/* ================================================================
 * miscere — mergesort cum comparatione fortuita, ut Perl
 *
 * Perl: sort { rand(1) <=> 0.5 } @array
 * Mergesort ascendens (bottom-up) imitatur.
 * ================================================================ */

static void
merge_fortuita(int *arr, int *tmp, int lo, int mid, int hi)
{
    int i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        if (drand48() <= 0.5)
            tmp[k++] = arr[i++];
        else
            tmp[k++] = arr[j++];
    }
    while (i < mid) tmp[k++] = arr[i++];
    while (j < hi)
        tmp[k++] = arr[j++];
    memcpy(arr + lo, tmp + lo, (size_t)(hi - lo) * sizeof(int));
}

static void
miscere(int *arr, int n)
{
    int tmp[35];
    int width, lo, mid, hi;

    for (width = 1; width < n; width *= 2) {
        for (lo = 0; lo < n; lo += 2 * width) {
            mid = lo + width;
            hi  = lo + 2 * width;
            if (mid > n)
                mid = n;
            if (hi > n)
                hi = n;
            if (mid < hi)
                merge_fortuita(arr, tmp, lo, mid, hi);
        }
    }
}


/* ================================================================
 * geton — cellulam accensam fortuitam eligit
 * ================================================================ */

static int
geton(int *map)
{
    int on[35], n = 0;
    int i;

    for (i = 0; i < GRID_LAT * GRID_ALT; i++) {
        if (map[i])
            on[n++] = i;
    }
    if (n == 0)
        return 0;

    miscere(on, n);
    return on[0];
}


/* ================================================================
 * corpus_genera — corpus pro charactere generat
 * ================================================================ */

static void
corpus_genera(int c, int *map)
{
    int d, iter, cell, x, y, q;

    /* semen: minuscula characteris */
    d = c;
    if (d >= 'A' && d <= 'Z')
        d = d + 32;
    srand48((long)d);

    memset(map, 0, GRID_LAT * GRID_ALT * sizeof(int));
    map[4 * GRID_LAT + 2] = 1;

    /* ambulatio fortuita: 20 passus */
    for (iter = 0; iter < 20; iter++) {
        cell = geton(map);
        x    = cell % GRID_LAT;
        y    = cell / GRID_LAT;

        q = rand_int(4);
        if      (q == 0)
            y++;
        else if (q == 1)
            y--;
        else if (q == 2)
            x++;
        else
            x--;

        if (x >= 4)
            x = 4;
        if (x < 0)
            x = 0;
        if (y >= 6)
            y = 6;
        if (y < 3)
            y = 3;

        map[y * GRID_LAT + x] = 1;
    }

    /* tres cellulae fortuitae removentur */
    for (iter = 0; iter < 3; iter++) {
        cell      = geton(map);
        map[cell] = 0;
    }

    /* --- emendationes pro characteribus specialibus --- */

    if (c == 'p' || c == 'P') {
        map[4*5+0] = 1;
        map[5*5+0] = 1;
        map[6*5+0] = 1;
    }
    if (c == 'i' || c == 'I') {
        map[4*5+0] = 1;
        map[5*5+0] = 1;
        map[5*5+1] = 1;
        map[5*5+2] = 1;
    }
    if (c == 'r' || c == 'R') {
        map[5*5+0] = 1;
        map[5*5+1] = 1;
        map[5*5+2] = 1;
    }
    if (c == 'l' || c == 'L') {
        map[3*5+4] = 1;
        map[3*5+3] = 1;
        map[4*5+3] = 1;
        map[5*5+3] = 1;
        map[6*5+3] = 1;
    }
    if (c == 'w' || c == 'W') {
        map[3*5+4] = 1;
        map[3*5+3] = 1;
        map[4*5+3] = 1;
        map[5*5+3] = 1;
        map[6*5+3] = 1;
    }
    if (c == 'n' || c == 'N') {
        map[5*5+4] = 1;
    }
    if (c == 'b' || c == 'B') {
        map[4*5+0] = 1;
        map[4*5+1] = 1;
        map[5*5+1] = 1;
        map[4*5+2] = 1;
        map[4*5+3] = 1;
    }
    if (c == 'o' || c == 'O') {
        map[4*5+0] = 1;
        map[4*5+1] = 1;
        map[4*5+2] = 1;
    }
    if (c == 'y' || c == 'Y') {
        map[4*5+0] = 1;
        map[4*5+1] = 1;
        map[4*5+2] = 1;
    }
    if (c == 'a' || c == 'A') {
        map[4*5+0] = 1;
        map[4*5+1] = 1;
        map[5*5+1] = 1;
        map[6*5+1] = 1;
        map[4*5+2] = 1;
    }

    /* pileus pro maiusculis et numeris */
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        int i;
        for (i = 5; i <= 9; i++)
            map[i] = 1;
        if (c >= 'A' && c <= 'Z') {
            map[11] = 1;
            map[13] = 1;
        }
    }

    /* spatium et tabula: vacuum */
    if (c == ' ' || c == '\t') {
        memset(map, 0, GRID_LAT * GRID_ALT * sizeof(int));
    }

    /* anguli */
    if (c == '<') {
        int pat[5] = {0, 0, 1, 1, 0};
        int i;
        for (i = 0; i < GRID_LAT * GRID_ALT; i++)
            map[i] = pat[i % 5];
    }
    if (c == '>') {
        int pat[5] = {0, 1, 1, 0, 0};
        int i;
        for (i = 0; i < GRID_LAT * GRID_ALT; i++)
            map[i] = pat[i % 5];
    }
}


/* ================================================================
 * atlanti_scribe — omnes glyphos amplificatos 2x in atlante ponit
 * ================================================================ */

static void
atlanti_scribe(uint32_t *atlas)
{
    int ch;

    for (ch = 0; ch < NUM_GLYPHI; ch++) {
        int map[GRID_LAT * GRID_ALT];
        int orig_x = (ch % 16) * GLYPH_LAT;
        int orig_y = (ch / 16) * GLYPH_ALT;
        int ry, rx, sy, sx;

        corpus_genera(ch, map);

        for (ry = 0; ry < RAW_ALT; ry++) {
            for (rx = 0; rx < RAW_LAT; rx++) {
                uint32_t color = COL_VACUUS;

                /* row 0: vacuus (completus)
                 * rows 1-21: contentus (15 px) + 1 px completio
                 * rows 22-23: vacuus (completus)
                 */
                if (ry >= 1 && ry <= 21 && rx < 15) {
                    int cy = ry - 1;         /* 0..20 */
                    int mx = rx / 3;         /* 0..4  */
                    int my = cy / 3;         /* 0..6  */
                    if (map[my * GRID_LAT + mx])
                        color = COL_CORPUS;
                }

                for (sy = 0; sy < SCALA; sy++) {
                    for (sx = 0; sx < SCALA; sx++) {
                        int px = orig_x + rx * SCALA + sx;
                        int py = orig_y + ry * SCALA + sy;
                        atlas[py * ATLAS_LAT + px] = color;
                    }
                }
            }
        }
    }
}


/* ================================================================
 * main
 * ================================================================ */

int
main(int argc, char **argv)
{
    const char *via_exitus = "they_16.gif";
    uint32_t *atlas;
    pfr_gif_t *gif;

    if (argc > 1)
        via_exitus = argv[1];

    atlas = calloc(ATLAS_LAT * ATLAS_ALT, sizeof(uint32_t));
    if (!atlas) {
        fprintf(stderr, "memoria deficit\n");
        return 1;
    }

    atlanti_scribe(atlas);

    gif = pfr_gif_initia(via_exitus, ATLAS_LAT, ATLAS_ALT, 0, 1);
    if (!gif) {
        fprintf(stderr, "GIF creare non potest: %s\n", via_exitus);
        free(atlas);
        return 1;
    }

    pfr_gif_modum_pone(gif, PFR_QUANT_MEDIANA, PFR_DITHER_NULLUM);
    pfr_gif_tabulam_adde(gif, atlas);
    pfr_gif_fini(gif);

    free(atlas);
    fprintf(
        stderr, "%s generatus est (%dx%d, %d glyphi)\n",
        via_exitus, ATLAS_LAT, ATLAS_ALT, NUM_GLYPHI
    );
    return 0;
}
