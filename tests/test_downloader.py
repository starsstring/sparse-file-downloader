#!/usr/bin/env python3
"""Linux 集成测试：真实执行 C 程序，检查磁盘空洞、信号中断和二进制一致性。"""
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time

EXE = str(Path(sys.argv[1] if len(sys.argv) > 1 else "./sparse_downloader").resolve())
passed = 0


def run(*args, ok=True):
    result = subprocess.run([EXE, *map(str, args)], capture_output=True, text=True, timeout=30)
    if ok:
        assert result.returncode == 0, (args, result.returncode, result.stdout, result.stderr)
    else:
        assert result.returncode != 0, (args, result.stdout)
    return result


def check(name, condition):
    global passed
    assert condition, name
    passed += 1
    print(f"PASS {passed:02d}: {name}", flush=True)


def offset(target):
    return int(Path(str(target) + ".resume").read_text().split()[2])


def interrupt(source, target, sig, verify_lock=False):
    # 输出到文件避免管道缓冲填满；等待实际产生至少一块进度后发送信号。
    with (target.parent / (target.name + ".log")).open("w") as log:
        proc = subprocess.Popen([EXE, "download", str(source), str(target), "4096", "150"],
                                stdout=log, stderr=log)
        deadline = time.monotonic() + 10
        try:
            while time.monotonic() < deadline:
                try:
                    if offset(target) >= 4096:
                        break
                except (FileNotFoundError, IndexError):
                    pass
                assert proc.poll() is None, "download ended before interruption"
                time.sleep(0.01)
            else:
                raise AssertionError("no progress within 10 seconds")
            if verify_lock:
                conflict = run("download", source, target, ok=False)
                check("concurrent downloader is rejected by the file lock", "fcntl lock" in conflict.stderr)
                conflict = run("write", target, 0, "BAD", ok=False)
                check("concurrent manual writer is rejected by the file lock", "fcntl lock" in conflict.stderr)
            proc.send_signal(sig)
            rc = proc.wait(timeout=10)
            return rc
        finally:
            if proc.poll() is None:
                proc.kill()
                proc.wait()


with tempfile.TemporaryDirectory(prefix="sparse-homework-") as tmp:
    root = Path(tmp)
    hole = root / "hole.bin"
    run("create", hole, 131072)
    st = hole.stat()
    check("create: logical size is 128 KiB", st.st_size == 131072)
    check("create: disk allocation is smaller than logical size (sparse filesystem required)",
          st.st_blocks * 512 < st.st_size)
    check("unwritten hole reads as zero bytes", hole.read_bytes() == bytes(131072))
    run("write", hole, 65536, "HELLO")
    content = hole.read_bytes()
    check("positioned write preserves other regions and file size",
          len(content) == 131072 and content[65536:65541] == b"HELLO"
          and content[:65536] == bytes(65536) and content[65541:] == bytes(65531))
    check("status reports size, allocation and hole ratio",
          all(key in run("status", hole).stdout for key in
              ("logical_bytes=131072", "allocated_512b_blocks=", "hole_ratio_estimate=")))
    run("write", hole, 131070, "OVERFLOW", ok=False)
    check("out-of-range write does not alter data", hole.read_bytes() == content)
    run("create", hole, 131072, ok=False)
    check("create refuses to overwrite existing file", hole.read_bytes() == content)
    for size in ("-1", "12K", "131071", "9223372036854775808"):
        run("create", root / "invalid", size, ok=False)
    check("invalid sizes are rejected without creating a target", not (root / "invalid").exists())

    # 源文件含零字节、所有 256 种字节值，大小不是分块整数倍。
    source = root / "source.bin"
    data = bytes(8192) + bytes(range(256)) * 1000 + b"END\x00\xff"
    source.write_bytes(data)
    target = root / "download.bin"
    rc = interrupt(source, target, signal.SIGINT, verify_lock=True)
    saved = offset(target)
    check("SIGINT stops cleanly and records a partial offset", rc == 130 and 0 < saved < len(data))
    check("partial transfer keeps full logical size and unwritten holes",
          target.stat().st_size == len(data) and target.stat().st_blocks * 512 < len(data))
    resumed = run("download", source, target, 7777)
    check("restart uses persisted offset", f"Resume at offset={saved}" in resumed.stdout)
    check("resumed binary output matches source byte for byte", target.read_bytes() == data)
    check("complete status is correct", "state=complete" in run("status", target).stdout)
    check("completed download is safe to run again", "Download complete." in run("download", source, target).stdout)
    run("write", target, 0, "X", ok=False)
    check("manual writes cannot invalidate a download record", target.read_bytes() == data)

    killed = root / "killed.bin"
    rc = interrupt(source, killed, signal.SIGKILL)
    check("forced kill leaves a readable checkpoint", rc == -signal.SIGKILL and 0 < offset(killed) < len(data))
    run("download", source, killed)
    check("forced-kill recovery produces identical data", killed.read_bytes() == data)

    alias = root / "alias.bin"
    os.link(source, alias)
    run("download", source, alias, ok=False)
    check("source/target hard-link alias is rejected without truncation", source.read_bytes() == data)
    symlink = root / "symlink.bin"
    symlink.symlink_to(source)
    run("download", source, symlink, ok=False)
    check("symlink destination is rejected", source.read_bytes() == data)

    bad = root / "bad.bin"
    interrupt(source, bad, signal.SIGINT)
    Path(str(bad) + ".resume").write_text("BROKEN\n")
    run("download", source, bad, ok=False)
    check("corrupt metadata is rejected", True)
    Path(str(bad) + ".resume").write_text("SPARSE_V1 " + "9" * 100 + " 0 1 1 1 1 0 0 0 0\n")
    run("download", source, bad, ok=False)
    check("overflowing metadata integer is rejected", True)

    foreign = root / "foreign.bin"
    foreign.write_bytes(b"IMPORTANT" + bytes(len(data) - 9))
    run("download", source, foreign, ok=False)
    check("existing unrelated target data is preserved", foreign.read_bytes().startswith(b"IMPORTANT"))

    changed = root / "changed.bin"
    interrupt(source, changed, signal.SIGINT)
    with changed.open("r+b") as f:
        f.write(b"X")
    run("download", source, changed, ok=False)
    check("altered completed prefix is detected", True)

    replaced = root / "replaced.bin"
    interrupt(source, replaced, signal.SIGINT)
    moved = root / "old-target.bin"
    replaced.rename(moved)
    replaced.write_bytes(bytes(len(data)))
    run("download", source, replaced, ok=False)
    check("replaced target inode is detected", True)

    stale = root / "stale.bin"
    interrupt(source, stale, signal.SIGINT)
    with source.open("r+b") as f:
        f.seek(10000)
        f.write(b"modified")
    run("download", source, stale, ok=False)
    check("source modification is detected", True)

    small = root / "small.bin"
    small.write_bytes(b"small")
    run("download", small, root / "small-target", ok=False)
    check("download source smaller than 128 KiB is rejected", not (root / "small-target").exists())

print(f"ALL {passed} TESTS PASSED")
