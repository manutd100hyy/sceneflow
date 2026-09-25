#!/usr/bin/env python3
"""Trace official form scans into SVG paths (potrace).

The printed captions and table rules stay in the same 1536x2048 design
space as the PNG. Filled values are still drawn as real text on top.
"""
import subprocess
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
FORMS = ROOT / "data" / "forms"
THRESHOLD = 190


def trace(png: Path) -> None:
    image = Image.open(png).convert("L")
    bitmap = image.point(lambda p: 0 if p < THRESHOLD else 255, mode="1")
    pbm = png.with_suffix(".pbm")
    bitmap.save(pbm)
    svg = png.with_suffix(".svg")
    subprocess.check_call(
        ["potrace", "-b", "svg", "-t", "12", "-u", "1", "-o", str(svg), str(pbm)]
    )
    text = svg.read_text(encoding="utf-8")
    start = text.find("<svg")
    if start < 0:
        raise SystemExit("potrace did not write an svg root: %s" % svg)
    svg.write_text('<?xml version="1.0" encoding="UTF-8"?>\n' + text[start:], encoding="utf-8")
    pbm.unlink()
    print("%s -> %s (%d bytes)" % (png.name, svg.name, svg.stat().st_size))


def main() -> None:
    pages = sorted(FORMS.glob("*.png"))
    if not pages:
        raise SystemExit("no form pngs in %s" % FORMS)
    for png in pages:
        trace(png)


if __name__ == "__main__":
    sys.exit(main())
