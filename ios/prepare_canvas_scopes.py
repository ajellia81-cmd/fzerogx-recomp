"""Tag complete original modal UI calls, including text, icons and borders."""
from pathlib import Path
import sys

root=Path(sys.argv[1])
# Common memory-card draw; shared message widget and its button wrapper;
# race message, confirmation, window and pause/result-menu draw functions.
targets=("818C24A4","818403D4","818408E8","8182D038",
         "8182D888","8182D524","8182EBB8","8182F294")
declarations="""/* FZGX_NATIVE_CANVAS_SCOPE_V3 */
#ifdef __cplusplus
extern "C" {
#endif
unsigned fzgx_native_canvas_begin(unsigned owner);
void fzgx_native_canvas_end(unsigned previous);
void fzgx_ui_queue_record(unsigned address, void* cpu);
unsigned fzgx_ui_queue_draw(unsigned address);
#ifdef __cplusplus
}
#endif
"""
queue_hooks={
    "8184F734": [("\n\t/* 8184FB60 ", "\n    fzgx_ui_queue_record(c->r[30], c);\n\t/* 8184FB60 ")],
    "8184FC50": [("\n\t/* 8184FCD0 ", "\n    fzgx_ui_queue_record(c->r[31], c);\n\t/* 8184FCD0 ")],
    "8184FD64": [
        ("void fn_8184FD64(CPU* c) {", "void fn_8184FD64(CPU* c) {\n    unsigned queueTag=0;"),
        ("\n\t/* 8184FF34 ", "\n    queueTag=fzgx_ui_queue_draw(c->r[28]);\n\t/* 8184FF34 "),
        ("L_81850108:", "L_81850108:\n    fzgx_native_canvas_end(queueTag);")
    ],
}
found={name:0 for name in (*targets,*queue_hooks)}
for path in root.glob("*.c"):
    text=path.read_text()
    changed=False
    if "/* FZGX_NATIVE_CANVAS_SCOPE_V3 */" in text:
        for name in targets:
            if "void fn_"+name+"(CPU* c) {" in text:
                if text.count("fzgx_native_canvas_begin(0x"+name+"u)")!=1:
                    raise SystemExit("Incomplete existing native scope: "+name)
                found[name]+=1
        for name in queue_hooks:
            if "void fn_"+name+"(CPU* c) {" in text:found[name]+=1
        continue
    if "/* FZGX_NATIVE_CANVAS_SCOPE */" in text or "/* FZGX_NATIVE_CANVAS_SCOPE_V2 */" in text:
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
        if body.count("RET();")!=1 or "return;" in body:
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
