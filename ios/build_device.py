#!/usr/bin/env python3
"""Build on the hosted Mac; retry an internal Clang abort once, serially."""
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys

build = Path(sys.argv[1] if len(sys.argv) > 1 else "build-ios")
logs = Path("build-private-logs")
logs.mkdir(exist_ok=True)

def resources(stage):
    with (logs / "compiler-resources.txt").open("a") as output:
        output.write(f"{stage}: disk={shutil.disk_usage(build)}\n")
        output.flush()
        if platform.system() == "Darwin":
            for command in (["sysctl", "-n", "hw.memsize"], ["vm_stat"]):
                subprocess.run(command, stdout=output, stderr=output, check=False)

resources("before build")
command = ["cmake", "--build", str(build), "--config", "Release",
           "--target", "fzerogx_ios", "--parallel"]
initial = logs / "ios-build.txt"
with initial.open("w") as output:
    result = subprocess.run(command + ["3"], stdout=output, stderr=subprocess.STDOUT)
if result.returncode == 0:
    sys.exit(0)

# A syntax/link error must still fail immediately. Keep the original log and
# retry only the observed compiler crash, retaining completed object files and
# exactly the same Release optimization settings. No generated C is exported.
if not re.search(r"clang: error: (?:unable to execute command: Abort trap: 6|"
                 r"clang frontend command failed due to signal)", initial.read_text()):
    sys.exit(result.returncode)
resources("after internal compiler abort")
print("Clang aborted internally; retrying once with one compile job.", flush=True)
with (logs / "ios-build-retry.txt").open("w") as output:
    result = subprocess.run(command + ["1"], stdout=output, stderr=subprocess.STDOUT)
sys.exit(result.returncode)
