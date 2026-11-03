#!/usr/bin/env python3
"""Select an iOS-only boot-logo delay hook; keep desktop recomp tables intact."""
from pathlib import Path
import shutil,sys
source,destination=map(Path,sys.argv[1:])
shutil.copytree(source,destination,dirs_exist_ok=True)
p=destination/'patches.txt';text=p.read_text();old='80005E0C RET();'
if text.count(old)!=1:raise SystemExit('Pinned logo-delay patch changed')
p.write_text(text.replace(old,'80005E0C { extern void fzgx_ios_logo_delay(CPU*); fzgx_ios_logo_delay(c); } RET();'))
