# Fonts

InkView ships the small bitmap fonts in this folder. The large fonts
come from `tools/fetch-fonts.sh`, which downloads pinned versions and
checks their SHA-256. Run it once after you clone the repository:

```sh
sh tools/fetch-fonts.sh
```

## In the repository

| File | Font | License |
|---|---|---|
| `inkscreen14.bdf`, `inkscreen14b.bdf` | Inkscreen, by Caffeine Type Foundry | SIL OFL 1.1, see `OFL.txt` |
| `fangsongti16.bdf` | Fangsong ti, GB 2312 | Institute of Software, Academia Sinica, and Sony: permissive, notice in the BDF header |
| `jiskan16.bdf` | JIS X 9051 16-dot kanji | by permission to use, notice in the BDF header |
| `k14.bdf` | 14-dot kanji by H. Kagotani | public domain |
| `8x16rk.bdf` | Sony half-width kana | Sony: permissive, notice in the BDF header |
| `unicode/*.bdf` | the JIS fonts above, encoded as Unicode by `tools/bdfuni.py` | same as the source font |

Inkscreen is built in the caffeine-type repository. Do not edit the
BDF files here. Copy them from caffeine-type after a rebuild.

## Fetched

| Folder or file | Font | License |
|---|---|---|
| `inksans/` | Inksans CJK W3 and W6, by Caffeine Type Foundry | SIL OFL 1.1, see `inksans/OFL.txt` |
| `freefont/` | GNU FreeFont: FreeSans, FreeSerif, FreeMono | GPL 3 with the font exception, see `freefont/COPYING` |
| `notoemoji/` | Noto Emoji, monochrome | SIL OFL 1.1, see `notoemoji/OFL.txt` |
| `courier/` | Courier 10 Pitch, Bitstream Type 1 | Bitstream notice, see `courier/COPYING` |
| `unifont.otf`, `unifont-upper.otf` | GNU Unifont 18.0.01 | SIL OFL 1.1 (also GPL 2 with the font exception) |

Keep the FreeFont files separate from the other fonts. Do not merge
them into one font file. A merged font falls under the GPL as a whole.
