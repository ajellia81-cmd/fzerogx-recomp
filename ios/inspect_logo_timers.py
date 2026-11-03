"""Select private generated timer functions for encrypted owner-only review.

Never print their contents or include the plaintext output in an artifact.
This does not decode game data; it reads code already emitted by the recompiler.
"""
from pathlib import Path
import re,sys
root,output=map(Path,sys.argv[1:])
selected=[]
for path in root.rglob('*.c'):
    if '/bootstrap/' in str(path):continue
    source=path.read_text(errors='replace')
    for match in re.finditer(r'(?:void|static void)\s+(fn_[A-Za-z0-9_]+)\s*\(CPU\*\s*c\)\s*\{',source):
        start=match.end();end=start;depth=1
        while depth and end<len(source):
            depth+=(source[end]=='{')-(source[end]=='}');end+=1
        body=source[match.start():end]
        if match[1]=='fn_80005A08' or ('fn_80005E0C(c)' in body):
            selected.append(str(path)+'\n'+body)
if not selected:raise SystemExit('No timer callers found; nothing exported')
output.write_text('\n\n'.join(selected))
print('Selected private timer functions:',len(selected))
