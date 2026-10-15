# README 逐句核对与标错记录

核对日期：2026-10-15。工具：ChatGPT/Codex。

我把 AI 起草的 README 与当前源码、Makefile、测试脚本和已保存的板端日志逐项对照。以下记录包含本次实际发现的文档遗漏及修正，不把此前没有发生的错误补写成历史缺陷。程序和测试代码没有变化，板端 28 项测试结论沿用 2026-10-13 的原始日志，本次不冒充重新上板运行。

## 本次发现并修正的问题

| 编号 | 核对前的表述或问题 | 标错与原因 | 修正后的说明 | 依据 |
| --- | --- | --- | --- | --- |
| B01 | 首次下载仅列新目标或同尺寸全零目标 | 适用范围遗漏：已有空文件也被源码接受 | 补列已有空文件 | download_file：只有非零长度且长度不等才拒绝，随后 require_empty_data 和 ftruncate |
| B02 | 仅笼统写“最小文件大小为 131072” | 命令约束不够清楚：download 的源文件也有最小长度 | 分别注明 create SIZE 与 download SOURCE 的 131072 下限 | MIN_SIZE、main、download_file；PASS 08、28 |
| B03 | write TEXT 未解释参数边界 | 遗漏命令行文本的字节长度、空格与 NUL 限制 | 注明一个参数、含空格加引号、按字节计长、不可嵌入 NUL | main argc==5；modify_file 使用 strlen 与 write_all |
| B04 | evidence/README.md 列出 demo.log、first-run-observed.txt | 提交文件不一致：两份历史材料未随公开包发布 | 删除未发布文件条目，只链接本次实际存在的日志和图片 | 公开目录逐文件检查 |
| B05 | README 仅指向 AI 协作日志 | 未给出逐句核对材料入口，难以查找标错依据 | 新增 README_REVIEW.md 入口及协作记录形式说明 | 本文件与 AI_COLLABORATION.md |

源码还确认：零长度文件的空洞估算按 0% 显示；因此本次也补充了公式的零分母分支。以上 B01-B03 属于文档说明修正，没有新增对应板端测试，依据是当前源码检查。

## 最终 README 逐句核对

核对版本 SHA256：`23948d9b1244e028697b793e513e3a7185798349ab951501b80132a00c766fb6`。行号以同版本 README.md 为准；标题、空行和代码围栏不作为技术陈述计数，命令示例逐行保留。

| 编号 / 行号 | 原句或示例 | 结论 | 核对依据 |
| --- | --- | --- | --- |
| R01 / L3 | Linux 文件 IO 课程实验，使用 C / POSIX 接口实现。 | 通过，保留当前表述 | sparse_downloader.c、evidence/environment.txt、evidence/test.log；本次变更只含文档 |
| R02 / L3 | 最终程序于 2026-10-13 在鲁班猫 aarch64 开发板、ext4 文件系统上通过全部 28 项集成测试；2026-10-15 补充核对提交文档，C 源码与测试脚本未改动。 | 通过，保留当前表述 | sparse_downloader.c、evidence/environment.txt、evidence/test.log；本次变更只含文档 |
| R03 / L7 | - `create`：生成逻辑大小至少 128 KiB 的空洞文件。 | 通过，保留当前表述 | create_file、modify_file、download_file、status_file；PASS 01-05、11-15 |
| R04 / L8 | - `write`：按命令行偏移写入文本，保留其他区域与逻辑大小。 | 通过，保留当前表述 | create_file、modify_file、download_file、status_file；PASS 01-05、11-15 |
| R05 / L9 | - `download`：用本地源文件模拟分块下载，保存已完成偏移，支持 SIGINT / Ctrl+C 暂停与重启续传。 | 通过，保留当前表述 | create_file、modify_file、download_file、status_file；PASS 01-05、11-15 |
| R06 / L10 | - `status`：显示逻辑大小、512B 分配块数、实际分配字节、估算空洞占比和下载进度。 | 通过，保留当前表述 | create_file、modify_file、download_file、status_file；PASS 01-05、11-15 |
| R07 / L12 | 文件内容通过 open、read、write、lseek、fstat 等文件 IO 接口处理。 | 通过，保留当前表述 | create_file、modify_file、download_file、status_file；PASS 01-05、11-15 |
| R08 / L12 | 程序不包含 HTTP 网络协议。 | 通过，保留当前表述 | create_file、modify_file、download_file、status_file；PASS 01-05、11-15 |
| R09 / L16 | 在支持磁盘空洞的 Linux 文件系统中，进入项目目录运行： | 通过，保留当前表述 | Makefile all/test 与 CFLAGS；测试使用 st_blocks；evidence/build.log、test.log |
| R10 / L19 | make | 通过，示例与接口一致 | Makefile all/test 与 CFLAGS；测试使用 st_blocks；evidence/build.log、test.log |
| R11 / L20 | make test | 通过，示例与接口一致 | Makefile all/test 与 CFLAGS；测试使用 st_blocks；evidence/build.log、test.log |
| R12 / L23 | 编译器须支持 C11，测试需要 Python3；默认编译选项为 `-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror`。 | 通过，保留当前表述 | Makefile all/test 与 CFLAGS；测试使用 st_blocks；evidence/build.log、test.log |
| R13 / L24 | 板端原始测试结果见 [evidence/test.log](evidence/test.log)，缺陷审查与修复见 [AI_REVIEW.md](AI_REVIEW.md)。 | 通过，保留当前表述 | Makefile all/test 与 CFLAGS；测试使用 st_blocks；evidence/build.log、test.log |
| R14 / L29 | ./sparse_downloader create FILE SIZE | 通过，示例与接口一致 | main、number、MIN_SIZE、MAX_CHUNK、modify_file 的 argc/strlen 检查 |
| R15 / L30 | ./sparse_downloader write FILE OFFSET TEXT | 通过，示例与接口一致 | main、number、MIN_SIZE、MAX_CHUNK、modify_file 的 argc/strlen 检查 |
| R16 / L31 | ./sparse_downloader status FILE | 通过，示例与接口一致 | main、number、MIN_SIZE、MAX_CHUNK、modify_file 的 argc/strlen 检查 |
| R17 / L32 | ./sparse_downloader download SOURCE TARGET [CHUNK_BYTES] [DELAY_MS] | 通过，示例与接口一致 | main、number、MIN_SIZE、MAX_CHUNK、modify_file 的 argc/strlen 检查 |
| R18 / L35 | 大小与偏移均使用十进制字节数，不接受 `128K` 这样的单位后缀。 | 通过，保留当前表述 | main、number、MIN_SIZE、MAX_CHUNK、modify_file 的 argc/strlen 检查 |
| R19 / L36 | `create` 的 SIZE 至少为 131072 字节；`download` 的源文件也必须至少为 131072 字节。 | 已补充 B02 | main、number、MIN_SIZE、MAX_CHUNK、modify_file 的 argc/strlen 检查 |
| R20 / L37 | 分块默认 4096 字节，范围为 1 至 1048576；延迟默认 0 毫秒，范围为 0 至 60000。 | 通过，保留当前表述 | main、number、MIN_SIZE、MAX_CHUNK、modify_file 的 argc/strlen 检查 |
| R21 / L38 | `write` 的 TEXT 是一个命令行文本参数，含空格时需要引号；长度按字节计算，不支持嵌入 NUL，二进制内容可由 `download` 复制。 | 已补充 B03 | main、number、MIN_SIZE、MAX_CHUNK、modify_file 的 argc/strlen 检查 |
| R22 / L42 | 使用新的文件名，避免与旧实验文件重名。 | 通过，保留当前表述 | main 参数分支；create_file；evidence/01-create.log、02-write.log；PASS 01-06 |
| R23 / L45 | ./sparse_downloader create hole.bin 131072 | 通过，示例与接口一致 | main 参数分支；create_file；evidence/01-create.log、02-write.log；PASS 01-06 |
| R24 / L46 | ./sparse_downloader status hole.bin | 通过，示例与接口一致 | main 参数分支；create_file；evidence/01-create.log、02-write.log；PASS 01-06 |
| R25 / L47 | ./sparse_downloader write hole.bin 65536 HELLO | 通过，示例与接口一致 | main 参数分支；create_file；evidence/01-create.log、02-write.log；PASS 01-06 |
| R26 / L48 | od -An -tx1 -j 65532 -N 16 hole.bin | 通过，示例与接口一致 | main 参数分支；create_file；evidence/01-create.log、02-write.log；PASS 01-06 |
| R27 / L51 | 本次板端创建后的逻辑大小为 131072 字节，实际分配 4096 字节；写入 HELLO 后逻辑大小保持不变，实际分配空间为 8192 字节。 | 通过，保留当前表述 | main 参数分支；create_file；evidence/01-create.log、02-write.log；PASS 01-06 |
| R28 / L52 | 上述分配量来自本次 ext4 实测，其他文件系统或分配策略的结果可能不同。 | 通过，保留当前表述 | main 参数分支；create_file；evidence/01-create.log、02-write.log；PASS 01-06 |
| R29 / L56 | 在项目目录使用新的 source.bin、download.bin 文件名执行： | 通过，保留当前表述 | 源数据表达式 256×1024=262144；main、download_file、save_progress、check_source、check_prefix、lock_file；PASS 11-19、25-27 |
| R30 / L59 | python3 -c 'from pathlib import Path; Path("source.bin").write_bytes(bytes(range(256))*1024)' | 通过，示例与接口一致 | 源数据表达式 256×1024=262144；main、download_file、save_progress、check_source、check_prefix、lock_file；PASS 11-19、25-27 |
| R31 / L60 | ./sparse_downloader download source.bin download.bin 4096 150 | 通过，示例与接口一致 | 源数据表达式 256×1024=262144；main、download_file、save_progress、check_source、check_prefix、lock_file；PASS 11-19、25-27 |
| R32 / L61 | # 中途按 Ctrl+C，待程序退出后执行： | 通过，示例与接口一致 | 源数据表达式 256×1024=262144；main、download_file、save_progress、check_source、check_prefix、lock_file；PASS 11-19、25-27 |
| R33 / L62 | ./sparse_downloader status download.bin | 通过，示例与接口一致 | 源数据表达式 256×1024=262144；main、download_file、save_progress、check_source、check_prefix、lock_file；PASS 11-19、25-27 |
| R34 / L63 | ./sparse_downloader download source.bin download.bin 4096 | 通过，示例与接口一致 | 源数据表达式 256×1024=262144；main、download_file、save_progress、check_source、check_prefix、lock_file；PASS 11-19、25-27 |
| R35 / L64 | cmp source.bin download.bin | 通过，示例与接口一致 | 源数据表达式 256×1024=262144；main、download_file、save_progress、check_source、check_prefix、lock_file；PASS 11-19、25-27 |
| R36 / L65 | sha256sum source.bin download.bin | 通过，示例与接口一致 | 源数据表达式 256×1024=262144；main、download_file、save_progress、check_source、check_prefix、lock_file；PASS 11-19、25-27 |
| R37 / L68 | 暂停时保留目标及其 `.resume` 记录；不要删除记录后强行重新初始化已有目标。 | 通过，保留当前表述 | 源数据表达式 256×1024=262144；main、download_file、save_progress、check_source、check_prefix、lock_file；PASS 11-19、25-27 |
| R38 / L69 | 程序先同步目标数据，再通过临时文件及 rename 提交进度，并同步记录所在目录。 | 通过，保留当前表述 | 源数据表达式 256×1024=262144；main、download_file、save_progress、check_source、check_prefix、lock_file；PASS 11-19、25-27 |
| R39 / L70 | 恢复时检查源身份、大小与时间戳、目标身份及已完成前缀；发现不一致即拒绝恢复。 | 通过，保留当前表述 | 源数据表达式 256×1024=262144；main、download_file、save_progress、check_source、check_prefix、lock_file；PASS 11-19、25-27 |
| R40 / L71 | 锁为 fcntl 建议性锁，不能阻止不遵守锁约定的其他程序；下载期间状态查询可能提示忙，暂停后可查看。 | 通过，保留当前表述 | 源数据表达式 256×1024=262144；main、download_file、save_progress、check_source、check_prefix、lock_file；PASS 11-19、25-27 |
| R41 / L75 | - `create` 不覆盖已有文件，`write` 拒绝越界；已有下载记录的目标拒绝手动写入。 | 通过，保留当前表述 | create_file、modify_file、download_file、require_empty_data、status_file；PASS 06-10、17-28；未测范围见 AI_REVIEW.md |
| R42 / L76 | - 首次下载可使用不存在的目标、已有空文件，或与源同尺寸且内容全零的已有目标；无记录且含非零数据的目标被拒绝。 | 已补充 B01 | create_file、modify_file、download_file、require_empty_data、status_file；PASS 06-10、17-28；未测范围见 AI_REVIEW.md |
| R43 / L77 | - 仅支持 Linux 普通源文件和目标文件，拒绝符号链接及源目标指向同一 inode 的情况。 | 通过，保留当前表述 | create_file、modify_file、download_file、require_empty_data、status_file；PASS 06-10、17-28；未测范围见 AI_REVIEW.md |
| R44 / L78 | - 空洞占比根据 `max(0, 1 - st_blocks * 512 / st_size) * 100%` 估算，空文件按 0% 显示，不是逐字节测量。 | 通过，保留当前表述 | create_file、modify_file、download_file、require_empty_data、status_file；PASS 06-10、17-28；未测范围见 AI_REVIEW.md |
| R45 / L79 | - SIGKILL 中断与恢复已有对应用例通过；未进行真实断电、磁盘满或存储硬件故障实验。 | 通过，保留当前表述 | create_file、modify_file、download_file、require_empty_data、status_file；PASS 06-10、17-28；未测范围见 AI_REVIEW.md |
| R46 / L83 | 四张图片和同名日志位于 [evidence](evidence)。 | 通过，保留当前表述 | evidence/README.md、environment.txt、四组日志与 PNG、board_demo.py、board_capture.sh |
| R47 / L84 | 程序先在板端实际运行并保存结果，再在板端原生 X11 终端显示日志并截屏。 | 通过，保留当前表述 | evidence/README.md、environment.txt、四组日志与 PNG、board_demo.py、board_capture.sh |
| R48 / L85 | 中断演示由脚本发送 SIGINT，与 Ctrl+C 对前台进程发送的信号一致。 | 通过，保留当前表述 | evidence/README.md、environment.txt、四组日志与 PNG、board_demo.py、board_capture.sh |
| R49 / L86 | 开发板时钟未同步，截图显示的 2024 年日期不代表实际实验日期。 | 通过，保留当前表述 | evidence/README.md、environment.txt、四组日志与 PNG、board_demo.py、board_capture.sh |
| R50 / L90 | 工具为 ChatGPT/Codex；七条提示词的过程整理、AI 建议和我的采纳/拒绝判断见 [AI_COLLABORATION.md](AI_COLLABORATION.md)。 | 通过，保留当前表述 | AI_COLLABORATION.md、README_REVIEW.md；公开目录不含报告或个人信息 |
| R51 / L91 | README 的逐句核对、发现的问题、修改理由与验证依据见 [README_REVIEW.md](README_REVIEW.md)。 | 已补充 B05 | AI_COLLABORATION.md、README_REVIEW.md；公开目录不含报告或个人信息 |
| R52 / L92 | 提示词按已完成开发过程整理，不作为逐字原始聊天导出；实验报告、个人姓名与学号不在本仓库中。 | 通过，保留当前表述 | AI_COLLABORATION.md、README_REVIEW.md；公开目录不含报告或个人信息 |
| R53 / L96 | 可点击 GitHub 仓库页面的 **Code → Download ZIP** 下载完整项目。 | 通过，保留当前表述 | 公开仓库 ZIP；发布后匿名下载并按文件逐字节校验 |

本次共核对 53 条陈述或示例行。相对链接和文件名在发布前检查；最终公开 ZIP 发布后再做匿名下载核验。

## 我的采纳判断

我采纳上述补充，使说明与程序实际接受的参数、文件状态及提交内容一致。对空洞比例、信号中断和硬件故障仍保留范围说明，避免把估算写成精确测量、把脚本信号测试写成人工操作或真实断电。
