"""Owner-only inspection of already-generated C; never publish plaintext.

The game loads its own main module through the existing build. This script does
not decode disc data and does not copy ROMs, executable images or game assets.
"""
from pathlib import Path
import re, sys, tarfile
root, output = map(Path, sys.argv[1:])
files=[]
for p in root.rglob('*.c'):
    if 'bootstrap' in p.parts:continue
    if re.search(r'void\s+fn_[A-Za-z0-9_]+\s*\(CPU\*',p.read_text(errors='replace')):files.append(p)
if not files:raise SystemExit('No generated code for inspection')
with tarfile.open(output,'w:gz') as t:
    for p in sorted(files):t.add(p,arcname=str(p.relative_to(root)),recursive=False)
print('Prepared private function corpus; encrypt before upload. Files:',len(files))
