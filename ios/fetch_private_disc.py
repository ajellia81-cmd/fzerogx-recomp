"""Fetch the owner's shared build input without putting its access URL in logs."""
import json
import os
from pathlib import Path
from urllib.parse import quote, urlsplit
from urllib.request import Request, urlopen


def main():
    share_id = os.environ["ICLOUD_SOURCE_ID"]
    request = Request(
        "https://ckdatabasews.icloud.com/database/1/com.apple.cloudkit/production/public/records/resolve",
        data=json.dumps({"shortGUIDs": [{"value": share_id}]}).encode(),
        headers={"Content-Type": "application/json"},
    )
    with urlopen(request, timeout=60) as response:
        result = json.load(response)["results"][0]
    value = result["rootRecord"]["fields"]["fileContent"]["value"]
    url = value["downloadURL"].replace("${f}", quote("game.iso"))
    parsed = urlsplit(url)
    if parsed.scheme != "https" or not parsed.hostname.endswith(".icloud-content.com"):
        raise RuntimeError("Unexpected download host")
    expected = int(value["size"])
    target = Path("rom/game.iso")
    target.parent.mkdir(exist_ok=True)
    temporary = target.with_suffix(".partial")
    total = 0
    with urlopen(url, timeout=120) as response, temporary.open("wb") as out:
        while block := response.read(8 * 1024 * 1024):
            out.write(block)
            total += len(block)
    if total != expected:
        raise RuntimeError("Incomplete disc download")
    temporary.rename(target)
    print(f"User-supplied disc received: {total} bytes")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        # URL-bearing exceptions must not expose a signed link in public logs.
        print(f"Private disc download failed ({type(error).__name__})")
        raise SystemExit(1)
