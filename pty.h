/*
 * pty.h — pseudo-terminalis POSIX
 *
 * forkpty() proprium sine dependentia libutil.
 */

#ifndef PTY_H
#define PTY_H

#include <sys/types.h>
#include <termios.h>

/*
 * pty_forca — pseudo-terminale aperit et processus forcat.
 *
 * Scribit descriptorem magistri in *mag.
 * In parente reddit pid filii (> 0).
 * In filio reddit 0; stdin/stdout/stderr ad servum connexi sunt.
 * Si ws non NULL est, magnitudinem fenestrae ponit.
 * Si tp non NULL est, attributa terminalis ponit.
 * Reddit -1 si error.
 */
pid_t pty_forca(
    int *mag,
    struct termios *tp,
    struct winsize *ws
);

/*
 * pty_crea — PTY aperit, conchae processus forcat.
 *
 * Descriptorem magistri in *mag scribit, pid filii reddit.
 * Magister non-blocking ponitur.
 * Reddit pid filii, -1 si error.
 */
pid_t pty_crea(int *mag, int col, int lin);

#endif /* PTY_H */
