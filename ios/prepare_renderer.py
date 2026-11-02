#!/usr/bin/env python3
"""Generate an iOS-only renderer variant from the pinned upstream source."""
from pathlib import Path
import sys
import re

source, destination = map(Path, sys.argv[1:])
text = source.read_text()

def replace(old, new):
    global text
    if text.count(old) != 1:
        raise SystemExit(f"Pinned renderer changed: expected one occurrence of {old[:90]!r}")
    text = text.replace(old, new)

replace('#include "gl.h"', '#include "gl.h"\n#include "motion_history.h"\n#include "scene_classify.h"\n#include "scene_context.h"\n#include "ui_policy.h"\n#include <cmath>')
# Compile shipped shader keys before gameplay even when ANGLE has no program binaries.
replace('if (g_shipped_keys && g_binaries_supported) {', 'if (g_shipped_keys) {')
# The shipped desktop list contains dual-source destination-alpha variants.
# ANGLE without that extension draws the fallback key (dst_alpha=0). Normalize
# both disk records and shipped keys BEFORE deduplication and compilation.
replace('static void shader_cache_open() {', 'static void ios_shader_key(ShaderKey& k){if(!g_dual_source && k.dst_alpha==2)k.dst_alpha=0;canonicalize_shader_key(k); }\n\nstatic void shader_cache_open() {')
if text.count('canonicalize_shader_key(r.k);')!=2:
    raise SystemExit('Pinned shader cache record normalization changed')
text=text.replace('canonicalize_shader_key(r.k);','ios_shader_key(r.k);')
replace('canonicalize_shader_key(k);\n                if (have.insert(k).second)', 'ios_shader_key(k);\n                if (have.insert(k).second)')
replace('static int g_scale = 2;', 'static int g_scale = 2;\nstatic bool g_ios_transient=false, g_ios_prepared=false, g_ios_wide_view=false;\nstatic float g_ios_fixed_aspect=16.f/9;\nstatic float g_ios_scale_x=1, g_ios_scale_y=1;\nstatic int ios_px_x(float v){return int(std::lround(v*g_ios_scale_x));}\nstatic int ios_px_y(float v){return int(std::lround(v*g_ios_scale_y));}')
replace('int vw = w, vh = w * 3 / 4;\n    if (vh > h) { vh = h; vw = h * 4 / 3; }', 'int vw=w, vh=h; if(!g_ios_wide_view){ vh=int(w/g_ios_fixed_aspect); if(vh>h){vh=h;vw=int(h*g_ios_fixed_aspect);} }')
replace('if (c.writeback) copy_writeback(c);', 'if (c.writeback && !g_ios_transient) copy_writeback(c);')
replace('if (!g_pair_second) g_present_count++;', 'if (!g_pair_second && !g_ios_transient) g_present_count++;')
replace('''        g_render_frame++;
        const size_t n_dead = g_dead_pending.size();''', '''        if (!g_ios_prepared) {
        g_render_frame++;
        const size_t n_dead = g_dead_pending.size();''')
replace('''        // A set the GPU has finished with; see g_vaos.''', '''        }
        // A set the GPU has finished with; see g_vaos.''')
# iOS uses a fixed native drawable instead of an integer EFB scale. Keep the
# console's logical coordinates for projection/texture addressing and map X/Y
# storage coordinates independently to physical pixels.
a = text.index('void render_set_internal_scale(int scale) {')
z = text.index('\n// ---------------------------------------------------------------------------', a)
text = text[:a] + 'void render_set_internal_scale(int) {}\n' + text[z:]
replace('const uint32_t fmt = c.format & 0xF, w = c.dst_w, h = c.dst_h, s = (uint32_t)g_scale;', 'const uint32_t fmt = c.format & 0xF, w = c.dst_w, h = c.dst_h;\n    const uint32_t pw=ios_px_x(w), ph=ios_px_y(h);')
replace('std::vector<uint8_t> px((size_t)w * s * h * s * 4);', 'std::vector<uint8_t> px((size_t)pw * ph * 4);')
replace('glReadPixels(0, 0, (GLsizei)(w * s), (GLsizei)(h * s), GL_RGBA, GL_UNSIGNED_BYTE, px.data());', 'glReadPixels(0, 0, pw, ph, GL_RGBA, GL_UNSIGNED_BYTE, px.data());')
replace('return &px[(((size_t)y * s + s / 2) * w * s + (size_t)x * s + s / 2) * 4];', 'return &px[(size_t(std::min(int(ph)-1,int(std::floor((y+.5f)*g_ios_scale_y)))) * pw + std::min(int(pw)-1,int(std::floor((x+.5f)*g_ios_scale_x)))) * 4];')
for expression, axis in [
    ('EFB_W', 'x'), ('EFB_H', 'y'),
    ('t.w', 'x'), ('t.h', 'y'), ('c.dst_w', 'x'), ('c.dst_h', 'y'),
    ('c.src_w', 'x'), ('c.src_h', 'y'), ('c.src_x', 'x'),
    ('w', 'x'), ('h', 'y'), ('x.h', 'y'),
    ('(EFB_H - y1)', 'y'), ('(x1 - x0)', 'x'), ('(y1 - y0)', 'y'), ('x0', 'x'),
    ('(EFB_H - c.src_y - c.src_h)', 'y'), ('(c.src_x + c.src_w)', 'x'), ('(EFB_H - c.src_y)', 'y'),
    ('(h - row0 - rows)', 'y'), ('(h - row0)', 'y'),
]:
    text = re.sub(r'(?<![\w.])'+re.escape(expression)+r' \* g_scale\b', 'ios_px_'+axis+'('+expression+')', text)
# Raster-space depth planes and point size use physical target pixels too.
text = text.replace('(px / cw + 1.0) * 0.5 * ios_px_x(EFB_W)', '(px / cw + 1.0) * 0.5 * ios_px_x(EFB_W)')
text = text.replace('/ 6.0f * g_scale', '/ 6.0f * g_ios_scale_y')
replace('}  // namespace gx', '#include "render_extensions.inc"\n\n}  // namespace gx')
destination.write_text(text)
