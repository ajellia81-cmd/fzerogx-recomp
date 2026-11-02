#!/usr/bin/env python3
"""Generate an iOS-only renderer variant from the pinned upstream source."""
from pathlib import Path
import sys

source, destination = map(Path, sys.argv[1:])
text = source.read_text()

def replace(old, new):
    global text
    if text.count(old) != 1:
        raise SystemExit(f"Pinned renderer changed: expected one occurrence of {old[:90]!r}")
    text = text.replace(old, new)

replace('#include "gl.h"', '#include "gl.h"\n#include "motion_history.h"\n#include <cmath>')
replace('static int g_scale = 2;', 'static int g_scale = 2;\nstatic bool g_ios_transient=false, g_ios_prepared=false, g_ios_wide_view=false;')
replace('int vw = w, vh = w * 3 / 4;\n    if (vh > h) { vh = h; vw = h * 4 / 3; }', 'int vw=w, vh=h; if(!g_ios_wide_view){ vh=w*3/4; if(vh>h){vh=h;vw=h*4/3;} }')
replace('if (c.writeback) copy_writeback(c);', 'if (c.writeback && !g_ios_transient) copy_writeback(c);')
replace('if (!g_pair_second) g_present_count++;', 'if (!g_pair_second && !g_ios_transient) g_present_count++;')
replace('''        g_render_frame++;
        const size_t n_dead = g_dead_pending.size();''', '''        if (!g_ios_prepared) {
        g_render_frame++;
        const size_t n_dead = g_dead_pending.size();''')
replace('''        // A set the GPU has finished with; see g_vaos.''', '''        }
        // A set the GPU has finished with; see g_vaos.''')
replace('}  // namespace gx', '#include "render_extensions.inc"\n\n}  // namespace gx')
destination.write_text(text)
