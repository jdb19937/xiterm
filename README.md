# xiterm

A complete graphical terminal emulator built from raw pixels and POSIX
primitives, with absolutely zero external dependencies beyond a C
compiler and the operating system itself. Every byte of the rendering
pipeline, every escape sequence in the VT100 parser, every glyph in the
font atlas — all handcrafted, all self-contained, all yours.

Where other terminal emulators drag in massive toolkit dependencies,
font rendering libraries, and Unicode shaping engines just to show you a
blinking cursor, xiterm does everything with a single C file, a GIF
atlas, and a straightforward POSIX pseudo-terminal implementation that
doesn't even need libutil.

## What It Does

xiterm opens a native window, forks a shell with a pseudo-terminal, and
renders the entire terminal display by painting individual glyphs from a
bitmap font atlas. The VT100/ANSI parser handles cursor movement,
scrolling regions, line insertion and deletion, alternate screen buffers,
application cursor mode, and SGR attributes — everything you need to run
`bash`, `vi`, and the full constellation of interactive terminal
applications without missing a beat.

The window is freely resizable. Drag it to any size and xiterm
recalculates the cell grid, recreates the rendering surface, and
notifies the shell of its new dimensions — all in a single frame.

## The Font

xiterm ships with its own font generator. `titulus_genera` produces a
Roman monumental serif font where every glyph is defined as an 8x12
bitmap and amplified 4x to a crisp 32x48 pixel cell. The result is a
512x768 atlas packed into a single GIF file — 256 glyphs, zero
dependencies on FreeType, HarfBuzz, or any font rendering stack
whatsoever.

The design draws from classical Roman inscriptional lettering: a slashed
zero for disambiguation, serifed capitals, a `1` with a proper base.
Every character is legible at multiple scales.

## Building

```bash
make
```

That's it. One command, one binary. The build produces `xiterm` and the
optional `titulus_genera` font generator.

```bash
make titulus_16.gif
```

Regenerates the font atlas from scratch.

## Usage

```bash
xiterm                     # default font, 4x scale
xiterm mork_16.gif -s 2    # custom font, 2x scale
xiterm -s 8                # 8x scale (enormous)
```

Scale must be a power of two: 1, 2, 4, 8, or 16. The default grid is 96
columns by 64 rows — substantial enough for serious work, compact enough
for a focused session.

## The PTY

Most implementations lean on `forkpty()` from libutil and call it a day.
xiterm implements the entire pseudo-terminal lifecycle from first
principles: `posix_openpt`, `grantpt`, `unlockpt`, `ptsname`, `setsid`,
and the careful `dup2` dance that makes stdin, stdout, and stderr point
at the slave device. No library wrappers, no platform abstractions, no
surprises.

The master descriptor runs non-blocking. The main loop polls it every
frame, feeds raw bytes into the VT100 parser, and the next
`pfr_praesenta` call puts the result on screen. Simple, direct,
predictable.

## The Parser

The VT100 parser is a straightforward state machine — `S_NORMALIS`,
`S_ESC`, `S_CSI`, `S_OSC`, `S_ESC_CHARSET` — that processes one byte at
a time with zero allocations and zero recursion. It handles:

- Full CSI sequences: cursor positioning, line and character
  insertion/deletion, erase in display/line, scrolling regions, SGR
  attributes
- Private mode sequences: application cursor keys, autowrap, alternate
  screen buffer (modes 47, 1047, and 1049)
- ESC sequences: cursor save/restore, index, reverse index, full reset
- OSC sequences: accepted and silently consumed

Color is a property of the font, not the terminal. ANSI color codes pass
through the parser without effect — what you see is what the font
designer intended. Inverse video (SGR 7) works as expected.

## License

Free. Public domain. Use however you like.
