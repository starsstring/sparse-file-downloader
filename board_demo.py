#!/usr/bin/env python3
"""生成真实板端演示日志；截图由 board_capture.sh 捕获开发板 X11 终端。"""
from pathlib import Path
import os
import signal
import subprocess
import time

root = Path(__file__).resolve().parent
os.chdir(root)
evidence = root / "evidence"
evidence.mkdir(exist_ok=True)
work = Path(__import__("tempfile").mkdtemp(prefix="demo-", dir=str(evidence)))
exe = str(root / "sparse_downloader")


def command(args):
    result = subprocess.run(args, capture_output=True, text=True, timeout=30)
    if result.returncode:
        raise RuntimeError(result.stderr or result.stdout)
    readable = " ".join(Path(a).name if a.startswith(str(root)) else a for a in args)
    return "$ " + readable + "\n" + result.stdout


header = command(["uname", "-a"])
hole = str(work / "hole.bin")
first = header + command([exe, "create", hole, "131072"])
first += command([exe, "status", hole])
first += command(["ls", "-ls", hole])
(evidence / "01-create.log").write_text(first)

second = header + command([exe, "write", hole, "65536", "HELLO"])
second += command([exe, "status", hole])
second += command(["od", "-An", "-tx1", "-j", "65532", "-N", "16", hole])
(evidence / "02-write.log").write_text(second)

source = work / "source.bin"
target = work / "download.bin"
source.write_bytes(bytes(range(256)) * 1024)
meta = Path(str(target) + ".resume")
with (work / "interrupted.log").open("w") as log:
    proc = subprocess.Popen([exe, "download", str(source), str(target), "16384", "150"],
                            stdout=log, stderr=log)
    try:
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            if meta.exists() and int(meta.read_text().split()[2]) >= 32768:
                break
            if proc.poll() is not None:
                raise RuntimeError("download ended before signal")
            time.sleep(0.01)
        else:
            raise RuntimeError("no partial progress")
        proc.send_signal(signal.SIGINT)
        rc = proc.wait(timeout=10)
        if rc != 130:
            raise RuntimeError(f"unexpected exit {rc}")
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.wait()
third = header + "$ sparse_downloader download source.bin download.bin 16384 150\n"
third += (work / "interrupted.log").read_text()
third += "SIGINT sent (same signal as Ctrl+C); exit_code=130\n"
third += command([exe, "status", str(target)])
(evidence / "03-paused.log").write_text(third)
fourth = header + command([exe, "download", str(source), str(target), "65536"])
fourth += command([exe, "status", str(target)])
fourth += command(["sha256sum", str(source), str(target)])
if source.read_bytes() != target.read_bytes():
    raise RuntimeError("download mismatch")
fourth += "Byte-for-byte comparison: PASS\n"
(evidence / "04-resumed.log").write_text(fourth)
print(first + second + third + fourth)
