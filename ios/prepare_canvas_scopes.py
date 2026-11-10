"""Tag the complete common memory-card UI draw in generated C."""
from pathlib import Path
import sys

root=Path(sys.argv[1])
signature="void fn_818C24A4(CPU* c) {"
declarations="""/* FZGX_NATIVE_CANVAS_SCOPE */
#ifdef __cplusplus
extern "C" {
#endif
unsigned fzgx_native_canvas_begin(void);
void fzgx_native_canvas_end(unsigned previous);
#ifdef __cplusplus
}
#endif
"""
found=[]
for path in root.glob("*.c"):
    text=path.read_text()
    if signature not in text:continue
    found.append(path)
    if "/* FZGX_NATIVE_CANVAS_SCOPE */" in text:continue
    if text.count(signature)!=1:raise SystemExit("Common card draw signature is ambiguous")
    start=text.index(signature)
    end=text.find("\n/* ",start)
    if end<0:end=len(text)
    body=text[start:end]
    if body.count("RET();")!=1 or "return;" in body:
        raise SystemExit("Pinned common card draw return paths changed")
    body=body.replace(signature,signature+"\n    unsigned nativeCanvas=fzgx_native_canvas_begin();",1)
    body=body.replace("RET();","fzgx_native_canvas_end(nativeCanvas); RET();")
    path.write_text(declarations+text[:start]+body+text[end:])
if len(found)!=1:raise SystemExit("Pinned common card draw function changed")
print("Applied ordered common memory-card canvas scope")
