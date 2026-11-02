"""Package only an iPhoneOS app; never include its disc or generated source."""
from pathlib import Path
import subprocess
import sys
import zipfile

app = Path(sys.argv[1])
out = Path(sys.argv[2])
if not app.is_dir() or not (app / "FZeroGX").is_file():
    raise SystemExit("Missing complete app bundle")
for path in app.rglob("*"):
    if path.suffix.lower() in {".iso", ".ciso", ".dol", ".rel", ".rvz", ".c", ".cpp", ".mm", ".capture", ".raw", ".gci"}:
        raise SystemExit("Unexpected game input or source in app bundle")
binary = app / "FZeroGX"
arch = subprocess.check_output(["lipo", "-archs", str(binary)], text=True).strip()
if arch != "arm64":
    raise SystemExit(f"Expected device arm64, got {arch}")
load = subprocess.check_output(["xcrun", "vtool", "-show-build", str(binary)], text=True)
if "platform IOS\n" not in load:
    raise SystemExit("Executable is not an iPhoneOS build")
for path in [binary, *app.rglob("*.dylib")]:
    subprocess.run(["codesign", "--remove-signature", str(path)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as archive:
    for path in sorted(app.rglob("*")):
        if path.is_file() and "_CodeSignature" not in path.parts and path.name != "embedded.mobileprovision":
            archive.write(path, Path("Payload") / app.name / path.relative_to(app))
print(f"Unsigned device IPA packaged: {out.stat().st_size} bytes; no disc assets")
