#!/usr/bin/env python3
"""Add metadata-only iOS input observations to a generated host harness."""
from pathlib import Path
import sys
source, destination = map(Path, sys.argv[1:])
text = source.read_text()
for old, new in [
    ('#include "runtime.h"', '#include "runtime.h"\n#include "probe_frame.h"'),
    ('verts += b->verts.size();', 'ios_probe::observe(*b);\n            verts += b->verts.size();'),
    ('plat_exit_now(0);', 'plat_exit_now(ios_probe::finish());'),
]:
    if text.count(old) != 1:
        raise SystemExit(f"Pinned harness changed: {old}")
    text = text.replace(old, new)
destination.write_text(text)
