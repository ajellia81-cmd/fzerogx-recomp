#!/usr/bin/env python3
"""Check game-compensated UI and decoded-video metadata policies, without a ROM."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="fzgx-render-policy-") as tmp:
    tmp = Path(tmp)
    subprocess.run([sys.executable, str(root / "ios/prepare_renderer.py"),
                    str(root / "gcn-recomp/runtime/gx/render_gl.cpp"),
                    str(tmp / "render_ios.cpp")], check=True)
    runtime = root / "gcn-recomp/runtime"
    cmd = [os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-DGCN_GL_ES",
           "-DGCN_GL_ES30", "-ffunction-sections", "-fdata-sections"]
    for include in [root / "ios", runtime, runtime / "gx", runtime / "gx/glad_es/include", tmp]:
        cmd += ["-I" + str(include)]
    cmd += [str(root / "ios/tests/render_policy.cpp"),
            str(runtime / "gx/glad_es/src/gles2.c"), str(runtime / "hw/vi.cpp"), str(runtime / "gx/shadergen.cpp"),
            "-Wl,-dead_strip" if sys.platform == "darwin" else "-Wl,--gc-sections",
            "-pthread", "-o", str(tmp / "check")]
    subprocess.run(cmd, check=True)
    subprocess.run([str(tmp / "check")], check=True)

    session = [os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-pthread", "-I"+str(root/"ios"), "-I"+str(runtime), str(root/"ios/tests/session_policy.cpp"), "-o", str(tmp/"session")]
    subprocess.run(session, check=True)
    subprocess.run([str(tmp/"session")], check=True, timeout=15)
