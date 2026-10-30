#!/usr/bin/env python3
# builds assets/ui/font.ttf from assets/ui/font.aseprite; needs Aseprite, fonttools and pillow
# set ASEPRITE to the Aseprite executable when it is not in the default macOS place

import os
import subprocess
import tempfile
from pathlib import Path

from fontTools.agl import UV2AGL
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "assets/ui/font.aseprite"
OUTPUT = ROOT / "assets/ui/font.ttf"
ASEPRITE = os.environ.get("ASEPRITE", "/Applications/Aseprite.app/Contents/MacOS/aseprite")

UNIT = 64
UPM = 16 * UNIT
ASCENT = 14
DESCENT = 4
SPACING = 2
SPACE_ADVANCE = 4

# (baseline y of the row, top y, bottom y, characters in sheet order)
ROWS = [
    (13, 0, 15, "0123456789[]:;{}-_+−/\\<>.,\"'|~!^%$#@`&=()?"),
    (35, 22, 35, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
    (57, 48, 61, "abcdefghijklmnopqrstuvwxyz"),
]
# the two dots of " are separate columns in the sheet, so they join into one glyph
EXTRA_COLUMNS = {'"': 1}
# assets: Aseprite cannot export TTF and a sheet edit must not need hand conversion; the sheet is exported to PNG, glyphs are found by column gaps and mapped by ROWS order, and one sheet pixel is 64 font units so the font is pixel exact at size 16 and multiples


def export_sheet(path):
    subprocess.run([ASEPRITE, "-b", str(SOURCE), "--save-as", str(path)], check=True, capture_output=True)


def columns(alpha, width, top, bottom):
    spans, start = [], None
    for x in range(width + 1):
        filled = x < width and any(alpha[x, y] for y in range(top, bottom + 1))
        if filled and start is None:
            start = x
        if not filled and start is not None:
            spans.append((start, x - 1))
            start = None
    return spans


def trace(pixels):
    edges = {}
    for x, y in pixels:
        for neighbour, a, b in (
            ((x, y - 1), (x, y), (x + 1, y)),
            ((x + 1, y), (x + 1, y), (x + 1, y + 1)),
            ((x, y + 1), (x + 1, y + 1), (x, y + 1)),
            ((x - 1, y), (x, y + 1), (x, y)),
        ):
            if neighbour not in pixels:
                edges.setdefault(a, []).append(b)
    loops = []
    while edges:
        start = next(iter(edges))
        loop, at = [start], start
        while True:
            nexts = edges[at]
            to = nexts.pop()
            if not nexts:
                del edges[at]
            if to == start:
                break
            loop.append(to)
            at = to
        corners = []
        for i, p in enumerate(loop):
            before, after = loop[i - 1], loop[(i + 1) % len(loop)]
            if (p[0] - before[0], p[1] - before[1]) != (after[0] - p[0], after[1] - p[1]):
                corners.append(p)
        loops.append(corners)
    return loops


def glyph_outline(pixels, baseline):
    pen = TTGlyphPen(None)
    for loop in trace(pixels):
        points = [(x * UNIT, (baseline + 1 - y) * UNIT) for x, y in loop]
        pen.moveTo(points[0])
        for point in points[1:]:
            pen.lineTo(point)
        pen.closePath()
    return pen.glyph()


def build_font(sheet, output):
    alpha = sheet.getchannel("A").load()
    empty = TTGlyphPen(None).glyph()
    glyphs = {".notdef": empty, "space": empty}
    metrics = {".notdef": (SPACE_ADVANCE * UNIT, 0), "space": (SPACE_ADVANCE * UNIT, 0)}
    cmap = {0x20: "space"}

    for baseline, top, bottom, chars in ROWS:
        spans = columns(alpha, sheet.width, top, bottom)
        expected = len(chars) + sum(EXTRA_COLUMNS.get(char, 0) for char in chars)
        if len(spans) != expected:
            raise SystemExit(f"row at y={top}: sheet has {len(spans)} columns, ROWS expects {expected}")
        index = 0
        for char in chars:
            x0, x1 = spans[index]
            index += 1
            for _ in range(EXTRA_COLUMNS.get(char, 0)):
                x1 = spans[index][1]
                index += 1
            pixels = {
                (x - x0, y)
                for x in range(x0, x1 + 1)
                for y in range(top, bottom + 1)
                if alpha[x, y]
            }
            name = UV2AGL.get(ord(char), f"uni{ord(char):04X}")
            glyphs[name] = glyph_outline(pixels, baseline)
            metrics[name] = ((x1 - x0 + 1 + SPACING) * UNIT, 0)
            cmap[ord(char)] = name

    builder = FontBuilder(UPM, isTTF=True)
    builder.setupGlyphOrder(list(glyphs))
    builder.setupCharacterMap(cmap)
    builder.setupGlyf(glyphs)
    builder.setupHorizontalMetrics(metrics)
    builder.setupHorizontalHeader(ascent=ASCENT * UNIT, descent=-DESCENT * UNIT)
    builder.setupNameTable({"familyName": "RTS Pixel", "styleName": "Regular"})
    builder.setupOS2(
        sTypoAscender=ASCENT * UNIT,
        sTypoDescender=-DESCENT * UNIT,
        usWinAscent=ASCENT * UNIT,
        usWinDescent=DESCENT * UNIT,
    )
    builder.setupPost()
    builder.save(output)
    return len(cmap)


def main():
    with tempfile.TemporaryDirectory() as folder:
        png = Path(folder) / "font.png"
        export_sheet(png)
        count = build_font(Image.open(png).convert("RGBA"), OUTPUT)
    print(f"{OUTPUT.relative_to(ROOT)}: {count} characters")


if __name__ == "__main__":
    main()
