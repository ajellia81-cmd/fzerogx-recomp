"""Use the pinned upstream ANGLE build with public-SDK annotation compatibility."""
from pathlib import Path
import subprocess
import sys

source = Path("gcn-recomp/tools/build-angle-visionos.sh").read_text()
needle = 'SYMROOT="$BUILD" CODE_SIGNING_ALLOWED=NO -quiet build'
if source.count(needle) != 1:
    raise SystemExit("Upstream ANGLE build invocation changed; review compatibility flags")
# Public libc++ does not mark std::mutex with Clang capability annotations.
# These two static-analysis diagnostics do not change the actual lock/unlock
# calls. Keep the rest of WebKit's warnings-as-errors policy intact.
flags = "OTHER_CPLUSPLUSFLAGS='$(inherited) -Wno-error=thread-safety-attributes -Wno-error=thread-safety-analysis'"
source = source.replace(needle, f'SYMROOT="$BUILD" CODE_SIGNING_ALLOWED=NO {flags} -quiet build')
script = Path("build-ios/build-angle-compatible.sh")
script.parent.mkdir(exist_ok=True)
script.write_text(source)
subprocess.run(["sh", str(script), str(Path(sys.argv[1]).resolve()), "iphoneos"], check=True)
