# 空洞文件断点续传下载器

Linux 文件 IO 课程实验，使用 C / POSIX 接口实现。最终程序于 2026-10-13 在鲁班猫 aarch64 开发板、ext4 文件系统上通过全部 28 项集成测试；2026-10-15 补充核对提交文档，C 源码与测试脚本未改动。

## 功能

- `create`：生成逻辑大小至少 128 KiB 的空洞文件。
- `write`：按命令行偏移写入文本，保留其他区域与逻辑大小。
- `download`：用本地源文件模拟分块下载，保存已完成偏移，支持 SIGINT / Ctrl+C 暂停与重启续传。
- `status`：显示逻辑大小、512B 分配块数、实际分配字节、估算空洞占比和下载进度。

文件内容通过 open、read、write、lseek、fstat 等文件 IO 接口处理。程序不包含 HTTP 网络协议。

## 编译与测试

在支持磁盘空洞的 Linux 文件系统中，进入项目目录运行：

```sh
make
make test
```

编译器须支持 C11，测试需要 Python3；默认编译选项为 `-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror`。
板端原始测试结果见 [evidence/test.log](evidence/test.log)，缺陷审查与修复见 [AI_REVIEW.md](AI_REVIEW.md)。

## 命令与参数

```text
./sparse_downloader create FILE SIZE
./sparse_downloader write FILE OFFSET TEXT
./sparse_downloader status FILE
./sparse_downloader download SOURCE TARGET [CHUNK_BYTES] [DELAY_MS]
```

大小与偏移均使用十进制字节数，不接受 `128K` 这样的单位后缀。
`create` 的 SIZE 至少为 131072 字节；`download` 的源文件也必须至少为 131072 字节。
分块默认 4096 字节，范围为 1 至 1048576；延迟默认 0 毫秒，范围为 0 至 60000。
`write` 的 TEXT 是一个命令行文本参数，含空格时需要引号；长度按字节计算，不支持嵌入 NUL，二进制内容可由 `download` 复制。

## 创建与修改演示

使用新的文件名，避免与旧实验文件重名。

```sh
./sparse_downloader create hole.bin 131072
./sparse_downloader status hole.bin
./sparse_downloader write hole.bin 65536 HELLO
od -An -tx1 -j 65532 -N 16 hole.bin
```

本次板端创建后的逻辑大小为 131072 字节，实际分配 4096 字节；写入 HELLO 后逻辑大小保持不变，实际分配空间为 8192 字节。
上述分配量来自本次 ext4 实测，其他文件系统或分配策略的结果可能不同。

## 续传演示

在项目目录使用新的 source.bin、download.bin 文件名执行：

```sh
python3 -c 'from pathlib import Path; Path("source.bin").write_bytes(bytes(range(256))*1024)'
./sparse_downloader download source.bin download.bin 4096 150
# 中途按 Ctrl+C，待程序退出后执行：
./sparse_downloader status download.bin
./sparse_downloader download source.bin download.bin 4096
cmp source.bin download.bin
sha256sum source.bin download.bin
```

暂停时保留目标及其 `.resume` 记录；不要删除记录后强行重新初始化已有目标。
程序先同步目标数据，再通过临时文件及 rename 提交进度，并同步记录所在目录。
恢复时检查源身份、大小与时间戳、目标身份及已完成前缀；发现不一致即拒绝恢复。
锁为 fcntl 建议性锁，不能阻止不遵守锁约定的其他程序；下载期间状态查询可能提示忙，暂停后可查看。

## 文件保护与限制

- `create` 不覆盖已有文件，`write` 拒绝越界；已有下载记录的目标拒绝手动写入。
- 首次下载可使用不存在的目标、已有空文件，或与源同尺寸且内容全零的已有目标；无记录且含非零数据的目标被拒绝。
- 仅支持 Linux 普通源文件和目标文件，拒绝符号链接及源目标指向同一 inode 的情况。
- 空洞占比根据 `max(0, 1 - st_blocks * 512 / st_size) * 100%` 估算，空文件按 0% 显示，不是逐字节测量。
- SIGKILL 中断与恢复已有对应用例通过；未进行真实断电、磁盘满或存储硬件故障实验。

## 运行证据

四张图片和同名日志位于 [evidence](evidence)。
程序先在板端实际运行并保存结果，再在板端原生 X11 终端显示日志并截屏。
中断演示由脚本发送 SIGINT，与 Ctrl+C 对前台进程发送的信号一致。
开发板时钟未同步，截图显示的 2024 年日期不代表实际实验日期。

## AI 协作与文档核对

工具为 ChatGPT/Codex；七条提示词的过程整理、AI 建议和我的采纳/拒绝判断见 [AI_COLLABORATION.md](AI_COLLABORATION.md)。
README 的逐句核对、发现的问题、修改理由与验证依据见 [README_REVIEW.md](README_REVIEW.md)。
提示词按已完成开发过程整理，不作为逐字原始聊天导出；实验报告、个人姓名与学号不在本仓库中。

## 下载

可点击 GitHub 仓库页面的 **Code → Download ZIP** 下载完整项目。
