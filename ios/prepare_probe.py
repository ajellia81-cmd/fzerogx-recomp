#!/usr/bin/env python3
"""Add metadata-only iOS input observations to a generated host harness."""
from pathlib import Path
import sys
source, destination = map(Path, sys.argv[1:])
text = source.read_text()
for old, new in [
    ('#include "runtime.h"', '#include "runtime.h"\n#include "probe_frame.h"\n#include "logo_timing.h"'),
    ('pad_set_state(0, p);', 'pad_set_state(0, p);\n        if(const char* count=getenv("FZGX_PROBE_PLAYERS"))for(int channel=1;channel<std::min(4,atoi(count));++channel)pad_set_state(channel,p);'),
    ('verts += b->verts.size();', 'ios_probe::observe(*b);\n            if(const char* end=getenv("FZGX_PROBE_END_FRAME"))if(ios_probe::frames>=strtoul(end,nullptr,10))plat_exit_now(ios_probe::finish());\n            verts += b->verts.size();'),
    ('plat_exit_now(0);', 'plat_exit_now(getenv("FZGX_LOGO_TRACE")?ios_logo::probe_finish():ios_probe::finish());'),
]:
    if text.count(old) != 1:
        raise SystemExit(f"Pinned harness changed: {old}")
    text = text.replace(old, new)
destination.write_text(text)

