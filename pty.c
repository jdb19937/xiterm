/*
 * pty.c — pseudo-terminalis POSIX
 *
 * Implementatio propria forkpty() solum POSIX functionibus utens:
 * posix_openpt, grantpt, unlockpt, ptsname, setsid, ioctl.
 * Nulla dependentia libutil.
 */

#include "pty.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>

/*
 * pty_forca — pseudo-terminale aperit et processus forcat.
 *
 * Magister: posix_openpt(O_RDWR | O_NOCTTY) aperit PTY magistrum.
 * grantpt() et unlockpt() servum praeparant.
 * ptsname() nomen servi reddit.
 *
 * Post fork():
 *   Filius: setsid() novam sessionem creat, servum aperit (fit
 *   terminalis moderans), stdin/stdout/stderr ad servum ducit.
 *   Parens: descriptorem magistri reddit.
 */
pid_t pty_forca(
    int *mag,
    struct termios *tp,
    struct winsize *ws
) {
    /* magistrum aperi */
    int m = posix_openpt(O_RDWR | O_NOCTTY);
    if (m < 0)
        return -1;

    if (grantpt(m) != 0 || unlockpt(m) != 0) {
        close(m);
        return -1;
    }

    char *nomen_servi = ptsname(m);
    if (!nomen_servi) {
        close(m);
        return -1;
    }

    pid_t p = fork();
    if (p < 0) {
        close(m);
        return -1;
    }

    if (p == 0) {
        /* filius */
        close(m);

        /* nova sessio — filius fit dux sessionis */
        if (setsid() < 0)
            _exit(127);

        /* servum aperi — fit terminalis moderans */
        int s = open(nomen_servi, O_RDWR);
        if (s < 0)
            _exit(127);

#ifdef TIOCSCTTY
        /* in quibusdam systematibus necessarium */
        ioctl(s, TIOCSCTTY, 0);
#endif

        /* attributa terminalis pone */
        if (tp)
            tcsetattr(s, TCSANOW, tp);
        if (ws)
            ioctl(s, TIOCSWINSZ, ws);

        /* stdin, stdout, stderr ad servum duc */
        if (s != 0) { dup2(s, 0); }
        if (s != 1) { dup2(s, 1); }
        if (s != 2) { dup2(s, 2); }
        if (s > 2)
            close(s);

        return 0;
    }

    /* parens */
    *mag = m;
    return p;
}

pid_t pty_crea(int *mag, int col, int lin)
{
    struct winsize ws;
    memset(&ws, 0, sizeof(ws));
    ws.ws_row = (unsigned short)lin;
    ws.ws_col = (unsigned short)col;

    pid_t p = pty_forca(mag, NULL, &ws);
    if (p < 0)
        return -1;

    if (p == 0) {
        /* filius — conchae processus */
        setenv("TERM", "xterm", 1);

        char dim[32];
        snprintf(dim, sizeof(dim), "%d", col);
        setenv("COLUMNS", dim, 1);
        snprintf(dim, sizeof(dim), "%d", lin);
        setenv("LINES", dim, 1);

        const char *concha = getenv("SHELL");
        if (!concha)
            concha = "/bin/bash";

        execlp(concha, concha, "-l", (char *)NULL);
        _exit(127);
    }

    /* magister — non-blocking */
    int fl = fcntl(*mag, F_GETFL);
    if (fl >= 0)
        fcntl(*mag, F_SETFL, fl | O_NONBLOCK);

    return p;
}
