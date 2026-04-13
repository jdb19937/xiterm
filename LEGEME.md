# xiterm

Emulator terminalis graphicus. Fenestram per phantasma aperit, processus
conchae cum pseudo-terminali forcat, codices ANSI/VT100 interpretatur,
et glyphos ex atlante GIF reddit.

> **Nota:** README.md ignorandum est. Anglice scriptum est ad barbaros ineruditos pecunia emungendos.

## Aedificatio

Requirit phantasma installatam in `/opt/apotheca`.

```
face
```

Executabile `xiterm` generatur. Instrumentum `titulus_genera` quoque
aedificatur, quod fontem Titulus (atlas GIF) generat.

```
face purga
```

## Usus

```
xiterm [via_fontis] [-s scala]
```

Sine argumentis, `mork_16.gif` cum scala 4 adhibetur. Scala potentia
duorum esse debet (1, 2, 4, 8, 16).

Dimensiones praefinitae: 96 columnae, 64 lineae. Fenestra
redimensionabilis est — cellae ad multipla glyphorum rotundantur.

## Plicae

| Plica | Descriptio |
|---|---|
| `xiterm.c` | fons principalis — parser VT100, redditio, ansa eventuum |
| `pty.c` / `pty.h` | pseudo-terminalis POSIX sine dependentia libutil |
| `titulus_genera.c` | generator fontis Titulus — atlas 512x768 |
| `mork_16.gif` | atlas glyphorum praefinitus |
| `Faceplica` | aedificatio |

## Parser VT100

Codices sufficientes ad `bash` et `vi` currenda:

- CSI sequentiae: motus cursoris, deletio, insertio, volumen, SGR
- Modi privati: cursor applicativus, involutio, velum alterum (47, 1047, 1049)
- ESC sequentiae: cursor salvatus/restitutus, reset, charset
- OSC: acceptatur et ignoratur

Colores ANSI ignorantur — color proprietas fontis est. Solum inversio
(SGR 7) et reset (SGR 0) tractantur.

## Redditio

Glyphi ex atlante 512x768 (16x16 reticulo, singulus 32x48 pixelorum)
onerantur. Ad scalam currentem praecalculantur per mediationem quadratorum.
Cursor per inversionem colorum indicatur.

## Fontes

`titulus_genera` fontem Titulus generat — serif-inspiratum a titulis
Romanis monumentalibus. Glyphi in reticulo 8x12 definiuntur, deinde
4x amplificantur ad 32x48. Zero obliquus, I cum serifis, 1 cum basi.

```
face titulus_16.gif
```

## Dependentiae

- phantasma (`/opt/apotheca/lib/libphantasma.a`)
- Compilator C cum C99
- POSIX (fork, posix_openpt, setsid, ioctl, poll)

## Collocatio

```
face colloca
```

Executabile in `/opt/apotheca/bin/` collocatur.
