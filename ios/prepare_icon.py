#!/usr/bin/env python3
"""Reproduce the user-approved cover crop as iOS app-icon assets."""
import argparse
import hashlib
import io
import json
from pathlib import Path
from PIL import Image

# User-supplied cover artwork, approved as app-icon art; no disc assets are used.
LOGO_SHA256 = "ae0e1ef0eea6d3b255821f39df6bc3b339dcc4e9d6bf26229da8ca061722feb9"
PREVIEW_SHA256 = "bd2fe02cdf39f41f5f027bb2de447f5be6eeb47567305d11bb336bccb503e7aa"
PREVIEW_PIXELS_SHA256 = "71f58447bfce67282d252a202662b72381a7c05cee4ee948e5aa4b46f2ae909a"

def make_icon(logo):
    if logo.size != (580, 817):
        raise SystemExit("Unexpected source-cover dimensions")
    return logo.convert("RGB").crop((0, 104, 580, 684)).resize((1024, 1024), Image.Resampling.LANCZOS)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("destination", type=Path)
    parser.add_argument("--approved-sha256", required=True)
    parser.add_argument("--logo-file", type=Path)
    args = parser.parse_args()
    if args.approved_sha256 != PREVIEW_SHA256:
        raise SystemExit("The reviewed icon must be explicitly approved before packaging")
    source = args.logo_file or Path(__file__).with_name("icon-cover.webp")
    data = source.read_bytes()
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
