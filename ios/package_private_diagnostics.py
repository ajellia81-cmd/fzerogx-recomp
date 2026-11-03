#!/usr/bin/env python3
"""Select only logs and opt-in renderer captures for subsequent encryption."""
from pathlib import Path
import tarfile

paths = sorted(Path("build-private-logs").glob("*.txt"))
for directory in ("build-probe", "build-probe-race", "build-probe-story", "build-probe-menu-replay", "build-probe-menu-profile", "build-probe-menu-customize", "build-probe-menu-shop"):
    paths += sorted(Path(directory).glob("frame-*.capture"))
with tarfile.open("private-logs.tar.gz", "w:gz") as archive:
    for path in paths:
        if path.is_file():
            archive.add(path, arcname=str(path), recursive=False)
print(f"Selected {len(paths)} diagnostic files; encrypt before upload")
