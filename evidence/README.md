# 板端实测证据

实际测试日期：2026-10-13（北京时间）。设备：鲁班猫、aarch64、Ubuntu GCC 9.4.0、ext4。

| 文件 | 内容 |
| --- | --- |
| [environment.txt](environment.txt) | 开发板环境与原始板端时间 |
| [build.log](build.log) | 最终版板端严格编译命令 |
| [test.log](test.log) | 全部 28 项测试通过 |
| [01-create.log](01-create.log) / [01-create.png](01-create.png) | 128 KiB 逻辑大小，实际分配 4 KiB |
| [02-write.log](02-write.log) / [02-write.png](02-write.png) | 偏移 65536 写入 HELLO，逻辑大小不变 |
| [03-paused.log](03-paused.log) / [03-paused.png](03-paused.png) | SIGINT 暂停于 32768 字节，进度 12.50% |
| [04-resumed.log](04-resumed.log) / [04-resumed.png](04-resumed.png) | 从原偏移续传至完成，两个 SHA256 相同 |

程序先在开发板实际运行并保存结果，再在开发板原生 X11 终端显示日志并截屏。图片未进行图像编辑。演示中断由脚本发送 SIGINT，等价于 Ctrl+C 对前台进程发送的信号；没有进行真实断电实验。

板端时钟没有同步，画面显示 2024 年 6 月，实际测试与截屏发生于 2026-10-13。这里保留原始日期显示。本目录只列本次公开提交中实际包含的文件。
