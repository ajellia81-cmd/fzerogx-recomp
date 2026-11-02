#!/usr/bin/env python3
"""Reproduce the reviewed icon without storing game artwork in public git."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import urllib.request
from PIL import Image

LOGO_URL = "https://upload.wikimedia.org/wikipedia/fr/1/18/F-Zero_GX_Logo.jpg"
LOGO_SHA256 = "4b3e409b64a4b14e2b38d7c705f4cccc93fe1a8c26448fc69bdaaa9b6bcbefd7"
PREVIEW_SHA256 = "46c2d3478e614d0021d645e31cee5c09c4cb7349d81872d1e55e15c53f1f1ab2"

PREVIEW_PIXELS_SHA256 = "2d9332c6b22db6b75e4aa1a46588b04b6b9528c4e13af79872f0af585143d217"

def make_icon(logo):
    logo = logo.convert("RGB")
    logo = logo.resize((920, round(logo.height * 920 / logo.width)), Image.Resampling.LANCZOS)
    icon = Image.new("RGB", (1024, 1024), "white")
    icon.paste(logo, ((1024 - logo.width) // 2, (1024 - logo.height) // 2))
    return icon

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("destination", type=Path)
    parser.add_argument("--approved-sha256", required=True)
    parser.add_argument("--logo-file", type=Path)
    args = parser.parse_args()
    if args.approved_sha256 != PREVIEW_SHA256:
        raise SystemExit("The reviewed icon must be explicitly approved before packaging")
    if args.logo_file:
        data = args.logo_file.read_bytes()
    else:
        request = urllib.request.Request(LOGO_URL, headers={"User-Agent": "FZeroGX-iOS-icon-build/1.0"})
        with urllib.request.urlopen(request, timeout=60) as response:
            data = response.read(1024 * 1024)
    if hashlib.sha256(data).hexdigest() != LOGO_SHA256:
        raise SystemExit("Logo source changed; show a new preview before building")
    icon = make_icon(Image.open(io.BytesIO(data)))
    if hashlib.sha256(icon.tobytes()).hexdigest() != PREVIEW_PIXELS_SHA256:
        raise SystemExit("Icon reproduction differs from the approved preview")
    asset = args.destination / "AppIcon.appiconset"
    asset.mkdir(parents=True, exist_ok=True)
    entries = []
    for idiom, slots in {"iphone": [(20,2),(20,3),(29,2),(29,3),(40,2),(40,3),(60,2),(60,3)],
                         "ipad": [(20,1),(20,2),(29,1),(29,2),(40,1),(40,2),(76,1),(76,2),(83.5,2)]}.items():
        for size, scale in slots:
            filename = f"{idiom}-{size}-{scale}.png"
            pixels = round(size * scale)
            icon.resize((pixels,pixels), Image.Resampling.LANCZOS).save(asset / filename)
            entries.append({"idiom":idiom,"size":f"{size}x{size}","scale":f"{scale}x","filename":filename})
    icon.save(asset / "AppIcon-1024.png")
    entries.append({"idiom":"ios-marketing","size":"1024x1024","scale":"1x","filename":"AppIcon-1024.png"})
    (asset / "Contents.json").write_text(json.dumps({"images":entries,"info":{"version":1,"author":"xcode"}}))
    (args.destination / "Contents.json").write_text(json.dumps({"info":{"version":1,"author":"xcode"}}))

if __name__ == "__main__":
    main()
