"""Owner-only inspection of already-generated C; never publish plaintext.

The game loads its own main module through the existing build. This script does
not decode disc data and does not copy ROMs, executable images or game assets.
"""
from pathlib import Path
import io, json, re, struct, sys, tarfile
root, output = map(Path, sys.argv[1:])
files=[]
for p in root.rglob('*.c'):
    if 'bootstrap' in p.parts:continue
    if re.search(r'void\s+fn_[A-Za-z0-9_]+\s*\(CPU\*',p.read_text(errors='replace')):files.append(p)
if not files:raise SystemExit('No generated code for inspection')
with tarfile.open(output,'w:gz') as t:
    for p in sorted(files):t.add(p,arcname=str(p.relative_to(root)),recursive=False)
    # Private address-indexed strings permit static call-site tracing without
    # exposing disc inputs or binary module images in public logs.
    for rel in sorted((root / "rels-runtime").glob("*.rel")):
        data=rel.read_bytes()
        count,table=struct.unpack_from(">II",data,12)
        strings=[]
        for section in range(count):
            offset,size=struct.unpack_from(">II",data,table+8*section)
            offset &= ~3
            if not offset or not size:continue
            for match in re.finditer(rb"[ -~]{5,}\x00",data[offset:offset+size]):
                strings.append({"section":section,"offset":match.start(),"text":match.group()[:-1].decode("ascii")})
        imports,import_size=struct.unpack_from(">II",data,40)
        relocations=[]
        for at in range(imports,imports+import_size,8):
            module,position=struct.unpack_from(">II",data,at)
            section=0
            source=0
            while position+8<=len(data):
                delta,kind,target,addend=struct.unpack_from(">HBBI",data,position)
                position+=8
                if kind==203:break
                if kind==202:
                    section=target
                    source=0
                    continue
                source+=delta
                if kind not in (0,201):
                    relocations.append([section,source,module,target,addend,kind])
        raw=json.dumps({"strings":strings,"relocations":relocations}).encode()
        entry=tarfile.TarInfo("strings/"+rel.name+".json")
        entry.size=len(raw)
        t.addfile(entry,io.BytesIO(raw))
print('Prepared private function corpus; encrypt before upload. Files:',len(files))
