"""Tag complete original modal UI calls, including text, icons and borders."""
from pathlib import Path
import sys

root=Path(sys.argv[1])
# Card dialog dispatcher, progress/status renderer, file-list compositor and
# standalone card panel (distinct from the legacy 818C24A4 manager).
# Shared message widget and its button wrapper;
# race message, confirmation, window and pause/result-menu draw functions.
targets=("818C24A4","818403D4","818408E8","8182D038",
         "8182D888","8182D524","8182EBB8","8182F294",
         "818B6B28","818B8EDC","818BC310","818BFE20",
         "819E29BC","819E2CB8","819E3A94",
         "81962164","819623A4","81A23D64","81A2CC64","81A2CEEC",
         "8184D2AC","8184D4C8","8184D10C","819F53B0","81A2D1A0",
         "81853C9C","81853E40","81853FE4")
declarations="""/* FZGX_NATIVE_CANVAS_SCOPE_V10 */
#if defined(FZGX_IOS_INPUT_PROBE)
#include <stdio.h>
#include <stdlib.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
unsigned fzgx_native_canvas_begin(unsigned owner);
void fzgx_native_canvas_end(unsigned previous);
void fzgx_ui_queue_record(unsigned address, void* cpu);
unsigned fzgx_bitmap_group_begin(void);
void fzgx_bitmap_group_end(unsigned previous);
void fzgx_bitmap_record(unsigned address);
unsigned fzgx_ui_queue_draw(unsigned address);
unsigned fzgx_ui_sprite_begin(void);
unsigned fzgx_ui_immediate_begin(void);
#if defined(FZGX_IOS_INPUT_PROBE)
void fzgx_trace_ui_text(void* cpu);
#endif
#ifdef __cplusplus
}
#endif
"""
queue_hooks={
    "8184AC20": [
        ("void fn_8184AC20(CPU* c) {", "void fn_8184AC20(CPU* c) {\n    unsigned directTag=fzgx_ui_immediate_begin();"),
        ("RET();", "fzgx_native_canvas_end(directTag); RET();")],
    "8184E724": [
        ("void fn_8184E724(CPU* c) {", "void fn_8184E724(CPU* c) {\n    unsigned directTag=fzgx_ui_immediate_begin();"),
        ("RET();", "fzgx_native_canvas_end(directTag); RET();")],
    "8184A0D8": [("void fn_8184A0D8(CPU* c) {", "void fn_8184A0D8(CPU* c) {\n#if defined(FZGX_IOS_INPUT_PROBE)\n    fzgx_trace_ui_text(c);\n#endif")],
    "8184F734": [
        ("void fn_8184F734(CPU* c) {", "void fn_8184F734(CPU* c) {\n    unsigned spriteTag=fzgx_ui_sprite_begin();"),
        ("RET();", "fzgx_native_canvas_end(spriteTag); RET();"),
        ("\n\t/* 8184FA08 ", "\n    fzgx_bitmap_record(c->r[1]+8);\n\t/* 8184FA08 "),
        ("\n\t/* 8184FAD8 ", "\n    fzgx_bitmap_record(c->r[1]+8);\n\t/* 8184FAD8 "),
        ("\n\t/* 8184FB60 ", "\n    fzgx_bitmap_record(c->r[30]); fzgx_ui_queue_record(c->r[30], c);\n\t/* 8184FB60 ")],
    "8184FC50": [
        ("void fn_8184FC50(CPU* c) {", "void fn_8184FC50(CPU* c) {\n    unsigned spriteTag=fzgx_ui_sprite_begin();"),
        ("RET();", "fzgx_native_canvas_end(spriteTag); RET();"),
        ("\n\t/* 8184FCD0 ", "\n    fzgx_ui_queue_record(c->r[31], c);\n\t/* 8184FCD0 ")],
    "8184FD64": [
        ("void fn_8184FD64(CPU* c) {", "void fn_8184FD64(CPU* c) {\n    unsigned queueTag=0;"),
        ("\n\t/* 8184FF34 ", "\n    queueTag=fzgx_ui_queue_draw(c->r[28]);\n\t/* 8184FF34 "),
        ("L_81850108:", "L_81850108:\n    fzgx_native_canvas_end(queueTag);")
    ],
}
# Native-pixel badge composition and its independently reused glow renderer.
# Scale copied sprite records, preserving anchors, UVs and original assets.
for name in ("8194108C", "81941360"):
    queue_hooks[name]=[
        ("void fn_"+name+"(CPU* c) {", "void fn_"+name+"(CPU* c) {\n    unsigned bitmapGroup=fzgx_bitmap_group_begin();"),
        ("RET();", "fzgx_bitmap_group_end(bitmapGroup); RET();")]
# A sampled Shop hot path repeatedly searches the same 44-entry category table
# from its beginning to recover an item's ordinal. Count preceding equal
# categories once instead. Ownership/unlock checks remain original guest code.
queue_hooks["818F9028"] = [
    ("void fn_818F9028(CPU* c) {", "void fn_818F9028(CPU* c) {\n    unsigned fzgxRank=~0u; int fzgxRankProof=0;"),
    ("L_818F90DC:", """L_818F90DC:
    if (c->r[29] < 44u) {
        unsigned index=c->r[29], table=c->r[28];
        unsigned category=LD16(table+10u*index+2u), ordinal=0;
        for(unsigned n=0;n<index;++n)ordinal+=LD16(table+10u*n+2u)==category;
        fzgxRank=ordinal;
#if defined(FZGX_IOS_INPUT_PROBE)
        static unsigned char verified[44]={0};
        if(!verified[index]) { verified[index]=1; fzgxRankProof=1; }
        else
#endif
        { c->r[31]=ordinal; c->r[26]=index; goto L_818F9174; }
    }
"""),
    ("L_818F9174:", """L_818F9174:
#if defined(FZGX_IOS_INPUT_PROBE)
    if(fzgxRankProof)fprintf(stderr,"Shop ordinal differential checked index=%u\\n",c->r[29]);
    if(fzgxRank!=~0u && c->r[31]!=fzgxRank) { fprintf(stderr,"Shop ordinal differential failed\\n"); abort(); }
#endif
"""),
]
# The Item ownership path repeats the same inverse ordinal search, copying
# the entire 44-entry table for every candidate. Keep its original scratch
# records and downstream ownership checks; compute only the rank once.
queue_hooks["818F965C"]=[
    ("void fn_818F965C(CPU* c) {", "void fn_818F965C(CPU* c) {\n    unsigned fzgxItemRank=~0u; int fzgxItemProof=0;"),
    ("L_818F9710:", """L_818F9710:
    if(fzgxItemRank==~0u && c->r[30]<44u){
        unsigned index=c->r[30],table=c->r[29],ordinal=0;
        int category=(int)(int16_t)LD16(table+10u*index+2u);
        for(unsigned n=0;n<index;++n)ordinal+=(int)(int16_t)LD16(table+10u*n+2u)==category;
        fzgxItemRank=ordinal;
#if defined(FZGX_IOS_INPUT_PROBE)
        static unsigned char verified[44]={0};
        if(!verified[index]){verified[index]=1;fzgxItemProof=1;}
        else
#endif
        {
            for(unsigned n=0;n<440;++n)ST8(c->r[1]+44u+n,LD8(table+n));
            for(unsigned n=0;n<10;++n)ST8(c->r[1]+8u+n,LD8(table+10u*index+n));
            c->r[24]=c->r[1]+44u+10u*index;c->r[25]=(unsigned)category;
            c->r[26]=index;c->r[27]=ordinal;c->r[28]=ordinal;c->ctr=0;
            goto L_818F97A8;
        }
    }
"""),
    ("L_818F97A8:", """L_818F97A8:
#if defined(FZGX_IOS_INPUT_PROBE)
    if(fzgxItemRank!=~0u){
        if(c->r[28]!=fzgxItemRank || c->r[26]!=c->r[30]){fprintf(stderr,"Item ordinal differential failed\\n");abort();}
        for(unsigned n=0;n<10;++n)if(LD8(c->r[1]+8u+n)!=LD8(c->r[29]+10u*c->r[30]+n)){fprintf(stderr,"Item selected record differential failed\\n");abort();}
        for(unsigned n=0;n<440;++n)if(LD8(c->r[1]+44u+n)!=LD8(c->r[29]+n)){fprintf(stderr,"Item scratch differential failed\\n");abort();}
        if(fzgxItemProof)fprintf(stderr,"Item ownership ordinal differential checked index=%u\\n",c->r[30]);
    }
#endif
""")]
found={name:0 for name in (*targets,*queue_hooks)}
for path in root.glob("*.c"):
    text=path.read_text()
    changed=False
    if "/* FZGX_NATIVE_CANVAS_SCOPE_V10 */" in text:
        for name in targets:
            if "void fn_"+name+"(CPU* c) {" in text:
                if text.count("fzgx_native_canvas_begin(0x"+name+"u)")!=1:
                    raise SystemExit("Incomplete existing native scope: "+name)
                found[name]+=1
        for name in queue_hooks:
            if "void fn_"+name+"(CPU* c) {" in text:found[name]+=1
        continue
    if any("/* FZGX_NATIVE_CANVAS_SCOPE"+v+" */" in text for v in ("", "_V2", "_V3", "_V4", "_V5", "_V6", "_V7", "_V8", "_V9")):
        raise SystemExit("Regenerate original C before upgrading native UI scopes")
    for name in targets:
        signature="void fn_"+name+"(CPU* c) {"
        if signature not in text:continue
        if text.count(signature)!=1:raise SystemExit("Ambiguous pinned UI signature: "+name)
        found[name]+=1
        start=text.index(signature)
        end=text.find("\n/* ",start)
        if end<0:end=len(text)
        body=text[start:end]
        if body.count("RET();")!=(2 if name=="81A2D1A0" else 1) or "return;" in body:
            raise SystemExit("Pinned UI return paths changed: "+name)
        body=body.replace(signature,signature+"\n    unsigned nativeCanvas=fzgx_native_canvas_begin(0x"+name+"u);",1)
        body=body.replace("RET();","fzgx_native_canvas_end(nativeCanvas); RET();")
        text=text[:start]+body+text[end:]
        changed=True
    for name,replacements in queue_hooks.items():
        signature="void fn_"+name+"(CPU* c) {"
        if signature not in text:continue
        found[name]+=1
        start=text.index(signature)
        end=text.find("\n/* ",start)
        if end<0:end=len(text)
        body=text[start:end]
        for old,new in replacements:
            if body.count(old)!=1:raise SystemExit("Pinned queued UI anchor changed: "+name)
            body=body.replace(old,new)
        text=text[:start]+body+text[end:]
        changed=True
    if changed:path.write_text(declarations+text)
if any(count!=1 for count in found.values()):raise SystemExit("Pinned UI inventory changed: "+str(found))
print("Applied ordered native UI scopes:",len(targets))


