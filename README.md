# 空洞文件断点续传下载器

Linux 文件 IO 课程实验，使用 C / POSIX 接口实现。最终版本于 2026-10-13 在鲁班猫 aarch64 开发板、ext4 文件系统上通过全部 28 项集成测试。

## 功能

- `create`：生成逻辑大小至少 128 KiB 的空洞文件。
- `write`：按命令行偏移写入文本，保留其他区域与逻辑大小。
- `download`：用本地源文件模拟分块下载，保存已完成偏移，支持 SIGINT / Ctrl+C 暂停与重启续传。
- `status`：显示逻辑大小、512B 分配块数、实际分配字节、估算空洞占比和下载进度。

文件内容通过 open、read、write、lseek、fstat 等文件 IO 接口处理。程序不包含 HTTP 网络协议。

## 编译与测试

在 Linux 终端运行：

```sh
make
make test
```

编译器支持 C11，测试需要 Python3。默认使用 `-O2 -Wall -Wextra -Wpedantic -Werror`。测试日志见 [evidence/test.log](evidence/test.log)，代码审查见 [AI_REVIEW.md](AI_REVIEW.md)。

## 命令

```text
./sparse_downloader create FILE SIZE
./sparse_downloader write FILE OFFSET TEXT
./sparse_downloader status FILE
./sparse_downloader download SOURCE TARGET [CHUNK_BYTES] [DELAY_MS]
```

大小与偏移均使用十进制字节数。最小文件大小为 131072；默认分块为 4096 字节，允许范围为 1 至 1048576；延迟单位为毫秒，默认 0，允许范围为 0 至 60000。

## 创建与修改演示

使用新的文件名，避免与旧实验文件重名。

```sh
./sparse_downloader create hole.bin 131072
./sparse_downloader status hole.bin
./sparse_downloader write hole.bin 65536 HELLO
od -An -tx1 -j 65532 -N 16 hole.bin
```

实测创建后的逻辑大小为 131072 字节，实际分配 4096 字节。写入 HELLO 后逻辑大小保持不变，实际分配空间为 8192 字节。

## 续传演示

```sh
python3 -c 'from pathlib import Path; Path("source.bin").write_bytes(bytes(range(256))*1024)'
./sparse_downloader download source.bin download.bin 4096 150
# 中途按 Ctrl+C，再执行：
./sparse_downloader status download.bin
./sparse_downloader download source.bin download.bin 4096
cmp source.bin download.bin
sha256sum source.bin download.bin
```

必须保留目标文件及其 `.resume` 记录。程序先同步目标数据，再原子提交进度。恢复时检查源身份与时间戳、目标身份及已完成前缀。文件锁为建议性锁，下载期间状态查询可能提示忙，暂停后可查看。

## 文件保护与限制

- create 不覆盖已有文件，write 拒绝越界。
- 已有下载记录的目标拒绝手动写入。
- 初次下载可使用新目标，或同尺寸、内容全零的已有目标；无记录但含非零数据的目标被拒绝。
- 仅支持 Linux 普通文件，拒绝符号链接及源目标指向同一 inode 的情况。
- 空洞占比根据 `st_blocks * 512` 估算，不是逐字节测量。
- SIGKILL 恢复已测试；未进行真实断电、磁盘满或存储硬件故障实验。

## 运行证据

四张图片和同名日志位于 [evidence](evidence)。程序先在板端实际运行并保存结果，再在板端原生 X11 终端显示日志并截屏。中断演示由脚本发送 SIGINT，与 Ctrl+C 对前台进程发送的信号一致。开发板时钟未同步，截图显示的 2024 年日期不代表实际实验日期。

## AI 协作

工具为 ChatGPT/Codex。工作指令与采纳判断见 [AI_COLLABORATION.md](AI_COLLABORATION.md)。实验报告、个人姓名与学号不在本仓库中。

## 下载

可点击 GitHub 仓库页面的 **Code → Download ZIP** 下载完整项目。
