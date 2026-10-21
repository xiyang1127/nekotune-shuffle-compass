"""Exercise the installable ZIP against a real Linux host in an isolated profile."""
from __future__ import annotations

import argparse
import array
import json
import math
import os
from pathlib import Path
import socket
import subprocess
import tempfile
import time
import wave


def smoke(host: Path, package: Path) -> None:
    with tempfile.TemporaryDirectory(prefix="nekotune-compass-host-") as temporary:
        root = Path(temporary)
        address = str(root / "backend.sock")
        env = dict(os.environ, NEKOTUNE_HOME=str(root / "profile"), NEKOTUNE_SOCKET=address)
        with (root / "backend.log").open("w+") as log:
            process = subprocess.Popen([str(host.resolve())], env=env, stdout=log, stderr=log)
            try:
                deadline = time.monotonic() + 10
                while not Path(address).exists():
                    if process.poll() is not None or time.monotonic() >= deadline:
                        log.seek(0)
                        raise RuntimeError("Host did not start:\n" + log.read())
                    time.sleep(0.025)
                with socket.socket(socket.AF_UNIX) as peer:
                    peer.settimeout(10)
                    peer.connect(address)
                    channel = peer.makefile("rwb", buffering=0)
                    next_id = 0

                    def call(method: str, params: dict | None = None) -> dict:
                        nonlocal next_id
                        next_id += 1
                        channel.write(json.dumps({"id": next_id, "method": method, "params": params or {}}).encode() + b"\n")
                        while True:
                            line = channel.readline()
                            if not line:
                                raise RuntimeError("Host disconnected")
                            reply = json.loads(line)
                            if reply.get("id") == next_id:
                                if reply.get("status") != "ok":
                                    raise RuntimeError(f"{method}: {reply}")
                                return reply["data"]

                    owner = "nekotune.shuffle-compass"
                    strategy = owner + "/compass"

                    def service(name: str, params: dict | None = None) -> dict:
                        return call("extensions.call", {"id": owner, "service": name, "params": params or {}})

                    call("extensions.install", {"path": str(package.resolve())})
                    call("extensions.enable", {"id": owner, "trusted": True})
                    assert len(call("player.shuffle_strategies")["shuffle_strategies"]) == 2
                    assert service("settings") == {"frequency": 0, "variety": 0}
                    saved = service("configure", {"frequency": -0.8, "variety": 0.65})
                    assert saved == {"frequency": -0.8, "variety": 0.65}
                    active = service("activate")
                    assert active["playback_mode"] == "shuffle"
                    assert active["effective_shuffle_strategy"] == strategy

                    # Import distinct WAVs, then navigate while stopped; no audio is played.
                    for index, frequency in enumerate((330, 440, 550, 660)):
                        path = root / f"tone-{index}.wav"
                        samples = array.array("h", (int(1000 * math.sin(2 * math.pi * frequency * n / 8000)) for n in range(8000)))
                        if os.sys.byteorder != "little":
                            samples.byteswap()
                        with wave.open(str(path), "wb") as audio:
                            audio.setparams((1, 2, 8000, 0, "NONE", "not compressed"))
                            audio.writeframes(samples.tobytes())
                        call("queue.add", {"path": str(path)})
                    previous = None
                    for _ in range(8):
                        state = call("player.next", {"preserve_state": True})
                        current = state["song"]["queue_id"]
                        assert current != previous
                        assert state["effective_shuffle_strategy"] == strategy
                        previous = current
                    # Reconfigure with a live queue to exercise synchronous registration changes.
                    assert service("configure", {"frequency": 0.8})["frequency"] == 0.8
                    assert call("player.status")["effective_shuffle_strategy"] == strategy
                    call("extensions.disable", {"id": owner})
                    fallback = call("player.status")
                    assert fallback["shuffle_strategy"] == strategy
                    assert fallback["effective_shuffle_strategy"] == "shuffle_bag"
                    call("extensions.enable", {"id": owner})
                    assert service("settings")["frequency"] == 0.8
                    assert call("player.status")["effective_shuffle_strategy"] == strategy
                    call("extensions.reload", {"id": owner})
                    assert call("player.status")["effective_shuffle_strategy"] == strategy
                    entries = call("extensions.list")["extensions"]
                    entry = next(item for item in entries if item["id"] == owner)
                    assert entry["state"] == "running"
                    assert not entry["contributes"].get("pages")
                    assert entry["contributes"]["settings"][0]["shuffleStrategy"] == "compass"
                    assert entry["contributes"]["settings"][0]["source"].endswith("qml/Page.qml")
                    print("Real-host ZIP install, UI contributions, settings, navigation, fallback, re-enable and reload passed")
            finally:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", type=Path, required=True)
    parser.add_argument("--package", type=Path, required=True)
    args = parser.parse_args()
    smoke(args.host, args.package)
