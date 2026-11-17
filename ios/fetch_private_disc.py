"""Fetch the owner's shared build input without putting its access URL in logs."""
import json
import os
from pathlib import Path
from urllib.parse import quote, urlsplit
from urllib.request import Request, urlopen
from urllib.error import HTTPError, URLError
import time


class InputError(Exception):
    pass


def fetch(share_id, target, required_size=None):
    request = Request(
        "https://ckdatabasews.icloud.com/database/1/com.apple.cloudkit/production/public/records/resolve",
        data=json.dumps({"shortGUIDs": [{"value": share_id}]}).encode(),
        headers={"Content-Type": "application/json"},
    )
    with urlopen(request, timeout=60) as response:
        result = json.load(response)["results"][0]
    value = result["rootRecord"]["fields"]["fileContent"]["value"]
    url = value["downloadURL"].replace("${f}", quote(target.name))
    parsed = urlsplit(url)
    if parsed.scheme != "https" or not parsed.hostname.endswith(".icloud-content.com"):
        raise InputError("Unexpected download host")
    expected = int(value["size"])
    if required_size is not None and expected != required_size:
        raise InputError(f"Memory card size {expected} differs from {required_size}")
    target.parent.mkdir(exist_ok=True)
    temporary = target.with_suffix(".partial")
    total = 0
    with urlopen(url, timeout=120) as response, temporary.open("wb") as out:
        while block := response.read(8 * 1024 * 1024):
            out.write(block)
            total += len(block)
    if total != expected:
        raise InputError("Incomplete disc download")
    temporary.rename(target)
    print(f"Private build input received: {target.name} ({total} bytes)")


def fetch_with_retry(*args, **kwargs):
    for attempt in range(3):
        try:
            return fetch(*args, **kwargs)
        except HTTPError as error:
            if error.code not in (408, 429, 500, 502, 503, 504) or attempt == 2:
                raise
        except (URLError, TimeoutError):
            if attempt == 2:
                raise
        print(f"Transient private input transfer failure; retry {attempt + 1}/2")
        time.sleep(3 * (attempt + 1))


def main():
    fetch_with_retry(os.environ["ICLOUD_SOURCE_ID"], Path("rom/game.iso"))
    save_id = os.environ.get("ICLOUD_SAVE_ID")
    if save_id:
        fetch_with_retry(save_id, Path("rom/memcard_a.raw"), required_size=524288)


if __name__ == "__main__":
    try:
        main()
    except InputError as error:
        print(f"Private build input rejected: {error}")
        raise SystemExit(1)
    except Exception as error:
        # URL-bearing exceptions must not expose a signed link in public logs.
        print(f"Private build input download failed ({type(error).__name__}, status={getattr(error, 'code', 'unavailable')})")
        raise SystemExit(1)
