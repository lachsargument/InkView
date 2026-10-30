"""Re-encode BDF bitmap fonts to ISO10646 (Unicode), staying BDF.

InkView's BDF loader only understands Unicode encodings. Non-Unicode
X11 fonts (JIS X 0208/0201, Big5, GB2312) are remapped here, once, at
build time. Join files with "+" to merge them: the first file supplies
the header and metrics, later files only fill code points still
missing (e.g. jiskan16.bdf+8x16rk.bdf adds half-width kana and roman).

The charset mapping follows bdf2ttf.py from the bitmap-fonts TTF work.

Usage:
    python3 tools/bdfuni.py OUT.bdf FILE.bdf[+FILE.bdf]...
"""

import sys
from pathlib import Path


def to_unicode(enc, registry):
    """Map a BDF ENCODING in the given charset to a code point or None."""
    registry = registry.upper()
    if registry.startswith(("ISO10646", "ISO8859-1")):
        return enc
    if registry.startswith("JISX0201"):
        # JIS-Roman differs from ASCII only at yen sign and overline
        if 0x20 <= enc < 0x80:
            return {0x5C: 0xA5, 0x7E: 0x203E}.get(enc, enc)
        return 0xFF61 + enc - 0xA1 if 0xA1 <= enc <= 0xDF else None
    hi, lo = enc >> 8, enc & 0xFF
    if registry.startswith("BIG5"):
        # cp950 lacks most ETen extensions (kana, Cyrillic at C6A1-C8FE),
        # big5hkscs has them
        candidates = [(bytes([hi, lo]), "cp950"), (bytes([hi, lo]), "big5hkscs")]
    elif registry.startswith("JISX0208"):
        candidates = [(bytes([hi | 0x80, lo | 0x80]), "euc_jp")]
    elif registry.startswith("GB2312"):
        candidates = [(bytes([hi | 0x80, lo | 0x80]), "gb2312")]
    else:
        sys.exit(f"unsupported charset {registry}")
    for raw, codec in candidates:
        try:
            s = raw.decode(codec)
        except UnicodeDecodeError:
            continue
        if len(s) == 1:
            return ord(s)
    return None


def read_bdf(path):
    """Return (header lines, registry, glyph blocks as line lists)."""
    lines = Path(path).read_text(encoding="latin-1").splitlines()
    start = next(i for i, l in enumerate(lines) if l.startswith("STARTCHAR"))
    header = [l for l in lines[:start] if not l.startswith("CHARS ")]
    registry = "ISO10646"
    for l in header:
        if l.startswith("CHARSET_REGISTRY"):
            registry = l.split(None, 1)[1].strip('"')
    blocks, block = [], None
    for l in lines[start:]:
        if l.startswith("STARTCHAR"):
            block = [l]
        elif block is not None:
            block.append(l)
            if l == "ENDCHAR":
                blocks.append(block)
                block = None
    return header, registry, blocks


def encoding(block):
    for l in block:
        if l.startswith("ENCODING"):
            return int(l.split()[1])
    return -1


def rewrite_header(header, registry):
    out = []
    for l in header:
        key = l.split(None, 1)[0] if l.strip() else ""
        if key == "FONT":
            # XLFD ends in -REGISTRY-ENCODING
            out.append("-".join(l.split("-")[:-2] + ["ISO10646", "1"]))
        elif key == "CHARSET_REGISTRY":
            out.append('CHARSET_REGISTRY "ISO10646"')
        elif key == "CHARSET_ENCODING":
            out.append('CHARSET_ENCODING "1"')
        elif key == "DEFAULT_CHAR":
            cp = to_unicode(int(l.split()[1]), registry)
            out.append(f"DEFAULT_CHAR {cp if cp is not None else 0xFFFD}")
        else:
            out.append(l)
    return out


def convert(out, spec):
    first, *rest = spec.split("+")
    header, registry, _ = read_bdf(first)
    by_cp, unmapped = {}, 0
    for path in [first, *rest]:
        _, reg, blocks = read_bdf(path)
        for b in blocks:
            # unencoded glyphs (ENCODING -1) are unreachable, drop them
            enc = encoding(b)
            if enc < 0:
                continue
            cp = to_unicode(enc, reg)
            if cp is None:
                unmapped += path == first
            elif cp not in by_cp:
                by_cp[cp] = [f"ENCODING {cp}" if l.startswith("ENCODING") else l for l in b]
    lines = rewrite_header(header, registry)
    lines.append(f"CHARS {len(by_cp)}")
    for cp in sorted(by_cp):
        lines.extend(by_cp[cp])
    lines.append("ENDFONT")
    Path(out).write_text("\n".join(lines) + "\n", encoding="latin-1")
    note = f", {unmapped} unmapped" if unmapped else ""
    print(f"{out}: {len(by_cp)} chars{note}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    convert(sys.argv[1], sys.argv[2])
