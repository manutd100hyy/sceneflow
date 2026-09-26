#!/usr/bin/env python3
"""Fail if a Qt .qrc entry or assets/ filename is not plain ASCII.

Windows MinGW make cannot build qrc rules for non-ASCII names, spaces, or
characters such as '~' and '@'. Allowed characters are A-Z, a-z, 0-9, and
._-/ 
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SAFE = re.compile(r"^[A-Za-z0-9._/-]+$")


def check_path(label, rel):
    if not SAFE.fullmatch(rel) or rel.startswith("/") or ".." in rel.split("/"):
        return "%s: unsafe resource path %r" % (label, rel)
    return None


def main():
    errors = []
    qrc_files = sorted(ROOT.rglob("*.qrc"))
    qrc_files = [p for p in qrc_files if ".git" not in p.parts]
    if len(qrc_files) < 2:
        errors.append("expected qml and menuicons qrc files, found %d" % len(qrc_files))
    entries = 0
    for qrc in qrc_files:
        text = qrc.read_text(encoding="utf-8")
        found = re.findall(r"<file>([^<]+)</file>", text)
        if not found:
            errors.append("%s: no <file> entries" % qrc)
        for rel in found:
            rel = rel.strip()
            entries += 1
            problem = check_path(str(qrc.relative_to(ROOT)), rel)
            if problem:
                errors.append(problem)
                continue
            target = qrc.parent / rel
            if not target.is_file():
                errors.append("%s: missing %s" % (qrc.relative_to(ROOT), rel))
    assets = ROOT / "assets"
    asset_count = 0
    for path in assets.rglob("*"):
        if not path.is_file():
            continue
        asset_count += 1
        rel = path.relative_to(assets).as_posix()
        problem = check_path("assets", rel)
        if problem:
            errors.append(problem)
    if entries < 100 or asset_count < 100:
        errors.append("too few files: qrc=%d assets=%d" % (entries, asset_count))
    if errors:
        sys.stderr.write("\n".join(errors) + "\n")
        return 1
    print("ok: %d qrc entries, %d asset files" % (entries, asset_count))
    return 0


if __name__ == "__main__":
    sys.exit(main())
