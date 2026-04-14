/*
 * mork_genera.c — generat fontem coloratum "Mork"
 *
 * Atlas 512x768 pixelorum, 16x16 glyphi, singulus glyphus 32x48.
 * Glyphi in reticulo 2x3 pixelorum definiuntur, deinde 16x amplificantur.
 * Singulus character sex pixelis coloratis definitur.
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
#define CELL_LAT     2
#define CELL_ALT     3
#define SCALA        (GLYPH_LAT / CELL_LAT)    /* 16 */
#define NUM_GLYPHI   256

/* colores ARGB8888 */
#define K   0xFF000000u    /* niger */
#define R   0xFFFF0000u    /* ruber */
#define G   0xFF00FF00u    /* viridis */
#define GY  0xFF7F7F7Fu    /* griseus */
#define W   0xFFFFFFFFu    /* albus */
#define DB  0xFF00007Fu    /* caeruleus obscurus */
#define B   0xFF0000FFu    /* caeruleus */
#define DG  0xFF007F00u    /* viridis obscurus */
#define Y   0xFFFFFF00u    /* flavus */
#define BN  0xFF7F7F00u    /* fuscus */
#define DR  0xFF7F0000u    /* ruber obscurus */
#define MG  0xFFFF007Fu    /* magenta */
#define PU  0xFF7F007Fu    /* purpura */
#define OR  0xFFFF7F00u    /* aurantius */
#define CY  0xFF00FFFFu    /* cyaneus */
#define DM  0xFF202020u    /* obscurus */


/* ================================================================
 * glyphi — 2x3 pixeles colorati per characterem
 *
 * Dispositio: [row0col0, row0col1, row1col0, row1col1, row2col0, row2col1]
 * ================================================================ */

static uint32_t glyphi[NUM_GLYPHI][CELL_LAT * CELL_ALT];
static int glyphi_def[NUM_GLYPHI];

#define S(ch, a, b, c, d, e, f) do {    \
    glyphi[ch][0]=(a); glyphi[ch][1]=(b); \
    glyphi[ch][2]=(c); glyphi[ch][3]=(d); \
    glyphi[ch][4]=(e); glyphi[ch][5]=(f); \
    glyphi_def[ch] = 1;                   \
} while (0)

static void
glyphos_initia(void)
{
    memset(glyphi, 0, sizeof(glyphi));
    memset(glyphi_def, 0, sizeof(glyphi_def));

    /* --- nullus character --- */
    S(0x00, DM, DM, DM, DM, DM, DM);

    /* --- numeri 0-9 --- */
    S('0', G, K, K, K, W, W);
    S('1', G, K, K, W, K, W);
    S('2', G, K, K, W, W, K);
    S('3', G, K, W, K, K, W);
    S('4', G, K, W, W, K, K);
    S('5', G, K, W, K, W, K);
    S('6', G, W, K, K, K, W);
    S('7', G, W, K, K, W, K);
    S('8', G, W, K, W, K, K);
    S('9', G, W, W, K, K, K);

    /* --- litterae minusculae a-z --- */
    S('a', K, K, R, R, DR, DR);
    S('b', K, K, B, B, B, B);
    S('c', K, K, Y, BN, Y, BN);
    S('d', K, K, DG, B, B, DG);
    S('e', K, K, G, G, DG, DG);
    S('f', K, K, Y, Y, Y, BN);
    S('g', K, K, G, DG, G, DG);
    S('h', K, K, W, GY, W, GY);
    S('i', K, K, Y, Y, BN, BN);
    S('j', K, K, Y, DG, DG, DG);
    S('k', K, K, B, DB, B, DB);
    S('l', K, K, PU, BN, PU, PU);
    S('m', K, K, MG, MG, MG, MG);
    S('n', K, K, OR, OR, BN, OR);
    S('o', K, K, W, W, GY, GY);
    S('p', K, K, Y, Y, Y, Y);
    S('q', K, K, PU, B, B, PU);
    S('r', K, K, R, R, R, DR);
    S('s', K, K, OR, R, R, OR);
    S('t', K, K, OR, Y, Y, OR);
    S('u', K, K, B, B, DB, DB);
    S('v', K, K, DG, G, G, G);
    S('w', K, K, CY, CY, CY, CY);
    S('x', K, K, R, G, G, R);
    S('y', K, K, Y, Y, DG, DG);
    S('z', K, K, PU, Y, Y, PU);

    /* --- interpunctio et signa --- */
    S('.', K, K, K, K, W, K);
    S(',', K, K, K, K, GY, K);
    S(';', K, K, W, K, GY, K);
    S(':', K, K, W, K, W, K);
    S('"', W, W, K, K, K, K);
    S('\'', GY, GY, K, K, K, K);
    S('/', K, W, W, W, W, K);
    S('\\', W, K, W, W, K, W);
    S('<', K, W, W, K, K, W);
    S('>', W, K, K, W, W, K);

    S('{', GY, B, GY, K, GY, K);
    S('[', GY, G, GY, K, GY, K);
    S('(', W, Y, W, K, W, K);
    S('`', W, K, K, K, K, K);
    S('|', Y, K, Y, K, Y, K);
    S('}', B, GY, K, GY, K, GY);
    S(']', G, GY, K, GY, K, GY);
    S(')', Y, W, K, W, K, W);

    S('^', PU, PU, K, K, K, K);
    S('~', K, K, Y, Y, K, K);
    S('=', K, K, G, G, K, K);
    S('+', K, K, Y, B, K, K);
    S('!', R, K, R, K, W, K);
    S('?', MG, MG, K, MG, W, K);
    S('@', K, K, BN, BN, K, K);
    S('#', K, K, OR, OR, K, K);
    S('*', K, K, PU, G, K, K);
    S('$', K, K, DG, DG, K, K);
    S('%', K, K, DR, DR, K, K);
    S('&', B, K, B, K, B, K);
    S('_', K, K, K, K, W, W);
    S('-', K, K, W, W, K, K);
    S(' ', K, K, K, K, K, K);

    /* --- litterae maiusculae: row 0 accipit colores row 1 --- */
    int ch;
    for (ch = 'a'; ch <= 'z'; ch++) {
        int uc = ch - 32;
        if (glyphi_def[ch] && !glyphi_def[uc]) {
            glyphi[uc][0]  = glyphi[ch][2];
            glyphi[uc][1]  = glyphi[ch][3];
            glyphi[uc][2]  = glyphi[ch][2];
            glyphi[uc][3]  = glyphi[ch][3];
            glyphi[uc][4]  = glyphi[ch][4];
            glyphi[uc][5]  = glyphi[ch][5];
            glyphi_def[uc] = 1;
        }
    }

    /* --- characteres non definiti: colores fortuiti 0-127 per componentem --- */
    for (ch = 0; ch < NUM_GLYPHI; ch++) {
        if (!glyphi_def[ch]) {
            int j;
            for (j = 0; j < 6; j++) {
                uint8_t r = (uint8_t)(rand() % 128);
                uint8_t g = (uint8_t)(rand() % 128);
                uint8_t b = (uint8_t)(rand() % 128);
                glyphi[ch][j] = 0xFF000000u | ((uint32_t)r << 16)
                    | ((uint32_t)g << 8) | (uint32_t)b;
            }
        }
    }
}

#undef S


/* ================================================================
 * functiones principales
 * ================================================================ */

/*
 * atlanti_scribe — amplificat glyphos 16x et in atlante ponit.
 */
static void
atlanti_scribe(uint32_t *atlas)
{
    int ch, cy, cx, sy, sx;

    for (ch = 0; ch < NUM_GLYPHI; ch++) {
        int orig_x = (ch % 16) * GLYPH_LAT;
        int orig_y = (ch / 16) * GLYPH_ALT;

        for (cy = 0; cy < CELL_ALT; cy++) {
            for (cx = 0; cx < CELL_LAT; cx++) {
                uint32_t color = glyphi[ch][cy * CELL_LAT + cx];
                for (sy = 0; sy < SCALA; sy++) {
                    for (sx = 0; sx < SCALA; sx++) {
                        int px = orig_x + cx * SCALA + sx;
                        int py = orig_y + cy * SCALA + sy;
                        atlas[py * ATLAS_LAT + px] = color;
                    }
                }
            }
        }
    }
}

int
main(int argc, char **argv)
{
    const char *via_exitus = "mork_16.gif";
    uint32_t *atlas;
    pfr_gif_t *gif;

    if (argc > 1)
        via_exitus = argv[1];

    glyphos_initia();

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
