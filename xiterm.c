/*
 * xiterm.c — emulator terminalis graphicus
 *
 * Fenestram per phantasma aperit, processus conchae cum pseudo-terminali
 * forcat, codices ANSI/VT100 interpretatur, et glyphos ex mork.gif reddit.
 * Sufficiens est ad bash et vi currenda.
 * Fenestra redimensionabilis est.
 */

#include "pty.h"

#include <phantasma/phantasma.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>


/* ================================================================
 * constantiae
 * ================================================================ */

#define COL_MAX         320     /* maxima latitudo in cellis */
#define LIN_MAX         128     /* maxima altitudo in cellis */
#define COL_PRAE        96      /* columnae praefinitae */
#define LIN_PRAE        64      /* lineae praefinitae */
#define SCALA_PRAE      4       /* scala praefinita */
#define ATLAS_LAT       512     /* latitudo imaginis fontis */
#define ATLAS_ALT       768     /* altitudo imaginis fontis */
#define GLYPH_LAT       32      /* latitudo glyphi in atlante */
#define GLYPH_ALT       48      /* altitudo glyphi in atlante */
#define GLYPH_PIX_MAX   (GLYPH_LAT * GLYPH_ALT)

#define PARAM_MAX       16      /* parametra CSI maxima */
#define ALVEUS_PTY      4096    /* alveus lectionis pty */


/* ================================================================
 * typi
 * ================================================================ */

typedef struct cella {
    uint8_t ch;
    uint8_t attr;   /* bit 0: clarus, bit 1: inversus */
} cella_t;

typedef enum {
    S_NORMALIS = 0,
    S_ESC,
    S_CSI,
    S_OSC,
    S_ESC_CHARSET
} status_parser_t;

typedef struct terminalis {
    cella_t cellae[LIN_MAX][COL_MAX];
    int     col_num;            /* columnae currentis */
    int     lin_num;            /* lineae currentis */
    int     cur_lin;
    int     cur_col;
    int     reg_summa;
    int     reg_ima;
    int     salv_lin;
    int     salv_col;
    uint8_t attr;
    int     app_cursor;
    int     involutio;

    /* velum alterum */
    int     alt_activum;
    cella_t alt_cellae[LIN_MAX][COL_MAX];
    int     alt_cur_lin;
    int     alt_cur_col;

    /* parser */
    status_parser_t status;
    int     params[PARAM_MAX];
    int     n_params;
    int     privatus;
} terminalis_t;

/* ================================================================
 * status globalis
 * ================================================================ */

static uint32_t      atlas[ATLAS_LAT * ATLAS_ALT];
static uint32_t      glyphi[256][GLYPH_PIX_MAX];
static int           glyph_lat_s;   /* latitudo glyphi scalati */
static int           glyph_alt_s;   /* altitudo glyphi scalati */
static int           scala;
static terminalis_t  term;
static int           pty_mag;
static pid_t         filius;
static int           fen_lat;
static int           fen_alt;

/* ================================================================
 * fons glyphorum — onerat mork.gif
 * ================================================================ */

/*
 * glyphos_praecalcula — glyphos ex atlante 16x demittit ad scalam currentem.
 * Quadrata (16/scala)×(16/scala) pixelorum mediatur.
 */
static void glyphos_praecalcula(void)
{
    glyph_lat_s = GLYPH_LAT * scala / 16;
    glyph_alt_s = GLYPH_ALT * scala / 16;
    int blk     = 16 / scala;
    int blk2    = blk * blk;

    for (int ch = 0; ch < 256; ch++) {
        uint32_t *g = glyphi[ch];
        int base_x  = (ch % 16) * GLYPH_LAT;
        int base_y  = (ch / 16) * GLYPH_ALT;

        for (int oy = 0; oy < glyph_alt_s; oy++) {
            for (int ox = 0; ox < glyph_lat_s; ox++) {
                uint32_t sr = 0, sg = 0, sb = 0;
                int n_opac  = 0;

                for (int by = 0; by < blk; by++) {
                    for (int bx = 0; bx < blk; bx++) {
                        int ax     = base_x + ox * blk + bx;
                        int ay     = base_y + oy * blk + by;
                        uint32_t p = atlas[ay * ATLAS_LAT + ax];
                        if ((p >> 24) != 0 && (p & 0x00FFFFFF) != 0) {
                            sr += (p >> 16) & 0xFF;
                            sg += (p >> 8) & 0xFF;
                            sb += p & 0xFF;
                            n_opac++;
                        }
                    }
                }

                uint32_t color;
                if (n_opac > blk2 / 2) {
                    color = 0xFF000000
                        | ((sr / n_opac) << 16)
                        | ((sg / n_opac) << 8)
                        | (sb / n_opac);
                } else {
                    color = 0xFF000000;
                }

                g[oy * glyph_lat_s + ox] = color;
            }
        }
    }
}

static int fontem_onera(const char *via)
{
    pfr_gif_lector_t *l = pfr_gif_lege_initia(via);
    if (!l)
        return -1;

    int lat, alt;
    if (
        pfr_gif_lege_dimensiones(l, &lat, &alt) != 0 ||
        lat != ATLAS_LAT || alt != ATLAS_ALT
    ) {
        pfr_gif_lege_fini(l);
        return -1;
    }

    if (pfr_gif_lege_tabulam(l, atlas) != 0) {
        pfr_gif_lege_fini(l);
        return -1;
    }

    pfr_gif_lege_fini(l);
    glyphos_praecalcula();
    return 0;
}

/* ================================================================
 * operationes terminalis
 * ================================================================ */

static void cellam_pone(int lin, int col, cella_t c)
{
    if (lin >= 0 && lin < term.lin_num && col >= 0 && col < term.col_num)
        term.cellae[lin][col] = c;
}

static void cellam_purga(int lin, int col)
{
    cella_t c = {' ', 0};
    cellam_pone(lin, col, c);
}

static void lineam_purga(int lin, int col_ab, int col_ad)
{
    for (int c = col_ab; c <= col_ad && c < term.col_num; c++)
        cellam_purga(lin, c);
}

static void regionem_purga(int lin_ab, int col_ab, int lin_ad, int col_ad)
{
    for (int l = lin_ab; l <= lin_ad && l < term.lin_num; l++)
        lineam_purga(
            l, l == lin_ab ? col_ab : 0,
            l == lin_ad ? col_ad : term.col_num - 1
        );
}

/* ================================================================
 * volumen (scroll)
 * ================================================================ */

static void sursum_volve(void)
{
    for (int l = term.reg_summa; l < term.reg_ima; l++)
        memcpy(
            term.cellae[l], term.cellae[l + 1],
            sizeof(cella_t) * (size_t)term.col_num
        );
    lineam_purga(term.reg_ima, 0, term.col_num - 1);
}

static void deorsum_volve(void)
{
    for (int l = term.reg_ima; l > term.reg_summa; l--)
        memcpy(
            term.cellae[l], term.cellae[l - 1],
            sizeof(cella_t) * (size_t)term.col_num
        );
    lineam_purga(term.reg_summa, 0, term.col_num - 1);
}

/* ================================================================
 * motus cursoris
 * ================================================================ */

static void cursor_fixa(void)
{
    if (term.cur_lin < 0)
        term.cur_lin = 0;
    if (term.cur_lin >= term.lin_num)
        term.cur_lin = term.lin_num - 1;
    if (term.cur_col < 0)
        term.cur_col = 0;
    if (term.cur_col >= term.col_num)
        term.cur_col = term.col_num - 1;
}

static void linea_nova(void)
{
    if (term.cur_lin == term.reg_ima)
        sursum_volve();
    else if (term.cur_lin < term.lin_num - 1)
        term.cur_lin++;
}

/* ================================================================
 * parser VT100 — SGR (Select Graphic Rendition)
 * ================================================================ */

static void sgr_exsequere(void)
{
    /* colores ANSI ignorantur — color est proprietas fontis.
     * solum inverse (7) et reset (0) tractantur.
     * \x1b[m sine parametris aequivalet \x1b[0m. */
    if (term.n_params == 0) {
        term.attr = 0;
        return;
    }
    for (int i = 0; i < term.n_params; i++) {
        int p = term.params[i];
        if (p == 0) {
            term.attr = 0;
        } else if (p == 7) {
            term.attr |= 1;  /* inversus */
        } else if (p == 27) {
            term.attr &= (uint8_t)~1;
        } else if (
            p == 38 && i + 2 < term.n_params &&
            term.params[i + 1] == 5
        ) {
            i += 2;
        } else if (
            p == 48 && i + 2 < term.n_params &&
            term.params[i + 1] == 5
        ) {
            i += 2;
        }
    }
}

/* ================================================================
 * parser VT100 — sequentiae CSI
 * ================================================================ */

static void ad_pty_scribe(const char *s, int n)
{
    while (n > 0) {
        ssize_t r = write(pty_mag, s, (size_t)n);
        if (r <= 0)
            break;
        s += r;
        n -= (int)r;
    }
}

static int param(int idx, int prae)
{
    if (idx < term.n_params && term.params[idx] > 0)
        return term.params[idx];
    return prae;
}

static void csi_exsequere(int ch)
{
    int n, m;

    if (term.privatus) {
        int activare = (ch == 'h');
        for (int i = 0; i < term.n_params; i++) {
            int p = term.params[i];
            if (p == 1) {
                term.app_cursor = activare;
            } else if (p == 7) {
                term.involutio = activare;
            } else if (p == 25) {
                /* cursor visibilis — semper visibilis */
            } else if (p == 47 || p == 1047) {
                if (activare && !term.alt_activum) {
                    memcpy(
                        term.alt_cellae, term.cellae,
                        sizeof(term.cellae)
                    );
                    term.alt_cur_lin = term.cur_lin;
                    term.alt_cur_col = term.cur_col;
                    term.alt_activum = 1;
                    regionem_purga(
                        0, 0, term.lin_num - 1,
                        term.col_num - 1
                    );
                } else if (!activare && term.alt_activum) {
                    memcpy(
                        term.cellae, term.alt_cellae,
                        sizeof(term.cellae)
                    );
                    term.cur_lin     = term.alt_cur_lin;
                    term.cur_col     = term.alt_cur_col;
                    term.alt_activum = 0;
                }
            } else if (p == 1049) {
                if (activare && !term.alt_activum) {
                    term.salv_lin = term.cur_lin;
                    term.salv_col = term.cur_col;
                    memcpy(
                        term.alt_cellae, term.cellae,
                        sizeof(term.cellae)
                    );
                    term.alt_cur_lin = term.cur_lin;
                    term.alt_cur_col = term.cur_col;
                    term.alt_activum = 1;
                    regionem_purga(
                        0, 0, term.lin_num - 1,
                        term.col_num - 1
                    );
                } else if (!activare && term.alt_activum) {
                    memcpy(
                        term.cellae, term.alt_cellae,
                        sizeof(term.cellae)
                    );
                    term.cur_lin     = term.salv_lin;
                    term.cur_col     = term.salv_col;
                    term.alt_activum = 0;
                }
            }
        }
        return;
    }

    switch (ch) {
    case 'A':
        n = param(0, 1);
        term.cur_lin -= n;
        cursor_fixa();
        break;
    case 'B':
        n = param(0, 1);
        term.cur_lin += n;
        cursor_fixa();
        break;
    case 'C':
        n = param(0, 1);
        term.cur_col += n;
        cursor_fixa();
        break;
    case 'D':
        n = param(0, 1);
        term.cur_col -= n;
        cursor_fixa();
        break;
    case 'H':
    case 'f':
        n = param(0, 1);
        m = param(1, 1);
        term.cur_lin = n - 1;
        term.cur_col = m - 1;
        cursor_fixa();
        break;
    case 'J':
        n = param(0, 0);
        if (n == 0)
            regionem_purga(
                term.cur_lin, term.cur_col,
                term.lin_num - 1, term.col_num - 1
            );
        else if (n == 1)
            regionem_purga(0, 0, term.cur_lin, term.cur_col);
        else if (n == 2 || n == 3)
            regionem_purga(0, 0, term.lin_num - 1, term.col_num - 1);
        break;
    case 'K':
        n = param(0, 0);
        if (n == 0)
            lineam_purga(term.cur_lin, term.cur_col, term.col_num - 1);
        else if (n == 1)
            lineam_purga(term.cur_lin, 0, term.cur_col);
        else if (n == 2)
            lineam_purga(term.cur_lin, 0, term.col_num - 1);
        break;
    case 'L':
        n = param(0, 1);
        for (int i = 0; i < n; i++) {
            for (int l = term.reg_ima; l > term.cur_lin; l--)
                memcpy(
                    term.cellae[l], term.cellae[l - 1],
                    sizeof(cella_t) * (size_t)term.col_num
                );
            lineam_purga(term.cur_lin, 0, term.col_num - 1);
        }
        break;
    case 'M':
        n = param(0, 1);
        for (int i = 0; i < n; i++) {
            for (int l = term.cur_lin; l < term.reg_ima; l++)
                memcpy(
                    term.cellae[l], term.cellae[l + 1],
                    sizeof(cella_t) * (size_t)term.col_num
                );
            lineam_purga(term.reg_ima, 0, term.col_num - 1);
        }
        break;
    case 'P':
        n = param(0, 1);
        for (int i = term.cur_col; i < term.col_num - n; i++)
            term.cellae[term.cur_lin][i] =
                term.cellae[term.cur_lin][i + n];
        for (int i = term.col_num - n; i < term.col_num; i++)
            cellam_purga(term.cur_lin, i);
        break;
    case '@':
        n = param(0, 1);
        for (int i = term.col_num - 1; i >= term.cur_col + n; i--)
            term.cellae[term.cur_lin][i] =
                term.cellae[term.cur_lin][i - n];
        for (
            int i = term.cur_col; i < term.cur_col + n &&
            i < term.col_num; i++
        )
            cellam_purga(term.cur_lin, i);
        break;
    case 'S':
        n = param(0, 1);
        for (int i = 0; i < n; i++)
            sursum_volve();
        break;
    case 'T':
        n = param(0, 1);
        for (int i = 0; i < n; i++)
            deorsum_volve();
        break;
    case 'd':
        n = param(0, 1);
        term.cur_lin = n - 1;
        cursor_fixa();
        break;
    case 'G':
    case '`':
        n = param(0, 1);
        term.cur_col = n - 1;
        cursor_fixa();
        break;
    case 'm':
        sgr_exsequere();
        break;
    case 'r':
        n = param(0, 1);
        m = param(1, term.lin_num);
        term.reg_summa = n - 1;
        term.reg_ima   = m - 1;
        if (term.reg_summa < 0)
            term.reg_summa = 0;
        if (term.reg_ima >= term.lin_num)
            term.reg_ima = term.lin_num - 1;
        if (term.reg_summa >= term.reg_ima) {
            term.reg_summa = 0;
            term.reg_ima   = term.lin_num - 1;
        }
        term.cur_lin = 0;
        term.cur_col = 0;
        break;
    case 'c':
        ad_pty_scribe("\x1b[?1;0c", 7);
        break;
    case 'n':
        n = param(0, 0);
        if (n == 6) {
            char resp[32];
            int rn = snprintf(
                resp, sizeof(resp), "\x1b[%d;%dR",
                term.cur_lin + 1, term.cur_col + 1
            );
            ad_pty_scribe(resp, rn);
        } else if (n == 5) {
            ad_pty_scribe("\x1b[0n", 4);
        }
        break;
    case 's':
        term.salv_lin = term.cur_lin;
        term.salv_col = term.cur_col;
        break;
    case 'u':
        term.cur_lin = term.salv_lin;
        term.cur_col = term.salv_col;
        cursor_fixa();
        break;
    case 'X':
        n = param(0, 1);
        for (int i = 0; i < n && term.cur_col + i < term.col_num; i++)
            cellam_purga(term.cur_lin, term.cur_col + i);
        break;
    case 'E':
        n = param(0, 1);
        term.cur_lin += n;
        term.cur_col = 0;
        cursor_fixa();
        break;
    case 'F':
        n = param(0, 1);
        term.cur_lin -= n;
        term.cur_col = 0;
        cursor_fixa();
        break;
    default:
        break;
    }
}

/* ================================================================
 * parser VT100 — ansa principalis
 * ================================================================ */

static void characterem_pone(int ch)
{
    if (term.cur_col >= term.col_num) {
        if (term.involutio) {
            term.cur_col = 0;
            linea_nova();
        } else {
            term.cur_col = term.col_num - 1;
        }
    }

    cella_t c;
    c.ch   = (uint8_t)ch;
    c.attr = term.attr;
    cellam_pone(term.cur_lin, term.cur_col, c);
    term.cur_col++;
}

static void octum_processa(int ch)
{
    switch (term.status) {
    case S_NORMALIS:
        if (ch == 0x1b) {
            term.status = S_ESC;
        } else if (ch == '\r') {
            term.cur_col = 0;
        } else if (ch == '\n' || ch == '\x0b' || ch == '\x0c') {
            linea_nova();
        } else if (ch == '\b') {
            if (term.cur_col > 0)
                term.cur_col--;
        } else if (ch == '\t') {
            term.cur_col = (term.cur_col + 8) & ~7;
            if (term.cur_col >= term.col_num)
                term.cur_col = term.col_num - 1;
        } else if (ch == '\a') {
            /* campana — ignora */
        } else if (ch >= 0x20 && ch < 0x7f) {
            characterem_pone(ch);
        } else if (ch >= 0x80) {
            characterem_pone(ch);
        }
        break;

    case S_ESC:
        if (ch == '[') {
            term.status   = S_CSI;
            term.n_params = 0;
            term.privatus = 0;
            memset(term.params, 0, sizeof(term.params));
        } else if (ch == ']') {
            term.status = S_OSC;
        } else if (ch == '7') {
            term.salv_lin = term.cur_lin;
            term.salv_col = term.cur_col;
            term.status   = S_NORMALIS;
        } else if (ch == '8') {
            term.cur_lin = term.salv_lin;
            term.cur_col = term.salv_col;
            cursor_fixa();
            term.status = S_NORMALIS;
        } else if (ch == 'D') {
            linea_nova();
            term.status = S_NORMALIS;
        } else if (ch == 'M') {
            if (term.cur_lin == term.reg_summa)
                deorsum_volve();
            else if (term.cur_lin > 0)
                term.cur_lin--;
            term.status = S_NORMALIS;
        } else if (ch == 'c') {
            int cn = term.col_num, ln = term.lin_num;
            memset(&term, 0, sizeof(term));
            term.col_num   = cn;
            term.lin_num   = ln;
            term.reg_ima   = ln - 1;
            term.involutio = 1;
            regionem_purga(0, 0, ln - 1, cn - 1);
            term.status = S_NORMALIS;
        } else if (ch == '(' || ch == ')' || ch == '*' || ch == '+') {
            term.status = S_ESC_CHARSET;
        } else if (ch == '=' || ch == '>') {
            term.status = S_NORMALIS;
        } else {
            term.status = S_NORMALIS;
        }
        break;

    case S_CSI:
        if (ch == '?') {
            term.privatus = 1;
        } else if (ch >= '0' && ch <= '9') {
            if (term.n_params == 0)
                term.n_params = 1;
            term.params[term.n_params - 1] =
                term.params[term.n_params - 1] * 10 + (ch - '0');
        } else if (ch == ';') {
            if (term.n_params < PARAM_MAX)
                term.n_params++;
        } else if (ch >= 0x40 && ch <= 0x7e) {
            if (term.n_params == 0 && ch != 'm')
                term.n_params = 0;
            csi_exsequere(ch);
            term.status = S_NORMALIS;
        } else if (ch >= 0x20 && ch < 0x40) {
            /* intermedii — ignora */
        } else {
            term.status = S_NORMALIS;
        }
        break;

    case S_OSC:
        if (ch == '\a' || ch == '\\')
            term.status = S_NORMALIS;
        break;

    case S_ESC_CHARSET:
        term.status = S_NORMALIS;
        break;
    }
}

/* ================================================================
 * redditio — terminalis ad pixels
 * ================================================================ */

static void redde(uint32_t *pixels)
{
    /*
     * Scribe sequentialiter per lineas scansionis (imum ad summum
     * alvei = summum ad imum fenestrae ob CG y-inversus).
     * Glyphos praecalculatos adhibet — nullae functiones in ansa.
     */
    for (int lin = 0; lin < term.lin_num; lin++) {
        /* linea alvei: lin 0 -> imum alvei (summum fenestrae) */
        int base_y = (term.lin_num - 1 - lin) * glyph_alt_s;

        for (int row = 0; row < glyph_alt_s; row++) {
            uint32_t *dest = pixels
                + (base_y + glyph_alt_s - 1 - row) * fen_lat;

            for (int col = 0; col < term.col_num; col++) {
                cella_t ce = term.cellae[lin][col];
                int ch     = ce.ch;
                int est_cursor = (
                    lin == term.cur_lin &&
                    col == term.cur_col
                );

                const uint32_t *srow = glyphi[ch]
                    + row * glyph_lat_s;
                int inversus = est_cursor ^ (ce.attr & 1);

                if (inversus) {
                    for (int x = 0; x < glyph_lat_s; x++) {
                        uint32_t p = srow[x];
                        *dest++ = 0xFF000000
                            | ((255 - ((p >> 16) & 0xFF)) << 16)
                            | ((255 - ((p >> 8) & 0xFF)) << 8)
                            | (255 - (p & 0xFF));
                    }
                } else {
                    for (int x = 0; x < glyph_lat_s; x++)
                        *dest++ = srow[x];
                }
            }

            /* spatium residuum (si fenestra latior quam terminalis) */
            int residuum = fen_lat - term.col_num * glyph_lat_s;
            if (residuum > 0) {
                memset(dest, 0, (size_t)residuum * 4);
                dest += residuum;
            }
        }
    }

    /* lineae residuae infra terminalem */
    int lineae_pix = term.lin_num * glyph_alt_s;
    if (lineae_pix < fen_alt) {
        memset(
            pixels + lineae_pix * fen_lat, 0,
            (size_t)(fen_alt - lineae_pix) * (size_t)fen_lat * 4
        );
    }
}

/* ================================================================
 * claviatura — eventus phantasma ad sequentias PTY
 * ================================================================ */

static void clavem_mitte(int symbolum)
{
    char alv[16];
    int  n = 0;

    if (symbolum == PFR_CL_SURSUM) {
        if (term.app_cursor)
            n = snprintf(alv, sizeof(alv), "\x1bOA");
        else
            n = snprintf(alv, sizeof(alv), "\x1b[A");
    } else if (symbolum == PFR_CL_DEORSUM) {
        if (term.app_cursor)
            n = snprintf(alv, sizeof(alv), "\x1bOB");
        else
            n = snprintf(alv, sizeof(alv), "\x1b[B");
    } else if (symbolum == PFR_CL_DEXTRUM) {
        if (term.app_cursor)
            n = snprintf(alv, sizeof(alv), "\x1bOC");
        else
            n = snprintf(alv, sizeof(alv), "\x1b[C");
    } else if (symbolum == PFR_CL_SINISTRUM) {
        if (term.app_cursor)
            n = snprintf(alv, sizeof(alv), "\x1bOD");
        else
            n = snprintf(alv, sizeof(alv), "\x1b[D");
    } else if (symbolum == '\r' || symbolum == '\n') {
        alv[0] = '\r';
        n      = 1;
    } else if (symbolum == 0x7f) {
        alv[0] = 0x7f;
        n      = 1;
    } else if (symbolum == '\t') {
        alv[0] = '\t';
        n      = 1;
    } else if (symbolum == 0x1b) {
        alv[0] = 0x1b;
        n      = 1;
    } else if (symbolum >= 1 && symbolum <= 26) {
        alv[0] = (char)symbolum;
        n      = 1;
    } else if (symbolum >= 0x20 && symbolum < 0x7f) {
        alv[0] = (char)symbolum;
        n      = 1;
    }

    if (n > 0)
        ad_pty_scribe(alv, n);
}


/* ================================================================
 * redimensionatio
 * ================================================================ */

static void redimensiona(
    int nov_fen_lat, int nov_fen_alt,
    pfr_pictor_t *pictor,
    pfr_textura_t **textura_p,
    uint32_t **pix_p
) {
    /* novas dimensiones cellarum computa (ad multipla rotunda) */
    int nov_col = nov_fen_lat / glyph_lat_s;
    int nov_lin = nov_fen_alt / glyph_alt_s;
    if (nov_col < 1)
        nov_col = 1;
    if (nov_lin < 1)
        nov_lin = 1;
    if (nov_col > COL_MAX)
        nov_col = COL_MAX;
    if (nov_lin > LIN_MAX)
        nov_lin = LIN_MAX;

    if (nov_col == term.col_num && nov_lin == term.lin_num)
        return;

    /* dimensiones fenestrae ad multipla cellarum rotunda */
    fen_lat = nov_col * glyph_lat_s;
    fen_alt = nov_lin * glyph_alt_s;

    /* pictorem redimensiona (alveum et dimensiones internas renovat) */
    pfr_pictorem_redimensiona(pictor, fen_lat, fen_alt);

    /* texturam recrea */
    if (*textura_p)
        pfr_texturam_destrue(*textura_p);
    *textura_p = pfr_texturam_crea(
        pictor, PFR_PIXEL_ARGB8888, PFR_TEXTURA_FLUENS,
        fen_lat, fen_alt
    );

    /* alveum pixelorum recrea */
    free(*pix_p);
    *pix_p = calloc(
        (size_t)fen_lat * (size_t)fen_alt,
        sizeof(uint32_t)
    );

    /* novas lineas purga si terminalis crevit */
    for (int l = term.lin_num; l < nov_lin; l++)
        for (int c = 0; c < nov_col; c++) {
        term.cellae[l][c].ch   = ' ';
        term.cellae[l][c].attr = 0;
    }
    /* novas columnas purga si terminalis latior */
    for (int l = 0; l < nov_lin && l < term.lin_num; l++)
        for (int c = term.col_num; c < nov_col; c++) {
        term.cellae[l][c].ch   = ' ';
        term.cellae[l][c].attr = 0;
    }

    /* dimensiones renova */
    term.col_num   = nov_col;
    term.lin_num   = nov_lin;
    term.reg_summa = 0;
    term.reg_ima   = nov_lin - 1;
    cursor_fixa();

    /* PTY de novis dimensionibus certiorem fac */
    struct winsize ws;
    memset(&ws, 0, sizeof(ws));
    ws.ws_row = (unsigned short)nov_lin;
    ws.ws_col = (unsigned short)nov_col;
    ioctl(pty_mag, TIOCSWINSZ, &ws);
}

/* ================================================================
 * principale
 * ================================================================ */

int main(int argc, char **argv)
{
    const char *via_fontis = "mork_16.gif";
    scala = SCALA_PRAE;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            scala = atoi(argv[++i]);
            if (scala < 1 || scala > 16 || (scala & (scala - 1)) != 0)
                scala = SCALA_PRAE;
        } else {
            via_fontis = argv[i];
        }
    }

    /* terminalis initia */
    memset(&term, 0, sizeof(term));
    term.col_num   = COL_PRAE;
    term.lin_num   = LIN_PRAE;
    term.reg_ima   = LIN_PRAE - 1;
    term.involutio = 1;

    regionem_purga(0, 0, term.lin_num - 1, term.col_num - 1);

    /* fontem onera */
    if (fontem_onera(via_fontis) != 0) {
        fprintf(stderr, "xiterm: fons %s onerari non potest\n", via_fontis);
        return 1;
    }

    fen_lat = term.col_num * glyph_lat_s;
    fen_alt = term.lin_num * glyph_alt_s;

    /* pseudo-terminalis crea */
    filius = pty_crea(&pty_mag, term.col_num, term.lin_num);
    if (filius < 0) {
        fprintf(stderr, "xiterm: pty creari non potest\n");
        return 1;
    }

    /* phantasma initia */
    if (pfr_initia(PFR_INITIA_VIDEO) != 0) {
        fprintf(
            stderr, "xiterm: phantasma initiari non potest: %s\n",
            pfr_erratum()
        );
        return 1;
    }

    pfr_fenestra_t *fenestra = pfr_fenestram_crea(
        "xiterm", PFR_POS_MEDIUM, PFR_POS_MEDIUM,
        fen_lat, fen_alt, 0
    );
    if (!fenestra) {
        fprintf(stderr, "xiterm: fenestra creari non potest\n");
        pfr_fini();
        return 1;
    }

    pfr_pictor_t *pictor = pfr_pictorem_crea(
        fenestra, -1, PFR_PICTOR_CELER | PFR_PICTOR_SYNC
    );
    if (!pictor) {
        fprintf(stderr, "xiterm: pictor creari non potest\n");
        pfr_fenestram_destrue(fenestra);
        pfr_fini();
        return 1;
    }

    pfr_textura_t *textura = pfr_texturam_crea(
        pictor, PFR_PIXEL_ARGB8888, PFR_TEXTURA_FLUENS,
        fen_lat, fen_alt
    );
    if (!textura) {
        fprintf(stderr, "xiterm: textura creari non potest\n");
        pfr_pictorem_destrue(pictor);
        pfr_fenestram_destrue(fenestra);
        pfr_fini();
        return 1;
    }

    uint32_t *pix = calloc(
        (size_t)fen_lat * (size_t)fen_alt,
        sizeof(uint32_t)
    );
    if (!pix) {
        fprintf(stderr, "xiterm: memoria exhausta\n");
        pfr_texturam_destrue(textura);
        pfr_pictorem_destrue(pictor);
        pfr_fenestram_destrue(fenestra);
        pfr_fini();
        return 1;
    }

    signal(SIGCHLD, SIG_IGN);

    int currens = 1;
    while (currens) {
        pfr_eventus_t ev;
        while (pfr_eventum_lege(&ev)) {
            if (ev.typus == PFR_EXITUS) {
                currens = 0;
                break;
            }
            if (ev.typus == PFR_CLAVIS_INF) {
                int sym = ev.clavis.signum.symbolum;
                if (sym != 0)
                    clavem_mitte(sym);
            }
            if (ev.typus == PFR_FENESTRA_MUTATA) {
                redimensiona(
                    ev.fenestra.lat, ev.fenestra.alt,
                    pictor, &textura, &pix
                );
            }
        }
        if (!currens)
            break;

        /* lege ex pty */
        {
            uint8_t alveus[ALVEUS_PTY];
            ssize_t n = read(pty_mag, alveus, sizeof(alveus));
            if (n > 0) {
                for (ssize_t i = 0; i < n; i++)
                    octum_processa(alveus[i]);
            } else if (
                n == 0 || (
                    n < 0 && errno != EAGAIN &&
                    errno != EWOULDBLOCK
                )
            ) {
                int status;
                if (waitpid(filius, &status, WNOHANG) != 0)
                    currens = 0;
            }
        }

        /* redde et praesenta */
        if (pix && textura) {
            redde(pix);
            pfr_texturam_renova(textura, NULL, pix, fen_lat * 4);
            pfr_purga(pictor);
            pfr_texturam_pinge(pictor, textura, NULL, NULL);
            pfr_praesenta(pictor);
        }

        pfr_pausa(16);
    }

    free(pix);
    if (textura)
        pfr_texturam_destrue(textura);
    pfr_pictorem_destrue(pictor);
    pfr_fenestram_destrue(fenestra);
    pfr_fini();
    close(pty_mag);

    return 0;
}
