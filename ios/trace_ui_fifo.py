"""Private host-only trace of the ordered tag and pixel-state boundaries."""
from pathlib import Path
patches=[
("gcn-recomp/runtime/gx/fifo.cpp","void gx::set_draw_tag(uint8_t tag) {",
 'void gx::set_draw_tag(uint8_t tag) {\n    if(tag&64){static unsigned n[256]={};if(n[tag]++<4)fprintf(stderr,"UI FIFO tag=%u control=%u pointer=%08x pending=%u\\\n",unsigned(tag),unsigned(g_cp_cr),unsigned(pi_fifo_wptr()),unsigned(g_gp_len));}'),
("gcn-recomp/runtime/gx/xf.cpp","void draw_tag_reached(uint8_t tag) {",
 'void draw_tag_reached(uint8_t tag) {\n    if(tag&64){static unsigned n[256]={};if(n[tag]++<3)fprintf(stderr,"UI tag reached XF: %u\\n",unsigned(tag));}'),
("gcn-recomp/runtime/gx/xf.cpp","    st.tag = g_draw_tag;",
 '    st.tag = g_draw_tag;\n    if(st.tag&64){static unsigned n=0;if(n++<12)fprintf(stderr,"UI pixel state tagged: %u\\n",unsigned(st.tag));}')
]
for filename,old,new in patches:
 p=Path(filename);s=p.read_text()
 if s.count(old)!=1:raise SystemExit("Pinned XF trace anchor changed")
 p.write_text(s.replace(old,new))
